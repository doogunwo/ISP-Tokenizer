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
#include <sys/stat.h>
#include <sys/resource.h>
#include <dirent.h>


extern double g_token_time_us;
extern double get_time_in_us();
extern std::string LoadJSONFromFile(const std::string& path);
extern std::string LoadTXTFromFile(const std::string& path);
extern std::vector<int32_t> token(const std::string vocab_blob,
                                  const std::string merges_blob,
                                  const std::string added_token,
                                  const std::string text);

#define LBA_SIZE      512
#define BLOCK_COUNT   32
#define BUFFER_SIZE   (BLOCK_COUNT * LBA_SIZE)
#define NVME_DEVICE   "/dev/nvme0n1"
#define MODEL_PATH    "../model/byte_level_bpe_model.json"
#define MERGES_PATH   "../model/merges.txt"
#define MAX_EXTENTS   128

using json = nlohmann::json;
double process_time = 0; 
static double global_total_start = 0.0;
double get_elapsed() {
    return get_time_in_us() - global_total_start;
}



std::vector<uint64_t> CollectLBAsFromFolder(const std::string& folder_path) {
    std::vector<uint64_t> lbas;
    DIR* dir = opendir(folder_path.c_str());
    if (!dir) { perror("opendir"); return lbas; }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) continue;

        std::string path = folder_path + "/" + entry->d_name;
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) continue;

        size_t fiemap_size = sizeof(struct fiemap) + MAX_EXTENTS * sizeof(struct fiemap_extent);
        struct fiemap* fm = (struct fiemap*)malloc(fiemap_size);
        memset(fm, 0, fiemap_size);
        fm->fm_start = 0;
        fm->fm_length = ~0ULL;
        fm->fm_extent_count = MAX_EXTENTS;

        if (ioctl(fd, FS_IOC_FIEMAP, fm) != -1 && fm->fm_mapped_extents > 0) {
            lbas.push_back(fm->fm_extents[0].fe_physical / LBA_SIZE);
        }
        free(fm);
        close(fd);
    }
    closedir(dir);
    return lbas;
}

std::string ReadBlocksToShm(uint64_t start_lba, uint32_t num_blocks) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd < 0) { perror("[ERROR] NVMe open"); return ""; }

    std::vector<uint8_t> buf(BUFFER_SIZE);
    struct nvme_passthru_cmd cmd = {};
    cmd.opcode  = 0xd4;
    cmd.nsid    = 1;
    cmd.cdw10   = static_cast<__u32>(start_lba);
    cmd.cdw12   = num_blocks - 1;
    cmd.addr    = reinterpret_cast<__u64>(buf.data());
    cmd.data_len= BUFFER_SIZE;

    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    close(fd);

    return std::string(reinterpret_cast<char*>(buf.data()), buf.size());
}

void TokenProcess(const std::string& text, const std::string& vocab, const std::string& merges, const std::string& extras) {
    //print_cpu_usage("Before BPE workload");
    std::vector<int> ids = token(vocab, merges, extras, text);
    //print_cpu_usage("After BPE workload");
    std::cout <<"[경과시간] spdk 응답 전송 완료" <<get_elapsed() << " ms\n";

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file-or-folder-path>\n";
        return 1;
    }
    struct stat path_stat;
    stat(argv[1], &path_stat);

    auto j = json::parse(LoadJSONFromFile(MODEL_PATH));
    std::string vocab = j["model"]["vocab"].dump();
    std::string merges = LoadTXTFromFile(MERGES_PATH);
    std::string extras = R"({"[PAD]":0,"[UNK]":1,"[CLS]":2,"[SEP]":3,"[MASK]":4})";

    global_total_start = get_time_in_us();

    if (S_ISDIR(path_stat.st_mode)) {
        std::vector<uint64_t> lbas = CollectLBAsFromFolder(argv[1]);
        for (uint64_t lba : lbas) {
            std::string data = ReadBlocksToShm(lba, BLOCK_COUNT);
            size_t real_len = data.find('\0');
            std::string cleaned_data = (real_len != std::string::npos) ? data.substr(0, real_len) : data;
            TokenProcess(cleaned_data, vocab, merges, extras);
        }
    } else {
        int fd = open(argv[1], O_RDONLY);
        if (fd < 0) { perror("file open"); return 1; }

        size_t fiemap_size = sizeof(struct fiemap) + MAX_EXTENTS * sizeof(struct fiemap_extent);
        struct fiemap* fm = (struct fiemap*)malloc(fiemap_size);
        memset(fm, 0, fiemap_size);
        fm->fm_start = 0;
        fm->fm_length = ~0ULL;
        fm->fm_extent_count = MAX_EXTENTS;

        if (ioctl(fd, FS_IOC_FIEMAP, fm) == -1) {
            perror("FIEMAP ioctl");
            close(fd);
            free(fm);
            return 1;
        }

        std::vector<fiemap_extent> sorted_extents(fm->fm_extents, fm->fm_extents + fm->fm_mapped_extents);
        for (const auto& ext : sorted_extents) {
            uint64_t extent_start_lba = ext.fe_physical / LBA_SIZE;
            uint64_t extent_blocks = ext.fe_length / LBA_SIZE;

            for (uint64_t j = 0; j < extent_blocks; j += BLOCK_COUNT) {
                uint64_t current_lba = extent_start_lba + j;
                uint32_t nb = ((j + BLOCK_COUNT) > extent_blocks) ? (extent_blocks - j) : BLOCK_COUNT;
                std::string data = ReadBlocksToShm(current_lba, nb);

            }
        }
        free(fm);
        close(fd);
    }

    double total_us = get_time_in_us() - global_total_start;
    std::cout << "[INFO] Total time: " << total_us << " µs (≈ " << total_us / 1e6 << " sec)\n";
    return 0;
}
