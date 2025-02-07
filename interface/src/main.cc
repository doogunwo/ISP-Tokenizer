#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include "../tokenizer/tokenizer/bpe.h"
#include <fstream>  
// NVMe 디바이스 경로
#define NVME_DEV "/dev/nvme0n1"
#define BLOCK_SIZE 512
#define MAX_IO_BLOCKS 255
// LBA 매핑 구조체
typedef struct {
    const char *path;
    uint64_t lba_start;
    uint64_t lba_count;
} LBA_Map;

RE2 re(
    "('s|'t|'re|'ve|'m|'ll|'d| ?\\p{L}+| ?\\p{N}+| "
    "?[^\\s\\p{L}\\p{N}]+|\\s+\\(?!\\S\\)|\\s+)");

// LBA 매핑 테이블 (파일별 LBA 정보)
LBA_Map file_lba_map[] = {
    { "wiki_corpus.txt", 100, 2084 },      
};

#define FILE_MAP_COUNT (sizeof(file_lba_map) / sizeof(file_lba_map[0]))
// LBA 매핑 정보 가져오기
LBA_Map *get_lba_mapping(const char *filepath) {
    for (size_t i = 0; i < FILE_MAP_COUNT; i++) {
        if (strcmp(file_lba_map[i].path, filepath) == 0) {
            return &file_lba_map[i];
        }
    }
    return NULL;
}

// NVMe에서 LBA 데이터를 읽어 벡터로 반환하는 함수
std::vector<char> read_nvme_data(const char *filepath) {
    int fd = open(NVME_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open NVMe device");
        return {};
    }

    LBA_Map *mapping = get_lba_mapping(filepath);
    if (!mapping) {
        fprintf(stderr, "Error: No LBA mapping found for %s\n", filepath);
        close(fd);
        return {};
    }

    uint64_t remaining_blocks = mapping->lba_count;
    uint64_t current_lba = mapping->lba_start;
    size_t total_size = remaining_blocks * BLOCK_SIZE;

    std::vector<char> buffer(total_size, 0);  // 벡터 할당 후 0으로 초기화
    char *buffer_ptr = buffer.data();

    while (remaining_blocks > 0) {
        uint64_t blocks_to_read = (remaining_blocks > MAX_IO_BLOCKS) ? MAX_IO_BLOCKS : remaining_blocks;
        size_t data_size = blocks_to_read * BLOCK_SIZE;

        struct nvme_user_io io = {0};
        io.opcode = 0x02;  // NVMe READ 명령어
        io.addr = (uint64_t)buffer_ptr;
        io.nblocks = blocks_to_read - 1;
        io.slba = current_lba;

        if (ioctl(fd, NVME_IOCTL_SUBMIT_IO, &io) < 0) {
            perror("Failed to read data from NVMe");
            close(fd);
            return {};
        }

        buffer_ptr += data_size;
        remaining_blocks -= blocks_to_read;
        current_lba += blocks_to_read;
    }

    printf("✅ Successfully read %s from NVMe (Size: %lu bytes)\n", filepath, total_size);
    close(fd);
    return buffer;
}
int main() {
    // 1. NVMe에서 wiki_corpus.txt 읽기
    
    std::vector<char> wiki_data = read_nvme_data("wiki_corpus.txt");
    if (wiki_data.empty()) {
        fprintf(stderr, "❌ Failed to load wiki_corpus.txt from NVMe\n");
        return 1;
    }
    BPERanks bpe_ranks;
    std::fstream merges("./assets/merges.txt", std::ios::in);
    load_merge_rules(merges, &bpe_ranks);

    std::unordered_map<std::string, int> t2i;
    std::unordered_map<int, std::string> i2t;
    std::fstream vocab_file("./assets/vocab.txt", std::ios::in);
    if (!vocab_file) {
        std::cerr << "❌ Failed to open /asset/vocab.txt\n";
        return 1;
    }
    load_vocab(vocab_file, &t2i, &i2t);

    // 3. Unicode 변환 테이블 생성
    std::unordered_map<uint8_t, wchar_t> b2u;
    std::unordered_map<wchar_t, uint8_t> u2b;
    bytes_to_unicode(&b2u, &u2b);

    // 4. 토크나이징 및 인코딩
    std::vector<std::string> tokens;
    std::string current_text;
    for (char c : wiki_data) {
        if (c == '\n' || c == '\0') {  // 개행 문자나 널 문자가 나오면 한 문장을 처리
            if (!current_text.empty()) {
                tokenize(current_text, re, bpe_ranks, b2u, &tokens);
                current_text.clear();
            }
        } else {
            current_text += c;
        }
    }
    std::cout << "✅ Tokenized:\n";
    for (const auto& token : tokens) {
        std::cout << token << " ";
    }
    std::cout << std::endl;
}
