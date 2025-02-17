#ifndef NVME_IOCTL_H
#define NVME_IOCTL_H

#include <stdint.h>
#include <linux/nvme_ioctl.h>
#include <stddef.h>
#include "fiemap_util.h"
struct nvme_custom_cmd {
    uint32_t nsid;       // NVMe Namespace ID
    uint8_t opcode;      // NVMe 커스텀 명령
    void *buffer;        // 데이터 버퍼
    size_t size;         // 버퍼 크기
    uint32_t cdw10;      // 커스텀 제어워드 10
    uint32_t cdw11;      // 커스텀 제어워드 11
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
};

int bpe_tokenize(int fd, struct file_metadata *metadata);
void *allocate_nvme_buffer(size_t size);
#endif // NVME_IOCTL_H

