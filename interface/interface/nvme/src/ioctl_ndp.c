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
#include <sys/stat.h>

#define LBA_SIZE 4096
#define BLOCK_COUNT 32
#define BUFFER_SIZE (BLOCK_COUNT * LBA_SIZE) 
#define NVME_DEVICE "/dev/nvme0n1"

#define MAX_EXTENTS 32


//  현재 시간을 마이크로초(µs) 단위로 반환하는 함수
double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec * 1e6 + tv.tv_usec);
}

//  ioctl()을 사용하여 NVMe 명령 직접 실행 
void execute_nvme_command(uint64_t start_lba, uint32_t num_blocks) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return;
    }

    struct nvme_passthru_cmd cmd = {0};
    uint16_t buffer[BUFFER_SIZE] = {0};

    cmd.opcode = 0xD4;  // Vendor-Specific Opcode
    cmd.nsid = 1;
    cmd.cdw10 = start_lba;   // 시작 LBA
    cmd.cdw11 = 0;
    cmd.cdw12 = num_blocks-1;  // 전송할 블록 수 - 1
    cmd.cdw13 = 1;  // 전송할 블록 수 - 1

    cmd.data_len = BUFFER_SIZE;
    cmd.addr = (__u64)(uintptr_t)buffer;
    cmd.timeout_ms = 0;
    cmd.result = 0;

    printf("\n[INFO] NVMe cmd (cdw10: %lu, blocks: %u)\n", start_lba, num_blocks);

    double nvme_start_time = get_time_in_us();
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    double nvme_end_time = get_time_in_us();

    if (ret < 0) {
        perror("[ERROR] NVMe ioctl 명령 실패");
        close(fd);
        return;
    }

    printf("[INFO] 실행 시간: %.2f µs", nvme_end_time - nvme_start_time);

    size_t actual_data_size = 0;
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] != 0) actual_data_size++;
    }


    close(fd);
}

void generate_and_execute_lba_map(const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("[ERROR] 파일 열기 실패");
        return;
    }

    size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent) * MAX_EXTENTS;
    struct fiemap *fiemap = malloc(fiemap_size);
    memset(fiemap, 0, fiemap_size);
    fiemap->fm_start = 0;
    fiemap->fm_length = ~0ULL;
    fiemap->fm_extent_count = MAX_EXTENTS;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0 || fiemap->fm_mapped_extents == 0) {
        perror("[ERROR] FIEMAP ioctl 실패");
        free(fiemap);
        close(fd);
        return;
    }

    // LBA 목록을 저장할 동적 배열
    uint64_t *lba_map = NULL;
    size_t lba_count = 0;

    for (int i = 0; i < fiemap->fm_mapped_extents; i++) {
        struct fiemap_extent *ex = &fiemap->fm_extents[i];

        uint64_t start_lba = ex->fe_physical / LBA_SIZE;
        uint64_t block_count = ex->fe_length / LBA_SIZE;
        uint64_t end_lba = start_lba + block_count;

        for (uint64_t lba = start_lba; lba < end_lba; lba += BLOCK_COUNT) {
            // 마지막 요청은 BLOCK_COUNT보다 작을 수 있음
            uint32_t blocks = BLOCK_COUNT;
            if (lba + BLOCK_COUNT > end_lba) {
                blocks = end_lba - lba;
            }

            // 동적 확장
            lba_map = realloc(lba_map, sizeof(uint64_t) * (lba_count + 1));
            lba_map[lba_count++] = lba;
        }
    }

    close(fd);
    free(fiemap);

    // 실제 ioctl 실행
    for (size_t i = 0; i < lba_count; i++) {
        execute_nvme_command(lba_map[i], BLOCK_COUNT);
    }

    free(lba_map);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    generate_and_execute_lba_map(argv[1]);
    return 0;
}