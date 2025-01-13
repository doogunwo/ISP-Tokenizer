#include <stdio.h>
#include <stdlib.h>

// 파일 열기
void* nvme_open(const char* path, const char* mode) {
    FILE* file = fopen(path, mode);
    if (!file) {
        perror("파일 열기 실패");
        return NULL;
    }
    return file;
}

// 파일 읽기
size_t nvme_read(void* file, char* buffer, size_t size) {
    return fread(buffer, 1, size, (FILE*)file);
}

// 파일 닫기
void nvme_close(void* file) {
    fclose((FILE*)file);
}

