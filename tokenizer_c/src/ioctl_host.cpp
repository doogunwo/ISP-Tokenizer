#include "../include/tokenizers_cpp.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/fiemap.h>
#include <sys/time.h>

#define LBA_SIZE 4096
#define NVME_DEVICE "/dev/nvme0n1"
#define BUFFER_SIZE 4096  

using json = nlohmann::json;

// ✅ 현재 시간을 마이크로초(µs) 단위로 반환하는 함수
double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec * 1e6 + tv.tv_usec);
}

// ✅ JSON & TXT 파일 로드 함수
std::string LoadJSONFromFile(const std::string& path) {
    std::ifstream fs(path);
    return std::string((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
}

std::string LoadTXTFromFile(const std::string& path) {
    std::ifstream fs(path);
    std::stringstream buffer;
    buffer << fs.rdbuf();
    return buffer.str();
}

// ✅ BPE 토큰화 수행
void token_process(const std::string& text) {
    std::string model_path = "../model/byte_level_bpe_model.json";
    std::string merges_path = "../model/merges.txt";
    
    double model_load_start = get_time_in_us();
    std::string json_blob = LoadJSONFromFile(model_path);
    
    json j = json::parse(json_blob);
    json model = j["model"];
    std::string vocab_blob = model["vocab"].dump();
    std::string merges_blob = LoadTXTFromFile(merges_path);
    double model_load_end = get_time_in_us();
    
    std::cout << "[INFO] 모델 로드 시간: " << (model_load_end - model_load_start) << " µs" << std::endl;
    
    std::string added_token = R"({
        "[PAD]": 0,
        "[UNK]": 1,
        "[CLS]": 2,
        "[SEP]": 3,
        "[MASK]": 4
    })";
    
    double token_start_time = get_time_in_us();
    std::unique_ptr<tokenizers::Tokenizer> tokenizer = tokenizers::Tokenizer::FromBlobByteLevelBPE(vocab_blob, merges_blob, added_token);
    std::vector<int32_t> token_ids = tokenizer->Encode(text);
    double token_end_time = get_time_in_us();
    std::cout <<"[Host] token ids : " << token_ids.size() << std::endl;
    
    std::cout << "[Host] 토큰화 실행 시간: " << (token_end_time - token_start_time) << " µs" << std::endl;
}

// ✅ NVMe 블록 데이터를 읽어 BPE 토큰화 실행
void execute_nvme_command(uint64_t cdw10) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return;
    }

    struct nvme_passthru_cmd cmd = {0};  
    std::vector<uint8_t> buffer(BUFFER_SIZE, 0);   

    cmd.opcode = 0x02; // NVMe Read Command
    cmd.nsid = 1;
    cmd.cdw10 = cdw10;
    cmd.cdw11 = 0;
    cmd.data_len = BUFFER_SIZE;
    cmd.addr = reinterpret_cast<uintptr_t>(buffer.data());
    cmd.metadata = 0;
    cmd.metadata_len = 0;
    cmd.timeout_ms = 0;
    cmd.result = 0;

    double nvme_start_time = get_time_in_us();
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    double nvme_end_time = get_time_in_us();

    if (ret < 0) {
        perror("[ERROR] NVMe ioctl 명령 실패");
        close(fd);
        return;
    }

    // ✅ I/O 대역폭 계산 (MB/s)
    double execution_time_s = (nvme_end_time - nvme_start_time) / 1e6;
    double throughput = (BUFFER_SIZE / 1048576.0) / execution_time_s;  

    std::cout << "[INFO] NVMe 블록 데이터 읽기 성공! 실행 시간: " << (nvme_end_time - nvme_start_time) << " µs" << std::endl;
    std::cout << "[INFO] NVMe 블록 읽기 속도: " << throughput << " MB/s" << std::endl;

    // ✅ buffer에 실제로 담긴 데이터 크기 측정
    size_t actual_data_size = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        if (buffer[i] != 0) {
            actual_data_size++;
        }
    }
    std::cout << "[DEBUG] 실제 데이터 크기: " << actual_data_size << " bytes" << std::endl;

    // ✅ NVMe에서 읽은 데이터를 그대로 토큰화 (호스트에서 실행)
    std::string text(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
    token_process(text);

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file-path>" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];
    int fd = open(file_path.c_str(), O_RDONLY);
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
        std::cerr << "[ERROR] No extents found!" << std::endl;
        free(fiemap);
        close(fd);
        return 1;
    }

    extent = &fiemap->fm_extents[0];
    uint64_t physical_offset = extent->fe_physical / LBA_SIZE;
    std::cout << "Physical Offset: " << physical_offset << std::endl;

    free(fiemap);
    close(fd);

    double total_start_time = get_time_in_us();
    execute_nvme_command(physical_offset);
    double total_end_time = get_time_in_us();

    std::cout << "[INFO] 전체 실행 시간 (NVMe 블록 읽기 + 호스트 토큰화): " << (total_end_time - total_start_time) << " µs" << std::endl;

    return 0;
}
