#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnvme.h>
#include <nvme/ioctl.h>

#define WRITE_BUFFER_SIZE 4096 // 쓰기 크기 (4KB)

void write_to_zns(const char *device, unsigned int nsid, __u64 zslba, const char *data) {
    nvme_root_t root;
    nvme_ctrl_t ctrl;
    nvme_ns_t ns;
    char *buffer;
    int ret;

    // NVMe 토폴로지 스캔
    root = nvme_scan(NULL);
    if (!root) {
        fprintf(stderr, "Failed to scan NVMe topology\n");
        return;
    }

    // 컨트롤러 찾기
    ctrl = nvme_ctrl_find(root, device);
    if (!ctrl) {
        fprintf(stderr, "Failed to find NVMe controller: %s\n", device);
        nvme_free_tree(root);
        return;
    }

    // 네임스페이스 열기
    ns = nvme_open(ctrl);
    if (!ns) {
        fprintf(stderr, "Failed to open NVMe namespace %u on controller %s\n", nsid, device);
        nvme_free_tree(root);
        return;
    }

    // 쓰기 버퍼 준비
    buffer = aligned_alloc(4096, WRITE_BUFFER_SIZE);
    if (!buffer) {
        perror("Failed to allocate buffer");
        nvme_ns_close(ns);
        nvme_free_tree(root);
        return;
    }
    memset(buffer, 0, WRITE_BUFFER_SIZE);
    memcpy(buffer, data, strlen(data) < WRITE_BUFFER_SIZE ? strlen(data) : WRITE_BUFFER_SIZE);

    // ZNS Append 명령어 실행
    ret = nvme_zns_append(ns, buffer, WRITE_BUFFER_SIZE, zslba, 0);
    if (ret) {
        fprintf(stderr, "Failed to append to Zone starting at LBA %llu: %s\n", zslba, strerror(-ret));
    } else {
        printf("Successfully appended to Zone at LBA %llu\n", zslba);
    }

    // 메모리 해제 및 리소스 정리
    free(buffer);
    nvme_ns_flush(ns);
    nvme_free_tree(root);
}

int main() {
    const char *device = "/dev/nvme0"; // NVMe 컨트롤러 경로
    unsigned int nsid = 1;            // 네임스페이스 ID
    __u64 zslba = 0;                  // Zone Start LBA
    const char *data = "Hello, ZNS World!"; // 쓰기 데이터

    printf("Writing to ZNS namespace...\n");
    write_to_zns(device, nsid, zslba, data);

    return 0;
}

