#include <fcntl.h>
#include <unistd.h>
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

// 사용자 정의 명령 전송
bool send_custom_command(int fd) {
    struct nvme_passthru_cmd cmd = {};
    cmd.opcode = 0x2;   // 사용자 정의 명령 OpCode
    cmd.nsid = 1;        // Namespace ID
    cmd.cdw10 = 0;       // 추가 데이터 없음
    cmd.cdw11 = 0;       // CDW11: 시작 주소
    cmd.cdw12 = 0;       // CDW12: 데이터 크기 (0)
    cmd.cdw13 = 0;       // 작업 유형 (사용자 정의)
    cmd.cdw14 = 0;       // 사전 크기 (0)
    cmd.data_len = 0;    // 데이터 없음
    cmd.addr = 0;        // 데이터 버퍼 없음
    cmd.timeout_ms = 2000; // 타임아웃 설정 (2초)

    if (ioctl(fd, NVME_IOCTL_IO_CMD, &cmd) < 0) {
        perror("사용자 정의 명령 전송 실패");
        return false;
    }

    std::cout << "사용자 정의 명령 전송 성공: 응답 코드 = " << cmd.result << std::endl;
    return true;
}

// Keep Alive 명령 전송
void send_keep_alive(int fd) {
    struct nvme_admin_cmd cmd = {};
    cmd.opcode = 0x18;   // Keep Alive OpCode
    cmd.nsid = 0;        // Namespace ID는 0

    if (ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd) < 0) {
        perror("Keep Alive 명령 전송 실패");
    } else {
        std::cout << "Keep Alive 명령 전송 성공" << std::endl;
    }
}

int main() {
    const char *device_path = "/dev/nvme0n1"; // NVMe-oF 디바이스 경로
    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("디바이스 열기 실패");
        return 1;
    }

    try {
        bool success = false;

        while (!success) {
            // 사용자 정의 명령 전송
            success = send_custom_command(fd);

            // Keep Alive 명령 주기적으로 전송
            for (int i = 0; i < 5; i++) { // 5초 동안 Keep Alive 전송
                send_keep_alive(fd);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    close(fd);
    return 0;
}

