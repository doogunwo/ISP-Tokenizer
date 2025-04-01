#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/fiemap.h>
#include <string.h>
#include <sys/time.h>
#include <jansson.h>
#include <iconv.h>

#include "../include/byte_level_bpe.h"

#define LBA_SIZE 4096
#define BLOCK_COUNT 32
#define BUFFER_SIZE (BLOCK_COUNT * LBA_SIZE)
#define NVME_DEVICE "/dev/nvme0n1"

#define MODEL_PATH "../model/byte_level_bpe_model.json"
#define MERGES_PATH "../model/merges.txt"

// ✅ 현재 시간을 마이크로초 단위로 반환
double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec * 1e6 + tv.tv_usec);
}


// ✅ BPE 토큰화 수행
void token_process(const char* text) {
    char* json_blob = load_json_from_file(MODEL_PATH);
    if (!json_blob) return;

    json_error_t error;
    json_t* root = json_loads(json_blob, 0, &error);
    if (!root) {
        free(json_blob);
        return;
    }

    json_t* model = json_object_get(root, "model");
    json_t* vocab = json_object_get(model, "vocab");

    char* vocab_blob = json_dumps(vocab, JSON_COMPACT);
    char* merges_blob = load_txt_from_file(MERGES_PATH);
    const char* added_token = "{ \"[PAD]\": 0, \"[UNK]\": 1, \"[CLS]\": 2, \"[SEP]\": 3, \"[MASK]\": 4 }";

    size_t out_len;
    //토큰화 시간 측정
    double token_start = get_time_in_us();
    int* token_ids = tokenize(vocab_blob, strlen(vocab_blob), merges_blob, strlen(merges_blob),
                              added_token, strlen(added_token), text, strlen(text), &out_len);
    double token_end = get_time_in_us();

    printf("[Host] token ids size : %zu\n", out_len);
    printf("[Host] 토큰화 시간: %.0f µs\n", token_end - token_start);

    if (token_ids) free(token_ids);
    json_decref(root);
    free(json_blob);
    free(merges_blob);
    free(vocab_blob);
}

//  NVMe에서 블록 단위 읽기 + 토큰화 수행
void execute_nvme_command(uint64_t start_lba, uint32_t num_blocks) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return;
    }
    struct nvme_passthru_cmd cmd = {0};
    uint8_t buffer[BUFFER_SIZE] = {0};

    cmd.opcode = 0x02;  // 표준 NVMe Read
    cmd.nsid = 1;
    cmd.cdw10 = start_lba;
    cmd.cdw12 = num_blocks - 1;
    cmd.data_len = BUFFER_SIZE;
    cmd.addr = (__u64)(uintptr_t)buffer;
    printf("\n[INFO] NVMe cmd (cdw10: %lu, blocks: %u)\n", start_lba, num_blocks);

    double nvme_start = get_time_in_us();
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    double nvme_end = get_time_in_us();

    if (ret < 0) {
        perror("[ERROR] NVMe ioctl 명령 실패");
        close(fd);
        return;
    }
    printf("[INFO] 실행 시간: %.2f µs\n", nvme_end - nvme_start);

    token_process((char *) buffer);
    // 변환 및 토큰화
    

    close(fd);
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
        close(fd);
        return 1;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0;
    fiemap->fm_extent_count = 1;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1 || fiemap->fm_mapped_extents == 0) {
        perror("FIEMAP failed");
        free(fiemap);
        close(fd);
        return 1;
    }

    extent = &fiemap->fm_extents[0];
    uint64_t start_lba = extent->fe_physical / LBA_SIZE;
    printf("Physical Offset: %lu, Block Count: %u\n", start_lba, BLOCK_COUNT * 8);

    free(fiemap);
    close(fd);

    double total_start = get_time_in_us();

    for (int i = 0; i < 1; ++i) {
        uint64_t current_lba = start_lba + i * BLOCK_COUNT;
        execute_nvme_command(current_lba, BLOCK_COUNT);
    }

    double total_end = get_time_in_us();
    printf("[INFO] 전체 실행 시간 (NVMe 블록 읽기 + 호스트 토큰화): %.5e µs\n", total_end - total_start);

    return 0;
}
