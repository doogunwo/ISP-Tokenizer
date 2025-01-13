#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// NVMe 명령을 실행하는 함수
int send_nvme_read_command(const char *nvme_device, off_t start_lba, size_t num_blocks) {
    int fd = open(nvme_device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open NVMe device");
        return -1;
    }

    void *buffer = malloc(4096 * num_blocks);
    if (!buffer) {
        perror("Failed to allocate memory");
        close(fd);
        return -1;
    }

    // NVMe 읽기 명령 구성
    struct nvme_passthru_cmd cmd = {
        .opcode = 0x02, // NVMe READ 명령
        .nsid = 1,      // Namespace ID
        .addr = (unsigned long)buffer, // 읽을 데이터를 저장할 버퍼
        .data_len = 4096 * num_blocks, // 읽을 데이터 크기
        .cdw10 = (start_lba & 0xFFFFFFFF), // 시작 LBA (하위 32비트)
        .cdw11 = (start_lba >> 32),       // 시작 LBA (상위 32비트)
        .cdw12 = num_blocks - 1,          // 읽을 블록 수 - 1
    };

    // NVMe 명령 실행
    if (ioctl(fd, NVME_IOCTL_IO_CMD, &cmd) < 0) {
        perror("Failed to execute NVMe read command");
        free(buffer);
        close(fd);
        return -1;
    }

    printf("데이터 읽기 완료. 첫 16바이트:\n");
    for (int i = 0; i < 16 && i < 4096 * num_blocks; i++) {
        printf("%02x ", ((unsigned char *)buffer)[i]);
    }
    printf("\n");

    free(buffer);
    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <파일 경로>\n", argv[0]);
        return 1;
    }

    const char *file_path = argv[1];
    struct stat file_stat;

    // 파일 메타데이터 가져오기
    if (stat(file_path, &file_stat) == -1) {
        perror("파일 메타데이터 확인 실패");
        return 1;
    }

    printf("파일 경로: %s\n", file_path);
    printf("파일 크기: %ld bytes\n", file_stat.st_size);
    printf("inode 번호: %ld\n", file_stat.st_ino);
    printf("블록 크기: %ld bytes\n", file_stat.st_blksize);
    printf("할당된 블록 수: %ld\n", file_stat.st_blocks);

    // 파일 시스템의 LBA 정보를 얻기 위한 가정
    off_t start_lba = file_stat.st_blocks * (file_stat.st_blksize / 512);
    size_t num_blocks = (file_stat.st_size + file_stat.st_blksize - 1) / file_stat.st_blksize;

    printf("추정 LBA 시작 주소: %ld\n", start_lba);
    printf("읽을 블록 수: %zu\n", num_blocks);

    // NVMe 디바이스 경로 (수정 필요)
    const char *nvme_device = "/dev/nvme1n1";

    // NVMe 명령 실행
    if (send_nvme_read_command(nvme_device, start_lba, num_blocks) != 0) {
        fprintf(stderr, "NVMe 명령 실행 중 오류 발생\n");
        return 1;
    }

    return 0;
}

