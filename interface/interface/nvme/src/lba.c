#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <unistd.h>
#include <stdint.h>

#define SECTOR_SIZE 512      // 보통 NVMe의 섹터 크기
#define LBA_SIZE 4096        // NVMe LBA 크기

uint64_t get_file_start_lba(const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 0;
    }

    struct fiemap *fiemap;
    struct fiemap_extent *extent;
    size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
    fiemap = (struct fiemap *)malloc(fiemap_size);
    if (!fiemap) {
        perror("Memory allocation failed");
        close(fd);
        return 0;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0; // 전체 파일 크기 조회
    fiemap->fm_flags = 0;
    fiemap->fm_extent_count = 1; // 하나의 extent만 확인
    fiemap->fm_mapped_extents = 0;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1) {
        perror("FIEMAP ioctl failed");
        free(fiemap);
        close(fd);
        return 0;
    }

    if (fiemap->fm_mapped_extents == 0) {
        fprintf(stderr, "No extents found!\n");
        free(fiemap);
        close(fd);
        return 0;
    }

    extent = &fiemap->fm_extents[0];

    // **파일의 시작 LBA 계산**
    uint64_t physical_offset = extent->fe_physical;  // 파일이 디스크에서 시작하는 물리적 위치 (바이트 단위)
    uint64_t file_start_lba = physical_offset / LBA_SIZE;  // LBA 단위 변환

    printf("File Start LBA: %lu\n", file_start_lba);

    free(fiemap);
    close(fd);

    return file_start_lba;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file-path>\n", argv[0]);
        return 1;
    }

    uint64_t file_lba = get_file_start_lba(argv[1]);
    printf("File LBA: %lu\n", file_lba);
    
    return 0;
}

