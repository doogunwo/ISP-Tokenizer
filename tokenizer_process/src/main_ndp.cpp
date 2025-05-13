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
#include <iomanip>  // std::setprecision
#include "../include/shm.h"
#include <sstream>
std::ostringstream null_stream;  // 메모리 내 버퍼로 출력
extern double g_token_time_us;
extern double get_time_in_us();
extern std::string LoadJSONFromFile(const std::string& path);
extern std::string LoadTXTFromFile(const std::string& path);
extern std::vector<int32_t> token(const std::string vocab_blob,
                                  const std::string merges_blob,
                                  const std::string added_token,
                                  const std::string text);

#define LBA_SIZE      512
#define BLOCK_COUNT   256
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
    double time_before_ioctl = get_time_in_us();
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    double time_after_ioctl = get_time_in_us();
    double nvme_read_duration = time_after_ioctl - time_before_ioctl;
    printf("[DEBUG] Actual NVMe Read (ioctl) duration: %.3f µs\n", nvme_read_duration);
    
    if (ret < 0) {
        perror("ioctl NVMe read failed");
    }
    close(fd);

    return std::string(reinterpret_cast<char*>(buf.data()), buf.size());
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file-path>\n";
        return 1;
    }
    const char* file = argv[1];
    int fd = open(file, O_RDONLY);
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

    // Load model once
    std::vector<fiemap_extent> sorted_extents(fm->fm_extents, fm->fm_extents + fm->fm_mapped_extents);

    double t_start = get_time_in_us();
    
    for (const auto& ext : sorted_extents) {
        uint64_t extent_start_lba = ext.fe_physical / LBA_SIZE;
        uint64_t extent_blocks = ext.fe_length / LBA_SIZE;

        for (uint64_t j = 0; j < extent_blocks; j += BLOCK_COUNT) {
            uint64_t current_lba = extent_start_lba + j;
            uint32_t nb = ((j + BLOCK_COUNT) > extent_blocks) ? (extent_blocks - j) : BLOCK_COUNT;
            std::string data = ReadBlocksToShm(current_lba, nb);
            null_stream << data;
        }
    }


    double t_end = get_time_in_us();
    double elapsed = t_end - t_start;
    std::cout << "[INFO] Total time: " << elapsed << " µs (≈ " << elapsed / 1e6 << " sec)\n";


    free(fm);
    close(fd);
    return 0;
}
