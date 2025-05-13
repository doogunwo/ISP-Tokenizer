#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define LBA_SIZE 4096
#define BLOCK_COUNT 32
#define WRITE_SIZE (LBA_SIZE * BLOCK_COUNT)
#define TARGET_DEVICE "/dev/nvme0n1"
#define INPUT_FILE   "wiki.txt"
#define START_LBA    33980  // 시작 LBA

// UTF-8 안전하게 자르기: WRITE_SIZE보다 적거나 같은 범위에서 멀티바이트 경계 유지
size_t utf8_safe_cut(const unsigned char *buf, size_t len) {
    if (len == 0) return 0;
    size_t end = len;
    // 끝이 0x80~0xBF 사이(속바이트)이면 리더 찾기
    while (end > 0 && (buf[end] & 0xC0) == 0x80) {
        end--;
    }
    return end;
}

int main(void) {
    // UTF-8 로케일 설정 (넓은 함수 사용 시)
    setlocale(LC_ALL, "en_US.UTF-8");

    // 입력 파일 열기
    int in_fd = open(INPUT_FILE, O_RDONLY);
    if (in_fd < 0) {
        perror("open input file");
        return EXIT_FAILURE;
    }

    // 파일 크기 조회
    struct stat st;
    if (fstat(in_fd, &st) < 0) {
        perror("fstat");
        close(in_fd);
        return EXIT_FAILURE;
    }
    size_t file_size = st.st_size;

    // 읽을 최대 바이트 계산
    size_t max_read = (file_size < WRITE_SIZE) ? file_size : WRITE_SIZE;

    // 메모리 할당
    unsigned char *buffer = malloc(max_read);
    if (!buffer) {
        perror("malloc");
        close(in_fd);
        return EXIT_FAILURE;
    }

    // 파일에서 순차적으로 읽기
    ssize_t rd = read(in_fd, buffer, max_read);
    if (rd < 0) {
        perror("read");
        free(buffer);
        close(in_fd);
        return EXIT_FAILURE;
    }
    size_t read_bytes = rd;
    close(in_fd);

    // UTF-8 경계에 맞춰 자르기
    size_t write_bytes = utf8_safe_cut(buffer, read_bytes);
    if (write_bytes == 0) {
        fprintf(stderr, "No valid UTF-8 characters in buffer.\n");
        free(buffer);
        return EXIT_FAILURE;
    }

    // 장치 열기 (쓰기)
    int dev_fd = open(TARGET_DEVICE, O_RDWR);
    if (dev_fd < 0) {
        perror("open target device");
        free(buffer);
        return EXIT_FAILURE;
    }

    // pwrite로 LBA 오프셋만큼 이동 후 쓰기
    off_t offset = (off_t)START_LBA * LBA_SIZE;
    ssize_t wr = pwrite(dev_fd, buffer, write_bytes, offset);
    if (wr < 0) {
        perror("pwrite");
        close(dev_fd);
        free(buffer);
        return EXIT_FAILURE;
    } else if ((size_t)wr != write_bytes) {
        fprintf(stderr, "Partial write: %zd of %zu bytes\n", wr, write_bytes);
    }

    // 동기화
    if (fsync(dev_fd) < 0) {
        perror("fsync");
    }

    close(dev_fd);
    free(buffer);

    printf("Wrote %zu bytes from '%s' to %s at LBA %d successfully.\n",
           write_bytes, INPUT_FILE, TARGET_DEVICE, START_LBA);
    return EXIT_SUCCESS;
}

