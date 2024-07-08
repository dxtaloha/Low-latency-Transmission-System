#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    int map_fd, key = 0, err;
    __u32 ip_addr;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip-address>\n", argv[0]);
        return 1;
    }

    ip_addr = inet_addr(argv[1]);  // Convert IP address from string to binary

    // Open the existing map by pinning path
    map_fd = bpf_obj_get("/sys/fs/bpf/xdp_ip_map");
    if (map_fd < 0) {
        fprintf(stderr, "Failed to open the map: %d\n", map_fd);
        return 1;
    }

    // Update the map with the provided IP address
    err = bpf_map_update_elem(map_fd, &key, &ip_addr, BPF_ANY);
    if (err) {
        fprintf(stderr, "Failed to update the map: %d\n", err);
        return 1;
    }

    printf("IP address %s successfully updated in the map\n", argv[1]);
    close(map_fd);
    return 0;
}
