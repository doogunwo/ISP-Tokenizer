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
#include <sys/stat.h>  // for stat()

#define LBA_SIZE 4096
#define BLOCK_COUNT 32  // 128KB = 32 * 4KB

#define BUFFER_SIZE (BLOCK_COUNT * LBA_SIZE)
#define NVME_DEVICE "/dev/nvme0n1"
#define REQUEST_COUNT 8  // 순차적으로 처리할 요청 수

double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec * 1e6 + tv.tv_usec);
}

void execute_nvme_command(uint64_t start_lba, uint32_t num_blocks, uint32_t cdw13) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return;
    }

    struct nvme_passthru_cmd cmd = {0};
    uint16_t buffer[BUFFER_SIZE] = {0};

    cmd.opcode = 0xD4;
    cmd.nsid = 1;
    cmd.cdw10 = start_lba;
    cmd.cdw11 = 0;
    cmd.cdw12 = num_blocks - 1;
    cmd.cdw13 = cdw13;
    cmd.data_len = BUFFER_SIZE;
    cmd.addr = (__u64)(uintptr_t)buffer;

    double ioctl_start = get_time_in_us();
    if (ioctl(fd, NVME_IOCTL_IO_CMD, &cmd) < 0) {
        perror("[ERROR] NVMe ioctl 명령 전송 실패");
        close(fd);
        return;
    }
    double ioctl_end = get_time_in_us();

    // 수신된 토큰 개수 (첫 4바이트가 token count라고 가정)
    uint32_t token_count = 0;
    memcpy(&token_count, buffer, sizeof(uint32_t));
    printf("[INFO] 요청 %d → 토큰 개수: %u\n", cdw13, token_count);

    printf("[TIMER] 요청 %d ioctl 처리 시간: %.2f µs\n", cdw13, ioctl_end - ioctl_start);

    close(fd);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    const char *file_path = argv[1];
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("[ERROR] 파일 열기 실패");
        return 1;
    }

    // ✅ 전체 파일 크기 가져오기
    struct stat st;
    if (stat(file_path, &st) != 0) {
        perror("[ERROR] 파일 크기 확인 실패");
        close(fd);
        return 1;
    }
    off_t file_size = st.st_size;

    // ✅ 필요한 요청 수 계산
    uint64_t total_blocks = (file_size + LBA_SIZE - 1) / LBA_SIZE;
    uint32_t adjusted_request_count = (total_blocks + BLOCK_COUNT - 1) / BLOCK_COUNT;

    printf("[INFO] 파일 크기: %.2f MB, 전체 요청 수: %u\n",
           file_size / 1024.0 / 1024.0, adjusted_request_count);

    // FIEMAP으로 시작 LBA 계산
    struct fiemap *fiemap;
    struct fiemap_extent *extent;
    size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
    fiemap = (struct fiemap *)malloc(fiemap_size);
    if (!fiemap) {
        perror("[ERROR] 메모리 할당 실패");
        close(fd);
        return 1;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0;
    fiemap->fm_extent_count = 1;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1 || fiemap->fm_mapped_extents == 0) {
        perror("[ERROR] FIEMAP 실패");
        free(fiemap);
        close(fd);
        return 1;
    }

    extent = &fiemap->fm_extents[0];
    uint64_t start_lba = extent->fe_physical / LBA_SIZE;
    free(fiemap);
    close(fd);

    double program_start = get_time_in_us();
    printf("[INFO] 요청 %ld 시작\n", start_lba);

    // ✅ 계산된 요청 수만큼 반복
    for (int i = 0; i < 1; ++i) {
        uint64_t current_lba = start_lba + i * BLOCK_COUNT;

        
        double request_start = get_time_in_us();
        printf("[INFO] 요청 %d 시작\n", i);
        execute_nvme_command(current_lba, 31, i);
        double request_end = get_time_in_us();
        printf("[TIMER] 요청 %d 전체 처리 시간: %.2f µs\n", i, request_end - request_start);
    }

    double program_end = get_time_in_us();
    printf("[RESULT] 전체 실행 시간: %.2f µs\n", program_end - program_start);

    return 0;
}