#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main() {
    const char *file_path = "example.txt";
    struct stat file_stat;

    // 파일 메타데이터 가져오기
    if (stat(file_path, &file_stat) == -1) {
        perror("파일 메타데이터 확인 실패");
        return 1;
    }

    // 메타데이터 출력
    printf("파일 크기: %ld bytes\n", file_stat.st_size);
    printf("inode 번호: %ld\n", file_stat.st_ino);
    printf("블록 크기: %ld bytes\n", file_stat.st_blksize);
    printf("할당된 블록 수: %ld\n", file_stat.st_blocks);

    return 0;
}

