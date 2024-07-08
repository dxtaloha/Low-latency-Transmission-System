#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/resource.h>
#include <sys/stat.h>

#include <bpf/bpf.h>
#include <xdp/xsk.h>
#include <xdp/libxdp.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/icmpv6.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fpe.h>
#include <fpe_locl.h>




#include "../common/common_params.h"
#include "../common/common_user_bpf_xdp.h"
#include "../common/common_libbpf.h"

#define NUM_FRAMES         4096
#define FRAME_SIZE         XSK_UMEM__DEFAULT_FRAME_SIZE
#define RX_BATCH_SIZE      64
#define INVALID_UMEM_FRAME UINT64_MAX

//FPE 相关变量
const char *key = "2DE79D232DF5585D68CE47882AE256D6";
const char *tweak = "CBD09280979564";
unsigned int radix = 256;


static struct xdp_program *prog;
int xsk_map_fd;
bool custom_xsk = false;
struct config cfg = {
        .ifindex   = -1,   // 网络接口索引无效（-1）
};

struct config cfg_tx = {
        .ifindex   = -1,   // 网络接口索引无效（-1）
};

struct xsk_umem_info {
    struct xsk_ring_prod fq; // 生产者队列fill queue(类比fill ring）
    struct xsk_ring_cons cq; // 消费者 Completion queue
    struct xsk_umem *umem;  // umem包括用于存储数据包的内存以及其他管理这些数据的结构，代表了整个区域，buffer用于存储一组数据包的具体内存区域
    void *buffer;
};
struct stats_record {
    uint64_t timestamp;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t tx_packets;
    uint64_t tx_bytes;
};
struct xsk_socket_info {
    struct xsk_ring_cons rx;
    struct xsk_ring_prod tx;
    struct xsk_umem_info *umem;
    struct xsk_socket *xsk;

    uint64_t umem_frame_addr[NUM_FRAMES];
    uint32_t umem_frame_free;

    uint32_t outstanding_tx;

    struct stats_record stats;
    struct stats_record prev_stats;
};


// Prepare pseudo-header for UDP checksum calculation
struct {
    uint32_t src_addr;
    uint32_t dest_addr;
    uint8_t zero;
    uint8_t proto;
    uint16_t length;
} pseudo_hdr;

// 计算生产者环中的剩余空间
static inline __u32 xsk_ring_prod__free(struct xsk_ring_prod *r)
{
    r->cached_cons = *r->consumer + r->size;
    return r->cached_cons - r->cached_prod;
}

static const char *__doc__ = "AF_XDP kernel bypass example\n";

static const struct option_wrapper long_options[] = {

        {{"help",	 no_argument,		NULL, 'h' },
                            "Show help", false},

        {{"dev",	 required_argument,	NULL, 'd' },
                            "Operate on device <ifname>", "<ifname>", true},

        {{"skb-mode",	 no_argument,		NULL, 'S' },
                            "Install XDP program in SKB (AKA generic) mode"},

        {{"native-mode", no_argument,		NULL, 'N' },
                            "Install XDP program in native mode"},

        {{"auto-mode",	 no_argument,		NULL, 'A' },
                            "Auto-detect SKB or native mode"},

        {{"force",	 no_argument,		NULL, 'F' },
                            "Force install, replacing existing program on interface"},

        {{"copy",        no_argument,		NULL, 'c' },
                            "Force copy mode"},

        {{"zero-copy",	 no_argument,		NULL, 'z' },
                            "Force zero-copy mode"},

        {{"queue",	 required_argument,	NULL, 'Q' },
                            "Configure interface receive queue for AF_XDP, default=0"},

        {{"poll-mode",	 no_argument,		NULL, 'p' },
                            "Use the poll() API waiting for packets to arrive"},

        {{"quiet",	 no_argument,		NULL, 'q' },
                            "Quiet mode (no output)"},

        {{"filename",    required_argument,	NULL,  1  },
                            "Load program from <file>", "<file>"},

        {{"progname",	 required_argument,	NULL,  2  },
                            "Load program from function <name> in the ELF file", "<name>"},

        {{0, 0, NULL,  0 }, NULL, false}
};

static bool global_exit;

// 初始化umem
static struct xsk_umem_info *configure_xsk_umem(void *buffer, uint64_t size)
{
    struct xsk_umem_info *umem;
    int ret;

    umem = calloc(1, sizeof(*umem));
    if (!umem)
        return NULL;

    ret = xsk_umem__create(&umem->umem, buffer, size, &umem->fq, &umem->cq,
                           NULL);
    if (ret) {
        errno = -ret;
        return NULL;
    }

    umem->buffer = buffer;
    return umem;
}

// 在已经配置好的umem中分配一个具体的内存帧frame
static uint64_t xsk_alloc_umem_frame(struct xsk_socket_info *xsk)
{
    uint64_t frame;
    if (xsk->umem_frame_free == 0)
        return INVALID_UMEM_FRAME;

    frame = xsk->umem_frame_addr[--xsk->umem_frame_free];
    xsk->umem_frame_addr[xsk->umem_frame_free] = INVALID_UMEM_FRAME;
    return frame;
}

// 空闲位置
static void xsk_free_umem_frame(struct xsk_socket_info *xsk, uint64_t frame)
{
    assert(xsk->umem_frame_free < NUM_FRAMES);

    xsk->umem_frame_addr[xsk->umem_frame_free++] = frame;
}

// 空闲位置
static uint64_t xsk_umem_free_frames(struct xsk_socket_info *xsk)
{
    return xsk->umem_frame_free;
}

static struct xsk_socket_info *xsk_configure_socket(struct config *cfg,
                                                    struct xsk_umem_info *umem)
{
    struct xsk_socket_config xsk_cfg;
    struct xsk_socket_info *xsk_info;
    uint32_t idx;
    int i;
    int ret;
    uint32_t prog_id;

    xsk_info = calloc(1, sizeof(*xsk_info));
    if (!xsk_info)
        return NULL;

    xsk_info->umem = umem;
    xsk_cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
    xsk_cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
    xsk_cfg.xdp_flags = cfg->xdp_flags;
    xsk_cfg.bind_flags = cfg->xsk_bind_flags;
    xsk_cfg.libbpf_flags = (custom_xsk) ? XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD: 0;
    ret = xsk_socket__create(&xsk_info->xsk, cfg->ifname,
                             cfg->xsk_if_queue, umem->umem, &xsk_info->rx,
                             &xsk_info->tx, &xsk_cfg);
    if (ret)
        goto error_exit;

    if (custom_xsk) {
        ret = xsk_socket__update_xskmap(xsk_info->xsk, xsk_map_fd);
        if (ret)
            goto error_exit;
    } else {
        /* Getting the program ID must be after the xdp_socket__create() call */
        if (bpf_xdp_query_id(cfg->ifindex, cfg->xdp_flags, &prog_id))
            goto error_exit;
    }

    /* Initialize umem frame allocation */
    for (i = 0; i < NUM_FRAMES; i++)
        xsk_info->umem_frame_addr[i] = i * FRAME_SIZE;

    xsk_info->umem_frame_free = NUM_FRAMES;

    /* Stuff the receive path with buffers, we assume we have enough */
    ret = xsk_ring_prod__reserve(&xsk_info->umem->fq,
                                 XSK_RING_PROD__DEFAULT_NUM_DESCS,
                                 &idx);

    if (ret != XSK_RING_PROD__DEFAULT_NUM_DESCS)
        goto error_exit;

    for (i = 0; i < XSK_RING_PROD__DEFAULT_NUM_DESCS; i ++)
        *xsk_ring_prod__fill_addr(&xsk_info->umem->fq, idx++) =
                xsk_alloc_umem_frame(xsk_info);

    xsk_ring_prod__submit(&xsk_info->umem->fq,
                          XSK_RING_PROD__DEFAULT_NUM_DESCS);

    return xsk_info;

    error_exit:
    errno = -ret;
    return NULL;
}

static void complete_tx(struct xsk_socket_info *xsk)
{
    unsigned int completed;
    uint32_t idx_cq;

    if (!xsk->outstanding_tx)
        return;

    sendto(xsk_socket__fd(xsk->xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);

    /* Collect/free completed TX buffers */
    completed = xsk_ring_cons__peek(&xsk->umem->cq,
                                    XSK_RING_CONS__DEFAULT_NUM_DESCS,
                                    &idx_cq);

    if (completed > 0) {
        for (int i = 0; i < completed; i++)
            xsk_free_umem_frame(xsk,
                                *xsk_ring_cons__comp_addr(&xsk->umem->cq,
                                                          idx_cq++));

        xsk_ring_cons__release(&xsk->umem->cq, completed);
        xsk->outstanding_tx -= completed < xsk->outstanding_tx ?
                               completed : xsk->outstanding_tx;
    }
}

static inline __sum16 csum16_add(__sum16 csum, __be16 addend)
{
uint16_t res = (uint16_t)csum;

res += (__u16)addend;
return (__sum16)(res + (res < (__u16)addend));
}

static inline __sum16 csum16_sub(__sum16 csum, __be16 addend)
{
return csum16_add(csum, ~addend);
}

static inline void csum_replace2(__sum16 *sum, __be16 old, __be16 new)
{
    *sum = ~csum16_add(csum16_sub(~(*sum), old), new);
}


// IP checksum
uint16_t checksum(void *data, size_t len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        if (sum & 0x80000000)
            sum = (sum & 0xFFFF) + (sum >> 16);
        len -= 2;
    }

    if (len > 0)
        sum += *((uint8_t *)ptr);

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)~sum;
}


// UDP or TCP checksum
uint16_t udp_tcp_checksum(struct iphdr *ip, void *hdr, uint8_t *payload, size_t payload_len) {
    uint8_t buf[1500];
    size_t header_len = (ip->protocol == IPPROTO_UDP) ? sizeof(struct udphdr) : sizeof(struct tcphdr);
    size_t total_len = header_len + payload_len;
    size_t pseudo_header_len = sizeof(struct iphdr) - ((ip->ihl - 5) * 4);

    // 构造伪头部
    memset(buf, 0, pseudo_header_len);
    memcpy(buf, &ip->saddr, sizeof(ip->saddr));
    memcpy(buf + 4, &ip->daddr, sizeof(ip->daddr));
    buf[9] = ip->protocol;
    *((uint16_t *)(buf + 10)) = htons(total_len);

    // 头部
    memcpy(buf + pseudo_header_len, hdr, header_len);

    // 载荷
    memcpy(buf + pseudo_header_len + header_len, payload, payload_len);

    // 计算伪头部 + UDP/TCP 头部 + 载荷的校验和
    return checksum(buf, pseudo_header_len + total_len);
}


static bool process_packet(struct xsk_socket_info *xsk,
                           struct xsk_socket_info *xsk_socket_tx,
                           uint64_t addr, uint32_t len)
{
    uint8_t *pkt = xsk_umem__get_data(xsk->umem->buffer, addr);

    // 打印接收到的数据包内容
    printf("Received Packet (len=%u):\n", len);
    for (uint32_t i = 0; i < len; ++i) {
        printf("%02x ", pkt[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    printf("================================\n");

    int ret;
    uint32_t tx_idx = 0;
    uint8_t tmp_mac[ETH_ALEN];
    uint32_t tmp_ip;

    struct ethhdr *eth = (struct ethhdr *) pkt;
    struct iphdr *ip = (struct iphdr *) (eth + 1);
    uint8_t *payload;
    uint32_t payload_len;


    if(ip->protocol == IPPROTO_UDP){
        struct udphdr *udp = (struct udphdr *)((uint8_t *)ip + ip->ihl * 4);
        payload = (uint8_t *)udp + sizeof(struct udphdr); // 指向UDP数据载荷
        payload_len = ntohs(udp->len) - sizeof(struct udphdr);


        // Switch MAC addresses
        memcpy(tmp_mac, eth->h_dest, ETH_ALEN);
        memcpy(eth->h_dest, eth->h_source, ETH_ALEN);
        memcpy(eth->h_source, tmp_mac, ETH_ALEN);


        // Switch IP addresses
        memcpy(&tmp_ip, &ip->saddr, 4);
        memcpy(&ip->saddr, &ip->daddr, 4);
        memcpy(&ip->daddr, &tmp_ip, 4);

        FPE_KEY *ff1_key = FPE_ff1_create_key(key, tweak, radix);
        FPE_ff1_encrypt(payload, payload, ff1_key, payload_len);  // 原地加密
        FPE_ff1_decrypt(payload, payload, ff1_key, payload_len);

        // 更新 UDP 长度
        udp->len = htons(payload_len + sizeof(struct udphdr));

        // 更新 IP 总长度
        ip->tot_len = htons(ip->ihl * 4 + ntohs(udp->len));

        // 将当前校验和字段置零
        ip->check = 0;
        udp->check = 0;

        // IP header checksum
        ip->check = checksum(ip, ip->ihl * 4);

        // UDP checksum including pseudo-header
        udp->check = udp_tcp_checksum(ip, udp, payload, payload_len);

        if (udp->check == 0) {
            udp->check = 0xFFFF;
            // 为了区分“没有生成校验和”和“生成了校验和但计算结果为0”的情况，将计算结果为0的校验和设置为0xFFFF
        }
    } else if (ip->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp = (struct tcphdr *)((uint8_t *)ip + ip->ihl * 4);
        payload = (uint8_t *)tcp + (tcp->doff * 4); // 指向TCP数据载荷
        payload_len = ntohs(ip->tot_len) - (ip->ihl * 4) - (tcp->doff * 4);


        // Switch MAC addresses
        memcpy(tmp_mac, eth->h_dest, ETH_ALEN);
        memcpy(eth->h_dest, eth->h_source, ETH_ALEN);
        memcpy(eth->h_source, tmp_mac, ETH_ALEN);


        // Switch IP addresses
        memcpy(&tmp_ip, &ip->saddr, 4);
        memcpy(&ip->saddr, &ip->daddr, 4);
        memcpy(&ip->daddr, &tmp_ip, 4);

        // 加密
        FPE_KEY *ff1_key = FPE_ff1_create_key(key, tweak, radix);
        FPE_ff1_encrypt(payload, payload, ff1_key, payload_len);  // 原地加密
        FPE_ff1_decrypt(payload, payload, ff1_key, payload_len);

        // 更新 IP 总长度
        ip->tot_len = htons(ip->ihl * 4 + (tcp->doff * 4) + payload_len);

        // 更新校验和
        ip->check = 0;
        tcp->check = 0;
        ip->check = checksum(ip, ip->ihl * 4);
        tcp->check = udp_tcp_checksum(ip, tcp, payload, payload_len);
        if (tcp->check == 0) {
            tcp->check = 0xFFFF;
        }
    }

    // Submit packet to tx ring
    ret = xsk_ring_prod__reserve(&xsk_socket_tx->tx, 1, &tx_idx);
    if (ret != 1) {
        // No more transmit slots, drop the packet
        return false;
    }


    // 数据包从UEME1 复制到 UMEME2
    uint64_t addr_tx = xsk_ring_prod__tx_desc(&xsk_socket_tx->tx, tx_idx)->addr;
    uint8_t *dest = xsk_umem__get_data(xsk_socket_tx->umem->buffer, addr_tx);
    memcpy(dest, pkt, len);


    printf("copy success!\n");
    printf("================================\n");
    printf("copy Packet (len=%u):\n", len);
    for (uint32_t i = 0; i < len; ++i) {
        printf("%02x ", dest[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");


    xsk_ring_prod__tx_desc(&xsk_socket_tx->tx, tx_idx)->addr = addr_tx;
    xsk_ring_prod__tx_desc(&xsk_socket_tx->tx, tx_idx)->len = len;
    xsk_ring_prod__submit(&xsk_socket_tx->tx, 1);
    xsk_socket_tx->outstanding_tx++;

    xsk_socket_tx->stats.tx_bytes += len;
    xsk_socket_tx->stats.tx_packets++;

    return true;
}


static void handle_receive_packets(struct xsk_socket_info *xsk,
                                   struct xsk_socket_info *xsk_socket_tx)
{
    unsigned int rcvd, stock_frames, i;
    uint32_t idx_rx = 0, idx_fq = 0;
    int ret;

//    xsk_ring_cons__peek：开始对RX RING进行消费，返回可消费个数，并累加cached_cons；
//    与release相比，peek不会将数据从环中释放
    rcvd = xsk_ring_cons__peek(&xsk->rx, RX_BATCH_SIZE, &idx_rx);
    if (!rcvd)
        return;

    /* Stuff the ring with as much frames as possible */
    // fq当前可用于生成数据的空闲位置（队列引起，限制最大值）
    stock_frames = xsk_prod_nb_free(&xsk->umem->fq,
                                    xsk_umem_free_frames(xsk));

    if (stock_frames > 0) {
//    xsk_ring_prod__reserve：开始对FILL RING进行生产，返回可生产个数，并累加cached_prod；
        ret = xsk_ring_prod__reserve(&xsk->umem->fq, stock_frames,
                                     &idx_fq);

        /* This should not happen, but just in case */
        while (ret != stock_frames)
            ret = xsk_ring_prod__reserve(&xsk->umem->fq, rcvd,
                                         &idx_fq);
//        while (ret != stock_frames)
//            ret = xsk_ring_prod__reserve(&xsk->umem->fq, stock_frames,
//                                         &idx_fq);

//      填充fill ring
        for (i = 0; i < stock_frames; i++)
            *xsk_ring_prod__fill_addr(&xsk->umem->fq, idx_fq++) =
                    xsk_alloc_umem_frame(xsk);

        xsk_ring_prod__submit(&xsk->umem->fq, stock_frames);
    }

    /* Process received packets */
    for (i = 0; i < rcvd; i++) {
        uint64_t addr = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx)->addr;
        uint32_t len = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx++)->len;

        // 将数据包复制到UMEM2处理，若成功发送（true)则将xsk的数据包删除
        if (process_packet(xsk, xsk_socket_tx, addr, len)){
            xsk_free_umem_frame(xsk, addr);
        }

        // 更新接收数量
        xsk->stats.rx_bytes += len;
    }

    // 是否已经处理的数据包的描述符
    xsk_ring_cons__release(&xsk->rx, rcvd);
    xsk->stats.rx_packets += rcvd;

    /* Do we need to wake up the kernel for transmission */
    complete_tx(xsk_socket_tx);
}

static void rx_and_process(struct config *cfg,
                           struct xsk_socket_info *xsk_socket,
                           struct xsk_socket_info *xsk_socket_tx)
{
    struct pollfd fds[2];
    int ret, nfds = 1;

    memset(fds, 0, sizeof(fds)); //初始化fds为0
    fds[0].fd = xsk_socket__fd(xsk_socket->xsk);
    fds[0].events = POLLIN; // 轮询

    while(!global_exit) {
        if (cfg->xsk_poll_mode) {
            ret = poll(fds, nfds, -1);
            if (ret <= 0 || ret > 1)
                continue;
        }
        handle_receive_packets(xsk_socket, xsk_socket_tx);
    }
}

#define NANOSEC_PER_SEC 1000000000 /* 10^9 */
static uint64_t gettime(void)
{
    struct timespec t;
    int res;

    res = clock_gettime(CLOCK_MONOTONIC, &t);
    if (res < 0) {
        fprintf(stderr, "Error with gettimeofday! (%i)\n", res);
        exit(EXIT_FAIL);
    }
    return (uint64_t) t.tv_sec * NANOSEC_PER_SEC + t.tv_nsec;
}

static double calc_period(struct stats_record *r, struct stats_record *p)
{
    double period_ = 0;
    __u64 period = 0;

    period = r->timestamp - p->timestamp;
    if (period > 0)
        period_ = ((double) period / NANOSEC_PER_SEC);

    return period_;
}

static void stats_print(struct stats_record *stats_rec,
                        struct stats_record *stats_prev)
{
    uint64_t packets, bytes;
    double period;
    double pps; /* packets per sec */
    double bps; /* bits per sec */

    char *fmt = "%-12s %'11lld pkts (%'10.0f pps)"
                " %'11lld Kbytes (%'6.0f Mbits/s)"
                " period:%f\n";

    period = calc_period(stats_rec, stats_prev);
    if (period == 0)
        period = 1;

    packets = stats_rec->rx_packets - stats_prev->rx_packets;
    pps     = packets / period;

    bytes   = stats_rec->rx_bytes   - stats_prev->rx_bytes;
    bps     = (bytes * 8) / period / 1000000;

    printf(fmt, "AF_XDP RX:", stats_rec->rx_packets, pps,
           stats_rec->rx_bytes / 1000 , bps,
           period);

    packets = stats_rec->tx_packets - stats_prev->tx_packets;
    pps     = packets / period;

    bytes   = stats_rec->tx_bytes   - stats_prev->tx_bytes;
    bps     = (bytes * 8) / period / 1000000;

    printf(fmt, "       TX:", stats_rec->tx_packets, pps,
           stats_rec->tx_bytes / 1000 , bps,
           period);

    printf("\n");
}

static void *stats_poll(void *arg)
{
    unsigned int interval = 2;
    struct xsk_socket_info *xsk = arg;
    static struct stats_record previous_stats = { 0 };

    previous_stats.timestamp = gettime();

    /* Trick to pretty printf with thousands separators use %' */
    setlocale(LC_NUMERIC, "en_US");

    while (!global_exit) {
        sleep(interval);
        xsk->stats.timestamp = gettime();
        stats_print(&xsk->stats, &previous_stats);
        previous_stats = xsk->stats;
    }
    return NULL;
}

static void exit_application(int signal)
{
    int err;

    cfg.unload_all = true;
    err = do_unload(&cfg);
    if (err) {
        fprintf(stderr, "Couldn't detach XDP program on iface '%s' : (%d)\n",
                cfg.ifname, err);
    }

    signal = signal;
    global_exit = true;
}

int main(int argc, char **argv)
{
    int ret;
    void *packet_buffer, *packet_buffer_tx;
    uint64_t packet_buffer_size;
    DECLARE_LIBBPF_OPTS(bpf_object_open_opts, opts);
    DECLARE_LIBXDP_OPTS(xdp_program_opts, xdp_opts, 0);
    struct rlimit rlim = {RLIM_INFINITY, RLIM_INFINITY};
    struct xsk_umem_info *umem, *umem_tx;
    struct xsk_socket_info *xsk_socket, *xsk_socket_tx;
    struct bpf_object *obj;
    const char *map_name = "xdp_ip_map"; // 根据实际 map 名称更改
    const char *pin_path = "/sys/fs/bpf/xdp_ip_map"; // 根据实际情况更改挂载路径
    struct stat st = {0};
    pthread_t stats_poll_thread;
    int err;
    char errmsg[1024];

    /* Global shutdown handler */
    signal(SIGINT, exit_application);  // 按下ctrl +c 时执行exit_application，退出程序

    /* Cmdline options can change progname */
    parse_cmdline_args(argc, argv, long_options, &cfg, __doc__);

    /* Required option */
    if (cfg.ifindex == -1) {
        fprintf(stderr, "ERROR: Required option --dev missing\n\n");
        usage(argv[0], __doc__, long_options, (argc == 1));
        return EXIT_FAIL_OPTION;
    }

    if (cfg.redirect_ifindex  == -1) {
        fprintf(stderr, "ERROR: Required option -r missing\n\n");
        usage(argv[0], __doc__, long_options, (argc == 1));
        return EXIT_FAIL_OPTION;
    }
    cfg_tx.ifname = cfg.redirect_ifname;
    cfg_tx.ifindex = cfg.redirect_ifindex ;


    /* Load custom program if configured */
    if (cfg.filename[0] != 0) {
        struct bpf_map *map;

        //  使用filename和progname来识别我们想要从 ELF 文件创建的 XDP 程序
        custom_xsk = true;
        xdp_opts.open_filename = cfg.filename;
        xdp_opts.prog_name = cfg.progname;
        xdp_opts.opts = &opts;

        if (cfg.progname[0] != 0) {
            xdp_opts.open_filename = cfg.filename;
            xdp_opts.prog_name = cfg.progname;
            xdp_opts.opts = &opts;

            prog = xdp_program__create(&xdp_opts);
        } else {
            prog = xdp_program__open_file(cfg.filename,
                                          NULL, &opts);
        }
        err = libxdp_get_error(prog);
        if (err) {
            libxdp_strerror(err, errmsg, sizeof(errmsg));
            fprintf(stderr, "ERR: loading program: %s\n", errmsg);
            return err;
        }

        err = xdp_program__attach(prog, cfg.ifindex, cfg.attach_mode, 0);
        if (err) {
            libxdp_strerror(err, errmsg, sizeof(errmsg));
            fprintf(stderr, "Couldn't attach XDP program on iface '%s' : %s (%d)\n",
                    cfg.ifname, errmsg, err);
            return err;
        }

        /* We also need to load the xsks_map */
//        xdp_program__bpf_obj 的作用是从一个 xdp_program 结构体中获取关联的 bpf_object
        map = bpf_object__find_map_by_name(xdp_program__bpf_obj(prog), "xsks_map");
//        找到map关联的描述符fd
        xsk_map_fd = bpf_map__fd(map);
        if (xsk_map_fd < 0) {
            fprintf(stderr, "ERROR: no xsks map found: %s\n",
                    strerror(xsk_map_fd));
            exit(EXIT_FAILURE);
        }
    }

    //先检查 map 是否已被挂载，如果已挂载则不尝试重新挂载，而是直接使用已存在的 map
    // 打开 BPF ELF 文件
    obj = bpf_object__open("af_xdp_kern.o");
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }

    // 加载 BPF 程序
    ret = bpf_object__load(obj);
    if (ret) {
        fprintf(stderr, "Failed to load BPF program\n");
        return 1;
    }

    // 检查 map 是否已经被挂载
    if (stat(pin_path, &st) == -1) {
        // 指定的 pin_path 不存在，进行挂载
        struct bpf_map *map1= bpf_object__find_map_by_name(obj, map_name);
        if (!map1) {
            fprintf(stderr, "Failed to find %s\n", map_name);
            return 1;
        }

        ret = bpf_object__pin_maps(obj, "/sys/fs/bpf");
        if (ret) {
            fprintf(stderr, "Failed to pin map: %s\n", strerror(-ret));
            return 1;
        }

        printf("Map %s pinned successfully at %s.\n", map_name, pin_path);
    } else {
        printf("Map %s already pinned at %s, skipping pinning.\n", map_name, pin_path);
    }

    /* Allow unlimited locking of memory, so all memory needed for packet
     * buffers can be locked.
     */
    if (setrlimit(RLIMIT_MEMLOCK, &rlim)) {
        fprintf(stderr, "ERROR: setrlimit(RLIMIT_MEMLOCK) \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Allocate memory for NUM_FRAMES of the default XDP frame size */
    packet_buffer_size = NUM_FRAMES * FRAME_SIZE;
    if (posix_memalign(&packet_buffer,
                       getpagesize(), /* PAGE_SIZE aligned */
                       packet_buffer_size)) {
        fprintf(stderr, "ERROR: Can't allocate buffer memory \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&packet_buffer_tx,
                       getpagesize(), /* PAGE_SIZE aligned */
                       packet_buffer_size)) {
        fprintf(stderr, "ERROR: Can't allocate buffer_tx memory \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Initialize shared packet_buffer for umem usage */
    umem = configure_xsk_umem(packet_buffer, packet_buffer_size);
    if (umem == NULL) {
        fprintf(stderr, "ERROR: Can't create umem \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    umem_tx = configure_xsk_umem(packet_buffer_tx, packet_buffer_size);
    if (umem == NULL) {
        fprintf(stderr, "ERROR: Can't create umem \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }



    /* Open and configure the AF_XDP (xsk) socket */
    xsk_socket_tx = xsk_configure_socket(&cfg_tx, umem_tx);
    if (xsk_socket_tx == NULL) {
        fprintf(stderr, "ERROR: Can't setup AF_XDP socket_tx \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    xsk_socket = xsk_configure_socket(&cfg, umem);
    if (xsk_socket == NULL) {
        fprintf(stderr, "ERROR: Can't setup AF_XDP socket \"%s\"\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Start thread to do statistics display */
    if (verbose) {
        ret = pthread_create(&stats_poll_thread, NULL, stats_poll,
                             xsk_socket);
        if (ret) {
            fprintf(stderr, "ERROR: Failed creating statistics thread "
                            "\"%s\"\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /* Receive and count packets than drop them */
    rx_and_process(&cfg, xsk_socket, xsk_socket_tx);

    /* Cleanup */
    xsk_socket__delete(xsk_socket->xsk);
    xsk_socket__delete(xsk_socket_tx->xsk);
    xsk_umem__delete(umem->umem);
    xsk_umem__delete(umem_tx->umem);


    return EXIT_OK;
}