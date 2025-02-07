#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <string.h>
#include <stdint.h>
#include "tokenizers_c.h"
#include <stdio.h>
#include <ctype.h>
#include "../json.c/json.h"

#define NVME_DEV "/dev/nvme0n1"  // NVMe-oF 디바이스
#define OPCODE_READ 0x02         // NVMe READ 명령어
#define BLOCK_SIZE 512           // NVMe 블록 크기
#define MAX_IO_BLOCKS 255        // NVMe MDTS 제한 (최대 255 블록)
#define ALIGNMENT 4096           // NVMe 정렬 요구사항

// LBA 매핑 구조체
typedef struct {
    const char *path;
    uint64_t lba_start;
    uint64_t lba_count;
} LBA_Map;

// LBA 매핑 테이블
LBA_Map file_lba_map[] = {
    { "bbpe_tokenizer.json", 1000, 2932 }, // BPE 모델 (2932 블록)
    { "wiki_corpus.txt", 100, 2084 },      // 위키 코퍼스 (2084 블록)
    { "merges.txt", 1500, 891}
};

// 매핑된 파일 개수
#define FILE_MAP_COUNT (sizeof(file_lba_map) / sizeof(file_lba_map[0]))


struct json convert_json_header(char *json_data) {
    return json_parse(json_data); 
}

// LBA 매핑 찾기 함수
LBA_Map *get_lba_mapping(const char *filepath) {
    for (size_t i = 0; i < FILE_MAP_COUNT; i++) {
        if (strcmp(file_lba_map[i].path, filepath) == 0) {
            return &file_lba_map[i];
        }
    }
    return NULL;
}

// NVMe에서 데이터를 읽어 동적 메모리로 반환
int read_nvme_data(const char *filepath, char **output_buffer) {
    int fd = open(NVME_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open NVMe device");
        return -1;
    }

    LBA_Map *mapping = get_lba_mapping(filepath);
    if (!mapping) {
        fprintf(stderr, "Error: No LBA mapping found for %s\n", filepath);
        close(fd);
        return -1;
    }

    uint64_t remaining_blocks = mapping->lba_count;
    uint64_t current_lba = mapping->lba_start;
    size_t total_size = remaining_blocks * BLOCK_SIZE;

    // 버퍼 할당
    *output_buffer = (char*)malloc(total_size + 1); // NULL 종료 위해 +1
    if (!*output_buffer) {
        fprintf(stderr, "Memory allocation failed!\n");
        close(fd);
        return -1;
    }
    memset(*output_buffer, 0, total_size + 1);

    char *buffer_ptr = *output_buffer;

    while (remaining_blocks > 0) {
        uint64_t blocks_to_read = (remaining_blocks > MAX_IO_BLOCKS) ? MAX_IO_BLOCKS : remaining_blocks;
        size_t data_size = blocks_to_read * BLOCK_SIZE;

        struct nvme_user_io io = {0};
        io.opcode = OPCODE_READ;
        io.addr = (uint64_t)buffer_ptr;
        io.nblocks = blocks_to_read - 1;
        io.slba = current_lba;

        if (ioctl(fd, NVME_IOCTL_SUBMIT_IO, &io) < 0) {
            perror("Failed to read data from NVMe");
            free(*output_buffer);
            close(fd);
            return -1;
        }

        buffer_ptr += data_size;
        remaining_blocks -= blocks_to_read;
        current_lba += blocks_to_read;
    }

    printf("Successfully read %s from NVMe\n", filepath);
    close(fd);
    return 0;
}

void validate_json(const char *json_data) {
    if (json_data == NULL || strlen(json_data) == 0) {
        fprintf(stderr, "❌ JSON 데이터가 비어 있습니다!\n");
        exit(EXIT_FAILURE);
    }

    // JSON 형식 검증
    if (!json_valid(json_data)) {
        fprintf(stderr, "❌ JSON 데이터가 올바르지 않습니다!\n");
        fprintf(stderr, "JSON 내용: %s\n", json_data);  // 디버깅용 출력
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "%s\n", json_data);
    printf("✅ JSON 데이터가 유효합니다.\n");
}

// 테스트 실행
int main() {
    char *buffer_model;
    char *buffer_data;

    // NVMe에서 JSON 데이터를 읽어오기
    read_nvme_data("bbpe_tokenizer.json", &buffer_model);
    read_nvme_data("merges.json", &buffer_data);
    
    validate_json(buffer_model);
   
     
         

    return 0;
}
