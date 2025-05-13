#include "../include/tokenizers_cpp.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>
#include <sys/time.h>
#include <linux/perf_event.h> // PERF_TYPE_HW_CACHE 및 관련 상수 포함
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h> // C-style string functions, e.g. strerror, or for perror
#include <cerrno>   // For errno
#include "../include/shm.h" // Presumed to be part of your project
#include <sys/resource.h>

double g_token_time_us = 0;
using json = nlohmann::json;

// Perf counter 구조체 (label 추가)
struct PerfCounter {
    int fd;
    uint64_t value;
    // perf_event_attr attr; // attr은 syscall 시점에만 필요할 수 있음
    const char* label; // 각 카운터의 레이블 저장
};

std::vector<PerfCounter> perf_counters;
double bpe_time = 0; 
// perf_event.h 에 정의된 표준 enum 값 사용 권장
// 예: PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_ACCESS 등

uint64_t make_hw_cache_config(uint64_t cache_id, uint64_t op_id, uint64_t result_id) {
    return (cache_id) | (op_id << 8) | (result_id << 16);
}
void print_cpu_usage(const std::string& label) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    std::cout <<"------------------------------------------------------------\n";
    std::cout << "[RUSAGE] " << label << "\n";
    std::cout <<"------------------------------------------------------------\n";

    std::cout << "User time: " << usage.ru_utime.tv_sec * 1000 + usage.ru_utime.tv_usec / 1000.0 << " ms\n";
    std::cout << "System time: " << usage.ru_stime.tv_sec * 1000 + usage.ru_stime.tv_usec / 1000.0 << " ms\n";
    std::cout << "Max RSS: " << usage.ru_maxrss << " KB\n";
    std::cout << "Page Faults (soft): " << usage.ru_minflt << "\n";
    std::cout << "Page Faults (hard): " << usage.ru_majflt << "\n";
    std::cout << "Voluntary Context Switches: " << usage.ru_nvcsw << "\n";
    std::cout << "Involuntary Context Switches: " << usage.ru_nivcsw << "\n";
    std::cout << "Block Input Ops: " << usage.ru_inblock << "\n";
    std::cout << "Block Output Ops: " << usage.ru_oublock << "\n";
    std::cout <<"------------------------------------------------------------\n";

}

void start_perf_counters() {
    if (!perf_counters.empty()) { // 이미 카운터가 실행 중이면 중복 실행 방지
        // std::cerr << "[WARN] Perf counters already started." << std::endl;
        // return;
        // 또는 기존 카운터들을 정리하고 새로 시작할 수 있습니다.
        // 여기서는 간단히 기존 카운터가 있다면, 비우고 새로 시작합니다.
        for (const auto& counter : perf_counters) {
            close(counter.fd); // 이전 fd가 있다면 닫아줍니다.
        }
        perf_counters.clear();
    }

    std::vector<std::tuple<perf_event_attr, const char*>> events_to_add;

    auto add_event_config = [&](uint32_t type, uint64_t config, const char* label) {
        perf_event_attr pe{};
        pe.type = type;
        pe.size = sizeof(pe);
        pe.config = config;
        pe.disabled = 1;       // 생성 시 비활성화, 나중에 명시적으로 활성화
        pe.exclude_kernel = 1; // 커널 공간 이벤트 제외
        pe.exclude_hv = 1;     // 하이퍼바이저 이벤트 제외
        // pe.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING; // 필요시
        events_to_add.emplace_back(pe, label);
    };

    // 일반 HW 이벤트
    add_event_config(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "Instructions");
    add_event_config(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "CPU Cycles");
    add_event_config(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, "Branch Instructions");
    add_event_config(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, "Branch Misses");

    // 캐시 관련 이벤트 (L1D, L3) - 표준 enum 값 사용 권장
    // PERF_COUNT_HW_CACHE_L1D (0), PERF_COUNT_HW_CACHE_L3 (3)
    // PERF_COUNT_HW_CACHE_OP_READ (0), PERF_COUNT_HW_CACHE_OP_WRITE (1)
    // PERF_COUNT_HW_CACHE_RESULT_ACCESS (0), PERF_COUNT_HW_CACHE_RESULT_MISS (1)

    // L1 Data Cache Read Accesses
    add_event_config(PERF_TYPE_HW_CACHE,
                     make_hw_cache_config(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_ACCESS),
                     "L1D Read Access");
    // L1 Data Cache Read Misses
    add_event_config(PERF_TYPE_HW_CACHE,
                     make_hw_cache_config(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS),
                     "L1D Read Miss");
    // L1 Data Cache Write Accesses (만약 L1D 전체 접근/실패를 보려면 추가)
    // add_event_config(PERF_TYPE_HW_CACHE, 
    //                  make_hw_cache_config(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_ACCESS), 
    //                  "L1D Write Access");
    // L1 Data Cache Write Misses
    // add_event_config(PERF_TYPE_HW_CACHE, 
    //                  make_hw_cache_config(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_MISS), 
    //                  "L1D Write Miss");


    
    // L3 Cache (LLC) Write Accesses (필요시)
    // add_event_config(PERF_TYPE_HW_CACHE, 
    //                  make_hw_cache_config(PERF_COUNT_HW_CACHE_L3, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_ACCESS), 
    //                  "L3 Write Access");
    // L3 Cache (LLC) Write Misses (필요시)
    // add_event_config(PERF_TYPE_HW_CACHE, 
    //                  make_hw_cache_config(PERF_COUNT_HW_CACHE_L3, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_MISS), 
    //                  "L3 Write Miss");


    for (const auto& [pe_attr, label_str] : events_to_add) {
        // pid = 0 (current thread), cpu = -1 (any cpu), group_fd = -1 (no group), flags = 0
        int fd = syscall(__NR_perf_event_open, const_cast<perf_event_attr*>(&pe_attr), 0, -1, -1, 0);
        if (fd == -1) {
            // strerror(errno)를 사용하면 더 구체적인 오류 원인을 알 수 있습니다.
            fprintf(stderr, "[ERROR] perf_event_open failed for %s: %s\n", label_str, strerror(errno));
            continue; // 실패한 이벤트는 건너뛰고 계속 진행
        }
        ioctl(fd, PERF_EVENT_IOC_RESET, 0); // 카운터 리셋
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); // 카운터 활성화
        perf_counters.push_back({fd, 0, label_str});
    }
}

void stop_perf_counters() {
    for (size_t i = 0; i < perf_counters.size(); ++i) {
        if (perf_counters[i].fd == -1) continue; // 유효하지 않은 fd는 건너뜀

        ioctl(perf_counters[i].fd, PERF_EVENT_IOC_DISABLE, 0); // 카운터 비활성화
        if (read(perf_counters[i].fd, &perf_counters[i].value, sizeof(uint64_t)) != sizeof(uint64_t)) {
            fprintf(stderr, "[ERROR] Reading perf counter failed for %s: %s\n", perf_counters[i].label, strerror(errno));
        }
        close(perf_counters[i].fd); // 파일 디스크립터 닫기
        printf("[PERF] %s: %lu\n", perf_counters[i].label, perf_counters[i].value);
    }
    perf_counters.clear(); // 사용한 카운터 목록 비우기
}

// 마이크로초(microseconds)를 반환하도록 수정
double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

// get_time_in_ms 함수는 이제 get_time_in_us를 사용하여 밀리초를 반환 (또는 필요시 별도 유지)
// double get_time_in_ms(void) {
//     return get_time_in_us() / 1000.0;
// }

// ... (LoadJSONFromFile, LoadTXTFromFile 함수는 동일)
std::string LoadJSONFromFile(const std::string& path) {
    std::ifstream fs(path);
    if (!fs.is_open()) {
        std::cerr << "[ERROR] Failed to open JSON file: " << path << std::endl;
        return "";
    }
    std::string data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return data;
}

std::string LoadTXTFromFile(const std::string& path) {
    std::ifstream fs(path);
     if (!fs.is_open()) {
        std::cerr << "[ERROR] Failed to open TXT file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << fs.rdbuf();
    return buffer.str();
}


std::vector<int32_t> token( // 반환 타입을 int32_t로 명시
    const std::string vocab_blob,
    const std::string merges_blob,
    const std::string added_token_json, // 변수명 변경 (json 형태임을 명시)
    const std::string text
) {
    // FromBlobByteLevelBPE의 added_tokens 인자는 JSON 문자열을 받거나, 비어있을 수 있습니다.
    // 실제 tokenizers 라이브러리의 API를 확인해야 합니다.
    // 만약 added_tokens가 파일 경로 등을 받는다면 다르게 처리해야 합니다.
    // 여기서는 JSON 문자열로 가정합니다.
    std::unique_ptr<tokenizers::Tokenizer> tokenizer = tokenizers::Tokenizer::FromBlobByteLevelBPE(vocab_blob, merges_blob, added_token_json);
    if (!tokenizer) {
        std::cerr << "[ERROR] Failed to create tokenizer from blob." << std::endl;
        return {}; // 오류 발생 시 빈 벡터 반환
    }

    //start_perf_counters(); // perf 카운터 시작
    
    double token_start_us = get_time_in_us(); // 마이크로초 단위로 시작 시간 기록
    std::vector<int32_t> token_ids = tokenizer->Encode(text); // Encode 반환 타입이 int32_t로 가정
    double token_end_us = get_time_in_us();   // 마이크로초 단위로 종료 시간 기록
    //print_cpu_usage("bpe 시작");
    //stop_perf_counters(); // perf 카운터 중지 및 결과 출력
    bpe_time = bpe_time + (token_end_us - token_start_us);
    std::cout << "[LOG] 누적 토큰화 처리 시간: " << bpe_time << " µs (≈ " << bpe_time / 1000.0 << " ms)\n";

    return token_ids;
}