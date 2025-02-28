#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/fiemap.h>

#define SECTOR_SIZE 512  // NVMe 섹터 크기
#define LBA_SIZE 4096  // NVMe LBA 크기

void execute_nvme_command(uint64_t cdw10) {
    char command[512];

    snprintf(command, sizeof(command),
             "sudo nvme io-passthru /dev/nvme0n1 "
             "--opcode=0xD4 "
             "--namespace-id=1 "
             "--data-len=4096 "
             "--cdw10=%lu "
             "--cdw11=0 " // 항상 블록 1개만 읽기
             "--read "
             ,
              cdw10);

    printf("Executing command: \n%s\n", command);
    system(command);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file-path>\n", argv[0]);
        return 1;
    }

    const char *file_path = argv[1];
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    struct fiemap *fiemap;
    struct fiemap_extent *extent;
    size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
    fiemap = (struct fiemap *)malloc(fiemap_size);
    if (!fiemap) {
        perror("Memory allocation failed");
        close(fd);
        return 1;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0; // 전체 파일 크기
    fiemap->fm_flags = 0;
    fiemap->fm_extent_count = 1; // 하나의 extent만 확인
    fiemap->fm_mapped_extents = 0;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1) {
        perror("FIEMAP ioctl failed");
        free(fiemap);
        close(fd);
        return 1;
    }

    if (fiemap->fm_mapped_extents == 0) {
        fprintf(stderr, "No extents found!\n");
        free(fiemap);
        close(fd);
        return 1;
    }

    extent = &fiemap->fm_extents[0];

    // **LBA 값 계산**
    uint64_t physical_offset = extent->fe_physical / LBA_SIZE;

    printf("Physical Offset: %lu\n", physical_offset);

    free(fiemap);
    close(fd);

    // **NVMe 명령 실행 (블록 1개만 읽도록 설정)**
    execute_nvme_command(physical_offset);

    return 0;
}

