// writer.c (모든 설정 하드코딩)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rte_eal.h>
#include <rte_memzone.h>

#define MEMZONE_NAME "TEXT_MZ"
#define MAX_FILE_SIZE (4 * 1024 * 1024)  // 4MB
#define FILE_NAME "text_data.txt"

int main(int argc, char **argv) {
    // EAL 초기화 인자도 하드코딩
    char *eal_args[] = {
        "./writer",
        "--proc-type=primary",
        "--file-prefix=txtshare",
        "--no-pci"
    };
    int eal_argc = sizeof(eal_args) / sizeof(eal_args[0]);

    int ret = rte_eal_init(eal_argc, eal_args);
    if (ret < 0) {
        fprintf(stderr, "EAL init failed\n");
        return -1;
    }

    FILE *fp = fopen(FILE_NAME, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    // 파일 읽기
    char *buffer = malloc(MAX_FILE_SIZE);
    size_t read_size = fread(buffer, 1, MAX_FILE_SIZE - 1, fp);
    buffer[read_size] = '\0';
    fclose(fp);

    printf("Loaded %zu bytes from %s\n", read_size, FILE_NAME);

    // 메모존 생성
    const struct rte_memzone *mz = rte_memzone_reserve(MEMZONE_NAME, MAX_FILE_SIZE, rte_socket_id(), 0);
    if (!mz) {
        fprintf(stderr, "memzone reserve failed\n");
        free(buffer);
        return -1;
    }

    memcpy(mz->addr, buffer, read_size + 1);
    printf("Data copied to shared memzone.\n");

    printf("Press Enter to exit...\n");
    getchar();

    free(buffer);
    return 0;
}

