#include "../include/nvme.h"
#include "../include/vfs.h"
#include <iostream>
int main(){
  NVMe nvme("/dev/nvme0n1", 4096);
  VFS vfs(10000,nvme);

  std::vector<uint8_t> data(8192, 0x41);
  vfs.write_file("test.txt", data);
  
  Metadata meta = vfs.get_metadata("test.txt");

  uint64_t start_address = meta.start_lba * nvme.get_block_size(); // LBA -> 바이트 주소
  uint32_t data_size = meta.length * nvme.get_block_size();        // 전체 크기
  
  nvme.send_bpe_command(start_address, data_size, 500, data);
  
  std::cout << "BPE 명령 전송 완료: 파일=test.txt"
                  << ", 시작 LBA=" << meta.start_lba
                  << ", 크기=" << data_size << " 바이트" << std::endl;

  return 0;

}
