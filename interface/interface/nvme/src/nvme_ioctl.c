#include "../include/nvme_ioctl.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SPDK_BPE_TOKENIZE 0xD4

void *allocate_nvme_buffer(size_t size){
  void *buffer;
  posix_memalign(&buffer, 4096, size);
  memset(buffer, 0, size);
  return buffer;
}

int bpe_tokenize(int fd, struct file_metadata *metadata){
  for(uint32_t i=0; i<metadata->num_extents; i++){
    struct nvme_passthru_cmd ioctl_cmd = {0};

    ioctl_cmd.opcode = SPDK_BPE_TOKENIZE;
    ioctl_cmd.nsid = 1;
    ioctl_cmd.cdw10 = metadata->extents[i].physical_block / 512;  
    ioctl_cmd.cdw11 = metadata->extents[i].length / 4096;  

    ioctl_cmd.data_len = 4096;
  
    ioctl_cmd.addr = (uint64_t)allocate_nvme_buffer(ioctl_cmd.data_len);
    if (!ioctl_cmd.addr) return -1;
    
    if (ioctl(fd, NVME_IOCTL_IO_CMD, &ioctl_cmd) < 0) return -1;

    free((void *)ioctl_cmd.addr);

  }
  return 0;
}
