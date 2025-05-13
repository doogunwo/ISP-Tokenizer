#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <linux/fiemap.h>
#include <sys/time.h>
#include <nlohmann/json.hpp>

extern double get_time_in_us();
extern std::string LoadJSONFromFile(const std::string& path);
extern std::string LoadTXTFromFile(const std::string& path);
extern std::vector<int32_t> token(const std::string vocab_blob, const std::string merges_blob, const std::string added_token, const std::string text);

#define LBA_SIZE 4096
#define BLOCK_COUNT 32
#define BUFFER_SIZE (BLOCK_COUNT * LBA_SIZE)
#define NVME_DEVICE "/dev/nvme0n1"
#define MODEL_PATH "../model/byte_level_bpe_model.json"
#define MERGES_PATH "../model/merges.txt"

using json = nlohmann::json;

void TokenProcess(const std::string& full_input) {
    double model_load_start = get_time_in_us();

    std::string model_json = LoadJSONFromFile(MODEL_PATH);
    if (model_json.empty()) {
        std::cerr << "[ERROR] JSON 로드 실패\n";
        return;
    }

    auto j = nlohmann::json::parse(model_json);
    std::string vocab_blob = j["model"]["vocab"].dump();
    std::string merges_blob = LoadTXTFromFile(MERGES_PATH);
    std::string added_token = R"({ "[PAD]": 0, "[UNK]": 1, "[CLS]": 2, "[SEP]": 3, "[MASK]": 4 })";

    double model_load_end = get_time_in_us();
    std::cout << "[LOG] 모델 로딩 시간: " << (model_load_end - model_load_start) << " µs\n";

    // 1. 문자열 유효 길이 추출 (SPDK-BPE와 동일하게 '\0' 기준 자르기)
    size_t real_len = strnlen(full_input.c_str(), full_input.size());
    std::string input_text = full_input.substr(0, real_len);

    std::cout << "[DEBUG] 유효 input_text 길이: " << input_text.size() << "\n";

 
    // 3. 토큰화 실행
    std::vector<int> token_ids = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids2 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids3 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids4 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids5 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids6 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids7 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids8 = token(vocab_blob, merges_blob, added_token, input_text);
    std::vector<int> token_ids9 = token(vocab_blob, merges_blob, added_token, input_text);



    std::cout << "[Host] token ids size : " << token_ids.size() << "\n";

    std::cout << "\n";
}


std::string ReadBlocksFromNVMe(uint64_t start_lba, uint32_t num_blocks) {
    double open_start = get_time_in_us();
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return "";
    }

    double open_end = get_time_in_us();
    std::cout << "[LOG] NVMe 장치 open 시간: " << (open_end - open_start) << " µs\n";

    struct nvme_passthru_cmd cmd = {};
    std::vector<uint8_t> buffer(BUFFER_SIZE, 0);

    cmd.opcode = 0x02;
    cmd.nsid = 1;
    cmd.cdw10 = static_cast<__u32>(start_lba);
    cmd.cdw12 = num_blocks - 1;
    cmd.addr = reinterpret_cast<__u64>(buffer.data());
    cmd.data_len = BUFFER_SIZE;

    std::cout << "\n[INFO] NVMe cmd (cdw10: " << start_lba << ", blocks: " << num_blocks << ")\n";

    double ioctl_start = get_time_in_us();
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    double ioctl_end = get_time_in_us();

    close(fd);

    if (ret < 0) {
        perror("[ERROR] NVMe ioctl 실패");
        return "";
    }

    std::cout << "[INFO] NVMe read time: " << (ioctl_end - ioctl_start) << " µs\n";
    return std::string(buffer.begin(), buffer.end());
}

int main(int argc, char* argv[]) {
    double total_start = get_time_in_us();

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file-path>\n";
        return 1;
    }

    double map_start = get_time_in_us();
    std::string file_path = argv[1];
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("파일 열기 실패");
        return 1;
    }

    struct fiemap* fiemap;
    struct fiemap_extent* extent;
    size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
    fiemap = static_cast<struct fiemap*>(malloc(fiemap_size));
    if (!fiemap) {
        close(fd);
        return 1;
    }

    memset(fiemap, 0, fiemap_size);
    fiemap->fm_start = 0;
    fiemap->fm_length = ~0ULL;
    fiemap->fm_extent_count = 1;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1 || fiemap->fm_mapped_extents == 0) {
        perror("FIEMAP 실패");
        free(fiemap);
        close(fd);
        return 1;
    }

    extent = &fiemap->fm_extents[0];
    uint64_t start_lba = extent->fe_physical / LBA_SIZE;

    std::cout << "Physical Offset: " << extent->fe_physical
              << ", Block Count: " << BLOCK_COUNT * 8 << "\n";

    free(fiemap);
    close(fd);
    double map_end = get_time_in_us();
    std::cout << "[LOG] FIEMAP 처리 시간: " << (map_end - map_start) << " µs\n";

    double read_start = get_time_in_us();
    std::string nvme_data = ReadBlocksFromNVMe(start_lba, BLOCK_COUNT);
    double read_end = get_time_in_us();
    std::cout << "[LOG] 전체 NVMe 읽기 구간 시간: " << (read_end - read_start) << " µs\n";

    TokenProcess(nvme_data);

    double total_end = get_time_in_us();
    std::cout << "[INFO] 전체 실행 시간: " << (total_end - total_start) << " µs\n";

    return 0;
}
