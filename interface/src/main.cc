#include "../include/vfs.h"
#include "../include/nvme.h"
#include <iostream>
#include <vector>

int main() {
    try {
        // NVMe 디바이스 초기화
        NVMe nvme("/dev/nvme0n1", 512); // NVMe 디바이스 경로와 블록 크기 설정
        VFS vfs(10000, nvme);          // 총 블록 수는 10000으로 설정

        // 1. 파일 쓰기 테스트
        std::string file_name = "example.txt"; // 테스트 파일 이름
        std::vector<uint8_t> write_data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};

        std::cout << "파일 쓰기 중: " << file_name << std::endl;
        vfs.write_file(file_name, write_data);

        // 2. 파일 읽기 테스트
        std::cout << "파일 읽기 중: " << file_name << std::endl;
        std::vector<uint8_t> read_data = vfs.read_file(file_name, write_data.size());

        // 읽은 데이터 출력
        std::cout << "읽은 데이터: ";
        for (uint8_t byte : read_data) {
            std::cout << static_cast<char>(byte);
        }
        std::cout << std::endl;

        // 3. 메타데이터 출력
        vfs.print_metadata();

    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << std::endl;
    }

    return 0;
}

