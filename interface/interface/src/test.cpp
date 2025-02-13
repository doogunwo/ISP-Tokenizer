#include "nvme.h"

int main() {
    // fs_map 객체를 생성하고 파일을 지정합니다.
    fs_map fileSystem("mapping.txt");

    // 새로운 매핑 추가

    // 매핑 정보 출력
    fileSystem.add_to_mapping("filepath.txt", 2000,700);
    fileSystem.add_to_mapping("filepath.txt", 2000,800);
    // 파일에 저장
    fileSystem.save();

    return 0;
}

