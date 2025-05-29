// Hugepage 파일을 직접 mmap 가능한가?
// SPDK가 내부에서 관리중인 Hugepage 영역을 외부에서 파일처럼 매핑할수있는가?->불가능

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define HUGEPAGE_PATH "/mnt/huge/myhugepagefile"
#define HUGEPAGE_SIZE (2 * 1024 * 1024)  // 2MB (기본 hugepage size)

int main() {
    int fd = open(HUGEPAGE_PATH, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 파일 크기 설정
    ftruncate(fd, HUGEPAGE_SIZE);

    // hugepage mmap
    void* ptr = mmap(NULL, HUGEPAGE_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);

    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // 사용 예시
    sprintf(ptr, "Hello, hugepage!");
    printf("Wrote: %s\n", (char *)ptr);

    // 정리
    munmap(ptr, HUGEPAGE_SIZE);
    close(fd);
    return 0;
}
