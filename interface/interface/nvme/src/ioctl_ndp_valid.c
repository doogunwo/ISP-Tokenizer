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

#include <stdint.h>
#include <stdio.h>

int is_valid_utf16le(const uint16_t *data, size_t len) {
    size_t i = 0;

    while (i < len) {
        uint16_t word = data[i];

        if (word >= 0xD800 && word <= 0xDBFF) {
            // High surrogate: expect next to be low surrogate
            if (i + 1 >= len) return i;
            uint16_t next = data[i + 1];
            if (next < 0xDC00 || next > 0xDFFF) return i;
            i += 2;  // valid surrogate pair
        } else if (word >= 0xDC00 && word <= 0xDFFF) {
            // Low surrogate without preceding high surrogate
            return i;
        } else {
            i += 1;
        }
    }

    return -1; // valid
}

int is_valid_utf8(const uint8_t *data, size_t len) {
    size_t i = 0;

    while (i < len) {
        uint8_t byte = data[i];
        size_t remaining = len - i;

        if (byte <= 0x7F) {
            // ASCII: 0xxxxxxx
            i += 1;
        } else if ((byte >> 5) == 0x6 && remaining >= 2) {
            // 2-byte: 110xxxxx 10xxxxxx
            if ((data[i+1] >> 6) != 0x2)
                return i;
            i += 2;
        } else if ((byte >> 4) == 0xE && remaining >= 3) {
            // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
            if ((data[i+1] >> 6) != 0x2 || (data[i+2] >> 6) != 0x2)
                return i;
            i += 3;
        } else if ((byte >> 3) == 0x1E && remaining >= 4) {
            // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            if ((data[i+1] >> 6) != 0x2 || (data[i+2] >> 6) != 0x2 || (data[i+3] >> 6) != 0x2)
                return i;
            i += 4;
        } else {
            // Invalid UTF-8 header byte
            return i;
        }
    }

    return -1; // valid
}



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

    cmd.opcode = 0x02;  // Vendor-Specific Opcode
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
    int err16 = is_valid_utf16le(buffer, BUFFER_SIZE);
    if (err16 == -1) {
        printf("✅ UTF-16 검증 통과!\n");
    } else {
        printf("❌ UTF-16 오류 발생! 오류 바이트 오프셋");
    }

    int err8 = is_valid_utf8((uint8_t *)buffer, BUFFER_SIZE * sizeof(uint16_t));
    if (err8 == -1) {
        printf("✅ UTF-8 검증 통과!\n");
    } else {
        printf("❌ UTF-8 오류 발생! 오류 바이트 오프셋: %d (0x%02x)\n", err8, ((uint8_t *)buffer)[err8]);
    }


    printf("[DUMP] 버퍼 내용 (최대 512 bytes):\n");
    size_t dump_len = BUFFER_SIZE > 512 ? 512 : BUFFER_SIZE;

    for (size_t i = 0; i < dump_len; i += 16) {
        printf("%08lx  ", i);
        for (int j = 0; j < 16 && (i + j) < dump_len; ++j) {
            printf("%02x ", buffer[i + j]);
        }
        printf(" ");
        for (int j = 0; j < 16 && (i + j) < dump_len; ++j) {
            char c = buffer[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("\n");
    }


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
        close(fd);
        return 1;
    }

    fiemap->fm_start = 0;
    fiemap->fm_length = ~0;
    fiemap->fm_extent_count = 1;

    if (ioctl(fd, FS_IOC_FIEMAP, fiemap) == -1 || fiemap->fm_mapped_extents == 0) {
        perror("FIEMAP failed");
        free(fiemap);
        close(fd);
        return 1;
    }

    extent = &fiemap->fm_extents[0];
    uint64_t start_lba = extent->fe_physical / LBA_SIZE;
    printf("Physical Offset: %lu, Block Count: %u\n", start_lba, BLOCK_COUNT * 8);

    free(fiemap);
    close(fd);
    int lba0 = 59872;
    int blocks = 32;
    execute_nvme_command(lba0, blocks);

    return 0;
}
