#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NVME_DEV "/dev/nvme0n1"
#define VENDOR_OPCODE 0xd4
#define DATA_LEN 4096
#define LBA_SIZE 4096  // NVMe 블록 크기 (확인 필요)

uint64_t get_physical_offset(const char *filepath) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 0;
    }

    struct fiemap *fiemap;
    struct fiemap_extent *extent;
    size_t fiemap_size = sizeof(struct fiemap) + sizeof(struct fiemap_extent);
    fiemap = (struct fiemap *)malloc(fiemap_size);
    if (!fiemap) {
        perror("Memory allocation failed");
        close(fd);
        return 0;
    }

    memset(fiemap, 0, fiemap_size);
    fiemap->fm_start = 0;
    fiemap->fm_length = ~0; // 전체 파일 크기
    fiemap->fm_extent_count = 1; // 하나의 extent만 확인

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1) {
        perror("FIEMAP ioctl failed");
        free(fiemap);
        close(fd);
        return 0;
    }

    if (fiemap->fm_mapped_extents == 0) {
        fprintf(stderr, "No extents found!\n");
        free(fiemap);
        close(fd);
        return 0;
    }

    extent = &fiemap->fm_extents[0];
    uint64_t physical_offset = extent->fe_physical / LBA_SIZE;

    free(fiemap);
    close(fd);
    return physical_offset;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file-path>\n", argv[0]);
        return 1;
    }

    const char *file_path = argv[1];
    uint64_t physical_offset = get_physical_offset(file_path);
    if (physical_offset == 0) {
        fprintf(stderr, "Failed to get physical offset for %s\n", file_path);
        return 1;
    }

    printf("File: %s\n", file_path);
    printf("Physical Offset (LBA): %lu\n", physical_offset);

    int fd;
    struct nvme_passthru_cmd cmd;
    unsigned char data[DATA_LEN] = {0};

    fd = open(NVME_DEV, O_RDWR);
    if (fd < 0) {
        perror("Error opening NVMe device");
        return 1;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = VENDOR_OPCODE;  // 0xD4 벤더 명령
    cmd.nsid = 1;  // 네임스페이스 ID
    cmd.addr = (uintptr_t)data;  // 데이터를 받을 버퍼
    cmd.data_len = DATA_LEN;
    cmd.cdw10 = physical_offset;  // LBA Start (파일의 물리적 위치)
    cmd.cdw11 = 0;  // 블록 수 (한 블록만 읽기)
    
  // ioctl 호출 전, NVMe 명령어 정보 출력
printf("\n=== NVMe IOCTL Command Sent ===\n");
printf("Opcode     : %#x\n", cmd.opcode);
printf("Namespace  : %u\n", cmd.nsid);
printf("Data Addr  : %p\n", (void *)cmd.addr);
printf("Data Len   : %u bytes\n", cmd.data_len);
printf("CDW10      : %#x\n", cmd.cdw10);
printf("CDW11      : %#x\n", cmd.cdw11);
printf("CDW12      : %#x\n", cmd.cdw12);
printf("CDW13      : %#x\n", cmd.cdw13);
printf("CDW14      : %#x\n", cmd.cdw14);
printf("CDW15      : %#x\n", cmd.cdw15);
printf("==============================\n\n");
  // ioctl 호출
    int ret = ioctl(fd, NVME_IOCTL_IO_CMD, &cmd);
    if (ret < 0) {
        perror("NVMe ioctl failed");
        close(fd);
        return 1;
    }

    printf("NVMe Vendor Command : %#x\n",VENDOR_OPCODE);

    // 데이터 출력 (16진수로 확인)
    for (int i = 0; i < DATA_LEN/1024; i++) {
        if (i % 16 == 0) printf("\n");
        printf("%02x ", data[i]);
    }
    printf("\n");

    close(fd);
    return 0;
}

