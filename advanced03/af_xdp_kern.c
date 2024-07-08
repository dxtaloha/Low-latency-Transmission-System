#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/types.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, __u32);
    __type(value, __u32);
    __uint(max_entries, 64);
    __uint(pinning, LIBBPF_PIN_BY_NAME);
} xdp_ip_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __type(key, __u32);
    __type(value, __u32);
    __uint(max_entries, 64);
} xsks_map SEC(".maps");

#define bpf_ntohs(x) __builtin_bswap16(x)
#define bpf_ntohl(x) __builtin_bswap32(x)

// 检查指针是否在数据包的有效范围内
#define CHECK_PTR(ptr) if ((void *)(ptr) + 1 > data_end) return XDP_DROP;

SEC("xdp")
int xdp_sock_prog(struct xdp_md *ctx) {
    int index = ctx->rx_queue_index;
    __u32 key = 0;
    __u32 *ip_addr;

    ip_addr = bpf_map_lookup_elem(&xdp_ip_map, &key);
    if (ip_addr) {
        void *data_end = (void *)(long)ctx->data_end;
        void *data = (void *)(long)ctx->data;
        struct ethhdr *eth = data;

        // 检查以太网头部是否在数据包范围内
        if ((void *)(eth + 1) > data_end) {
            return XDP_DROP;
        }

        struct iphdr *ip = (struct iphdr *)(eth + 1);

        // 检查IP头部是否在数据包范围内
        if ((void *)(ip + 1) > data_end) {
            return XDP_DROP;
        }

        // 检查是否为IPv4包
        if (ip->version != 4) {
            return XDP_PASS;
        }

        // 检查IP头部长度是否正确
        if ((void *)((__u8 *)ip + ip->ihl * 4) > data_end) {
            return XDP_DROP;
        }

        if (ip->saddr == *ip_addr) {
            if (ip->protocol == IPPROTO_TCP) {
                struct tcphdr *tcp = (struct tcphdr *)((__u8 *)ip + ip->ihl * 4);

                // 检查TCP头部是否在数据包范围内
                if ((void *)(tcp + 1) > data_end) {
                    return XDP_DROP;
                }

                // 检查是否是TCP握手或挥手包
                if (tcp->syn || tcp->fin || tcp->rst) {
                    return XDP_PASS;
                }

                // 检查是否是纯ACK包（没有数据）
                if (tcp->ack && (bpf_ntohs(ip->tot_len) - ip->ihl * 4 - tcp->doff * 4 == 0)) {
                    return XDP_PASS;
                }

                // 重定向其他TCP数据包到用户态
                return bpf_redirect_map(&xsks_map, index, 0);

            } else if (ip->protocol == IPPROTO_UDP) {
                struct udphdr *udp = (struct udphdr *)((__u8 *)ip + (ip->ihl * 4));

                // 检查UDP头部是否在数据包范围内
                if ((void *)(udp + 1) > data_end) {
                    return XDP_DROP;
                }

                // 重定向UDP数据包到用户态
                return bpf_redirect_map(&xsks_map, index, 0);
            }
        }
    }
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
