#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <stdint.h>

#define MAX_EXTENTS 32   // 최대 32개의 extent를 가져옴

// 논리 블록 주소(LBA) 계산 함수
static uint64_t get_lba(uint64_t physical) {
    return physical / 512;  // 보통 블록 크기는 512바이트
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <파일 경로>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("파일 열기 실패");
        return EXIT_FAILURE;
    }

    struct fiemap *fiemap;
    struct fiemap_extent *extents;

    // 메모리 할당 (동적 크기 배열 사용)
    size_t fiemap_size = sizeof(struct fiemap) + MAX_EXTENTS * sizeof(struct fiemap_extent);
    fiemap = (struct fiemap *)malloc(fiemap_size);
    if (!fiemap) {
        perror("메모리 할당 실패");
        close(fd);
        return EXIT_FAILURE;
    }

    // FIEMAP 요청 초기화
    fiemap->fm_start = 0;        // 파일 시작점부터 검색
    fiemap->fm_length = ~0;      // 파일 끝까지 검색
    fiemap->fm_flags = 0;
    fiemap->fm_extent_count = MAX_EXTENTS;
    fiemap->fm_mapped_extents = 0;

    // FIEMAP IOCTL 호출
    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
        perror("FIEMAP 요청 실패");
        free(fiemap);
        close(fd);
        return EXIT_FAILURE;
    }

    // Extent 리스트 출력
    extents = fiemap->fm_extents;
    printf("파일: %s\n", filename);
    printf("Extent 개수: %u\n", fiemap->fm_mapped_extents);
    printf("논리 블록 주소 (LBA):\n");

    for (uint32_t i = 0; i < fiemap->fm_mapped_extents; i++) {
        uint64_t lba = get_lba(extents[i].fe_physical);
        printf("Extent %u: 논리 블록 %llu (물리 주소 %llu, 길이 %llu)\n",
               i, (unsigned long long)lba, 
               (unsigned long long)extents[i].fe_physical,
               (unsigned long long)extents[i].fe_length);
    }

    // 정리
    free(fiemap);
    close(fd);

    return EXIT_SUCCESS;
}
