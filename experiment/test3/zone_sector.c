#include <sys/ioctl.h>
#include <linux/blkzoned.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <device>\n", argv[0]);
        return 0;
    }

    const char *dev_path = argv[1];
    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 0;
    }

    struct blk_zone_range zrange;
    zrange.sector = 0;          // 초기화할 첫 번째 존의 시작 섹터
    zrange.nr_sectors = 524288; // 초기화할 총 섹터 수 (256MiB)

    if (ioctl(fd, BLKRESETZONE, &zrange) < 0) {
        perror("BLKRESETZONE failed");
        close(fd);
        return 0;
    }

    printf("Zone reset successfully.\n");
    close(fd);
    return 0;
}

