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

#define LBA_SIZE 4096  // NVMe LBA 크기
#define NVME_DEVICE "/dev/nvme0n1"
#define BUFFER_SIZE 4096  // NVMe에서 읽어올 데이터 크기

// ✅ 현재 시간을 마이크로초(µs) 단위로 반환하는 함수
double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec * 1e6 + tv.tv_usec);
}

// ✅ NVMe 응답을 해석하여 사람이 읽을 수 있도록 변환
void parse_nvme_response(uint8_t* nvme_data, size_t length) {
    if (length < sizeof(size_t)) {
        fprintf(stderr, "[ERROR] NVMe 응답 크기가 너무 작습니다! (%zu bytes)\n", length);
        return;
    }

    // 첫 번째 8바이트는 토큰 개수
    size_t token_count = *(size_t*)(nvme_data);
    int32_t* tokens = (int32_t*)(nvme_data + sizeof(size_t));

    printf("\n");
}

// ✅ ioctl()을 사용하여 NVMe 명령 직접 실행 및 I/O 대역폭 측정 추가
void execute_nvme_command(uint64_t cdw10) {
    int fd = open(NVME_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("[ERROR] NVMe 장치 열기 실패");
        return;
    }

    struct nvme_passthru_cmd cmd = {0};  // NVMe 명령 구조체 초기화
    uint8_t buffer[BUFFER_SIZE] = {0};   // NVMe 응답을 저장할 버퍼

    cmd.opcode = 0xD4;  // Vendor-Specific Opcode
    cmd.nsid = 1;       // Namespace ID
    cmd.cdw10 = cdw10;  // Logical Block Address (LBA)
    cmd.cdw11 = 0;      // 항상 블록 1개만 읽기
    cmd.data_len = BUFFER_SIZE;
    cmd.addr = (uintptr_t)buffer;  // ✅ 올바른 필드 사용
    cmd.metadata = 0;
    cmd.metadata_len = 0;
    cmd.timeout_ms = 0;
    cmd.result = 0;

    printf("[INFO] Sending NVMe command via ioctl()...\n");

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

    printf("[INFO] NVMe 블록 데이터 읽기 성공! 실행 시간: %.2f µs\n", nvme_end_time - nvme_start_time);
    printf("[INFO] NVMe 블록 읽기 속도: %.2f MB/s\n", throughput);

    // ✅ buffer에 실제로 담긴 데이터 크기 측정
    size_t actual_data_size = 0;
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] != 0) {
            actual_data_size++;
        }
    }
    printf("[DEBUG] 실제 데이터 크기: %lu bytes\n", (unsigned long)actual_data_size);
    // ✅ NVMe 응답을 사람이 읽을 수 있는 형태로 변환
    parse_nvme_response(buffer, BUFFER_SIZE);

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
    fiemap->fm_length = ~0; // 전체 파일 크기
    fiemap->fm_flags = 0;
    fiemap->fm_extent_count = 1; // 하나의 extent만 확인
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

    // **LBA 값 계산**
    uint64_t physical_offset = extent->fe_physical / LBA_SIZE;

    printf("Physical Offset: %lu\n", physical_offset);

    free(fiemap);
    close(fd);

    // **NVMe 명령 실행 후 데이터 읽기**
    double total_start_time = get_time_in_us();
    execute_nvme_command(physical_offset);
    double total_end_time = get_time_in_us();
    
    printf("[INFO] 전체 실행 시간 (NVMe 블록 읽기): %.2f µs\n", total_end_time - total_start_time);

    return 0;
}
