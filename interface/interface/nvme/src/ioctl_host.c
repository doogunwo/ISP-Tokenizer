#include "../../../../tokenizer_c/include/byte_level_bpe.h"
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
#include <jansson.h>  // JSON 파싱 라이브러리

#define LBA_SIZE 4096  
#define NVME_DEVICE "/dev/nvme0n1"
#define BUFFER_SIZE 4096  

#define MODEL_PATH "../../../../model/byte_level_bpe_model.json"
#define MERGES_PATH "../../../../model/merges.txt"

// ✅ 현재 시간을 마이크로초(µs) 단위로 반환하는 함수
double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec * 1e6 + tv.tv_usec);
}

// ✅ NVMe 블록 데이터를 읽어 BPE 토큰화 실행
void token_process(const char* text) {
    char* json_blob = load_json_from_file(MODEL_PATH);
    if (!json_blob) {
        return;
    }

    json_error_t error;
    json_t* root = json_loads(json_blob, 0, &error);
    if (!root) {
        fprintf(stderr, "[ERROR] JSON 파싱 실패: %s\n", error.text);
        free(json_blob);
        return;
    }

    json_t* model = json_object_get(root, "model");
    json_t* vocab = json_object_get(model, "vocab");

    char* vocab_blob = json_dumps(vocab, JSON_COMPACT);
    char* merges_blob = load_txt_from_file(MERGES_PATH);

    const char* added_token = "{ \"[PAD]\": 0, \"[UNK]\": 1, \"[CLS]\": 2, \"[SEP]\": 3, \"[MASK]\": 4 }";

    size_t out_len;
    double start_time = get_time_in_us();
    int* token_ids = tokenize(vocab_blob, strlen(vocab_blob), merges_blob, strlen(merges_blob),
                              added_token, strlen(added_token), text, strlen(text), &out_len);
    double end_time = get_time_in_us();

    if (token_ids) {
        printf("[Host] Tokenized output: ");
        for (size_t i = 0; i < out_len; i++) {
            printf("%d ", token_ids[i]);
        }
        printf("\n");

        printf("[Host] 토큰화 실행 시간: %.2f µs\n", end_time - start_time);
        free(token_ids);
    }

    json_decref(root);
    free(json_blob);
    free(merges_blob);
    free(vocab_blob);
}

// ✅ ioctl()을 사용하여 NVMe에서 블록 데이터 읽기
void execute_nvme_command(uint64_t cdw10) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return;
    }

    struct nvme_passthru_cmd cmd = {0};  
    uint8_t buffer[BUFFER_SIZE] = {0};   

    cmd.opcode = 0x02;  // ✅ 일반 NVMe Read 명령
    cmd.nsid = 1;
    cmd.cdw10 = cdw10;
    cmd.cdw11 = 0;
    cmd.data_len = BUFFER_SIZE;
    cmd.addr = (uintptr_t)buffer;
    cmd.metadata = 0;
    cmd.metadata_len = 0;
    cmd.timeout_ms = 0;
    cmd.result = 0;

    double start_time = get_time_in_us();
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    double end_time = get_time_in_us();

    if (ret < 0) {
        perror("[ERROR] NVMe ioctl 명령 실패");
        close(fd);
        return;
    }

    printf("[INFO] NVMe 블록 데이터 읽기 성공! 실행 시간: %.2f µs\n", end_time - start_time);

    // ✅ NVMe에서 읽은 데이터를 그대로 토큰화
    token_process((const char*)buffer);

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
        perror("Memory allocation failed");
        close(fd);
        return 1;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0;
    fiemap->fm_flags = 0;
    fiemap->fm_extent_count = 1;
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

    uint64_t physical_offset = extent->fe_physical / LBA_SIZE;
    printf("Physical Offset: %lu\n", physical_offset);

    free(fiemap);
    close(fd);

    double total_start_time = get_time_in_us();
    execute_nvme_command(physical_offset);
    double total_end_time = get_time_in_us();

    printf("[INFO] NVMe 블록 읽기 + 호스트 토큰화 전체 실행 시간: %.2f µs\n", total_end_time - total_start_time);

    return 0;
}
