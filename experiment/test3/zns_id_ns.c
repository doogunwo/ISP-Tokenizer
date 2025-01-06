#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>

#define NVME_NSID_ALL 0xffffffff

struct nvme_id_ns_zns {
    __u8  zoc;        // Zone Operations Capability
    __u8  ozcs;       // Optional Zone Command Support
    __u8  mar;        // Maximum Active Resources
    __u8  mor;        // Maximum Open Resources
    __u32 rrl;        // Read Resource Log Page
    __u32 frl;        // Finish Resource Log Page
    __u32 rrl1;
    __u32 rrl2;
    __u32 rrl3;
    __u32 frl1;
    __u32 frl2;
    __u32 frl3;
    __u8  lbafe[16];  // Logical Block Format Extension
};

void print_zns_id_ns(struct nvme_id_ns_zns *zns_info) {
    printf("ZNS Command Set Identify Namespace:\n");
    printf("zoc     : %u\n", zns_info->zoc);
    printf("ozcs    : %u\n", zns_info->ozcs);
    printf("mar     : 0x%x\n", zns_info->mar);
    printf("mor     : 0x%x\n", zns_info->mor);
    printf("rrl     : %u\n", zns_info->rrl);
    printf("frl     : %u\n", zns_info->frl);
    printf("rrl1    : %u\n", zns_info->rrl1);
    printf("rrl2    : %u\n", zns_info->rrl2);
    printf("rrl3    : %u\n", zns_info->rrl3);
    printf("frl1    : %u\n", zns_info->frl1);
    printf("frl2    : %u\n", zns_info->frl2);
    printf("frl3    : %u\n", zns_info->frl3);
    printf("lbafe  0: zsze:0x%x zdes:%u (in use)\n", zns_info->lbafe[0], zns_info->lbafe[1]);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <device>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dev_path = argv[1];
    int fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    struct nvme_admin_cmd cmd = {0};
    struct nvme_id_ns_zns zns_info;
    memset(&zns_info, 0, sizeof(zns_info));

    cmd.addr = (__u64)&zns_info;
    cmd.data_len = sizeof(zns_info);

    // Prepare NVMe Identify Namespace Command for ZNS
    cmd.opcode = 0x06; // Identify Command
    cmd.nsid = NVME_NSID_ALL;
    cmd.cdw10 = 0x0;   // CNS: Identify Namespace
    cmd.addr = (__u64)&zns_info;
    cmd.data_len = sizeof(zns_info);

    if (ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd) < 0) {
        perror("Identify Namespace failed");
        close(fd);
        return EXIT_FAILURE;
    }

    print_zns_id_ns(&zns_info);

    close(fd);
    return EXIT_SUCCESS;
}

