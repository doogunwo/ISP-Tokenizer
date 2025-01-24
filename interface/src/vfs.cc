#include "../include/vfs.h"
#include <iostream>
#include <stdexcept>

VFS::VFS(uint64_t total_blocks, NVMe& nvme_interface)
    : total_blocks(total_blocks), next_free_lba(0), nvme(nvme_interface) {}

void VFS::write_file(const std::string& file_name, const std::vector<uint8_t>& data) {
    if (file_map.find(file_name) != file_map.end()) {
        throw std::runtime_error("파일이 이미 존재합니다: " + file_name);
    }

    uint64_t blocks_needed = (data.size() + nvme.get_block_size() - 1) / nvme.get_block_size();
    if (next_free_lba + blocks_needed > total_blocks) {
        throw std::runtime_error("디스크 공간 부족");
    }

    Metadata meta(file_name, next_free_lba, blocks_needed);
    file_map[file_name] = meta;

    nvme.write_lba(meta.start_lba, data);
    next_free_lba += blocks_needed;

    std::cout << "파일 쓰기 완료: " << file_name
              << " (LBA 시작: " << meta.start_lba
              << ", 길이: " << meta.length << " 블록)" << std::endl;
}

std::vector<uint8_t> VFS::read_file(const std::string& file_name, size_t size) {
    auto it = file_map.find(file_name);
    if (it == file_map.end()) {
        throw std::runtime_error("파일을 찾을 수 없습니다: " + file_name);
    }

    Metadata meta = it->second;
    if (size > meta.length * nvme.get_block_size()) {
        throw std::runtime_error("읽기 크기가 파일 크기를 초과합니다");
    }

    return nvme.read_lba(meta.start_lba, size);
}

void VFS::delete_file(const std::string& file_name) {
    if (file_map.erase(file_name) == 0) {
        throw std::runtime_error("파일을 찾을 수 없습니다: " + file_name);
    }
    std::cout << "파일 삭제 완료: " << file_name << std::endl;
}

void VFS::print_metadata() const {
    std::cout << "=== 파일 메타데이터 ===" << std::endl;
    for (const auto& [file_name, meta] : file_map) {
        std::cout << "파일: " << meta.file_name
                  << ", LBA 시작: " << meta.start_lba
                  << ", 길이: " << meta.length << " 블록" << std::endl;
    }
}

Metadata VFS::get_metadata(const std::string& file_name) const {
    auto it = file_map.find(file_name);
    if (it == file_map.end()) {
        throw std::runtime_error("파일 메타데이터를 찾을 수 없습니다: " + file_name);
    }
    return it->second;
}

