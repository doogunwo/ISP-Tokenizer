#include "../include/nvme.h"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>

NVMe::NVMe(const std::string& device_path, size_t block_size)
    : device_path(device_path), block_size(block_size), device_fd(-1) {
    // NVMe 디바이스 열기
    device_fd = open(device_path.c_str(), O_RDWR);
    if (device_fd < 0) {
        throw std::runtime_error("NVMe 디바이스 열기 실패: " + device_path);
    }
    std::cout << "NVMe 디바이스 초기화 완료: " << device_path << std::endl;
}

NVMe::~NVMe() {
    if (device_fd >= 0) {
        close(device_fd);
    }
}

void NVMe::write_lba(uint64_t lba, const std::vector<uint8_t>& data) {
    // LBA 오프셋 계산
    size_t offset = lba * block_size;

    // NVMe 디바이스에 데이터 쓰기
    ssize_t result = pwrite(device_fd, data.data(), data.size(), offset);
    if (result < 0) {
        throw std::runtime_error("NVMe 쓰기 실패: LBA=" + std::to_string(lba) + ", 오류=" + std::strerror(errno));
    }

    std::cout << "NVMe 쓰기 성공: LBA=" << lba << ", 크기=" << data.size() << " 바이트" << std::endl;
}

std::vector<uint8_t> NVMe::read_lba(uint64_t lba, size_t size) {
    // LBA 오프셋 계산
    size_t offset = lba * block_size;

    // 데이터 읽기
    std::vector<uint8_t> buffer(size);
    ssize_t result = pread(device_fd, buffer.data(), size, offset);
    if (result < 0) {
        throw std::runtime_error("NVMe 읽기 실패: LBA=" + std::to_string(lba) + ", 오류=" + std::strerror(errno));
    }

    std::cout << "NVMe 읽기 성공: LBA=" << lba << ", 크기=" << result << " 바이트" << std::endl;
    return buffer;
}

size_t NVMe::get_block_size() const {
    return block_size;
}

void NVMe::send_bpe_command(uint64_t start_address, uint32_t data_size, uint32_t vocab_size, const std::vector<uint8_t>& data) {
    if (data.size() != data_size) {
        throw std::runtime_error("전송 데이터 크기와 data_size가 일치하지 않습니다");
    }
    size_t block_size = get_block_size();
    uint32_t block_count = data_size / block_size;
  
    struct nvme_passthru_cmd cmd = {};
    cmd.opcode = 0xC1;       // 사용자 정의 Opcode (BPE 작업)
    cmd.nsid = 1;            // Namespace ID
    cmd.cdw11 = start_address; // 시작 주소
    cmd.cdw12 = block_count;     // 데이터 크기
    cmd.cdw13 = 0x01;            // 작업 유형 (BPE 토크나이징)
    cmd.cdw14 = vocab_size;    // 사전 크기
    cmd.data_len = data.size();
    cmd.addr = reinterpret_cast<uint64_t>(data.data());
    cmd.timeout_ms = 10000;   // 타임아웃 설정 (2초)

    // ioctl 호출: NVMe 커스텀 명령 전달
    if (ioctl(device_fd, NVME_IOCTL_IO_CMD, &cmd) < 0) {
        throw std::runtime_error("BPE 커스텀 명령 실패: ioctl 호출 오류");
    }

    std::cout << "BPE 커스텀 명령 성공: 응답 코드 = " << cmd.result << std::endl;
}

