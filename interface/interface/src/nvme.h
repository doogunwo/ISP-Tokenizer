#ifndef BPE_NVME_H
#define BPE_NVME_H

#include <cstdint>
#include <stdint.h>
#include <linux/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>

// NVMe 패스스루 명령어 구조체
struct nvme_passthru_cmd {
    uint8_t opcode;        // NVMe 명령어 코드 (ex: 0xC1 -> GET_LBA_MAPPING)
    uint8_t flags;         // 추가 플래그 (일반적으로 0)
    uint16_t rsvd1;        // 예약됨 (사용 안 함)
    uint32_t nsid;         // NVMe Namespace ID (1부터 시작)
    uint32_t cdw2;         // Command Dword 2 (사용자 정의 가능)
    uint32_t cdw3;         // Command Dword 3 (사용자 정의 가능)
    uint64_t metadata;     // 메타데이터 버퍼 주소 (필요 시 사용)
    uint64_t addr;         // 데이터 버퍼 주소 (호스트 메모리)
    uint32_t metadata_len; // 메타데이터 크기
    uint32_t data_len;     // 데이터 크기
    uint32_t cdw10;        // Command Dword 10 (사용자 정의 가능)
    uint32_t cdw11;        // Command Dword 11 (사용자 정의 가능)
    uint32_t cdw12;        // Command Dword 12 (사용자 정의 가능)
    uint32_t cdw13;        // Command Dword 13 (사용자 정의 가능)
    uint32_t cdw14;        // Command Dword 14 (사용자 정의 가능)
    uint32_t cdw15;        // Command Dword 15 (사용자 정의 가능)
    uint32_t timeout_ms;   // 타임아웃 설정 (0이면 기본값 사용)
    uint32_t result;       // NVMe 컨트롤러에서 반환하는 값
};

typedef struct {
    const char *path;
    uint64_t lba_start;
    uint64_t lba_count;
} LBA_Map;

class fs_map {
private:
  std::vector<LBA_Map> map;
  std::string mapping_file;

public:
  
  fs_map(const std::string &file): mapping_file(file){
    load(file);
  }

  void add_to_mapping(const char *path, uint64_t lba_start, uint64_t lba_count){

    bool found = false;
    for (auto &entry : map) {
            if (strcmp(entry.path, path) == 0) {
                // 중복 경로가 있을 경우 덮어쓰기
                entry.lba_start = lba_start;
                entry.lba_count = lba_count;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // 중복이 없으면 새로운 매핑 추가
            LBA_Map new_map = {path, lba_start, lba_count};
            map.push_back(new_map);
    }
  }
  
  std::pair<uint64_t, uint64_t> get_range_from_path(const char *path) const {
    for(const auto &map : map){
      if(strcmp(map.path, path)==0) return {map.lba_start, map.lba_count};
    }
    return {0,0};
  }
  
  void save(){
    std::ofstream ofs(mapping_file, std::ios::out | std::ios::app);
    if(!ofs.is_open()){
      std::cerr << "Error opening file for save!" << std::endl;
      return;
    }

    for (const auto &entry : map) {
      ofs << entry.path << " " << entry.lba_start << " " << entry.lba_count << std::endl;
    }

    ofs.close();
  }

  void load(const std::string &filename){
    std::ifstream ifs(filename); 
    std::string path;
    uint64_t lba_start, lba_count;
    while (ifs >> path >> lba_start >> lba_count) {
      add_to_mapping(path.c_str(), lba_start, lba_count);
    }
    ifs.close();
  }

};

class NVME {
private:
  int fd;

public:
  NVME(const std::string &device);
  ~NVME();
  bool isOpen() const;

};



#endif 

