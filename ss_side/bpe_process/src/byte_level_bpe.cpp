#include "../include/tokenizers_cpp.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>
#include <sys/time.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/resource.h>
double bpe_time = 0; 
using json = nlohmann::json;
extern size_t count_raw_tokens(const std::string& text);
// Perf counter 구조체
struct PerfCounter {
    int fd;
    uint64_t value;
    perf_event_attr attr;
};

std::vector<PerfCounter> perf_counters;

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

uint64_t make_hw_cache_config(int cache, int op, int result) {
    return (cache) | (op << 8) | (result << 16);
}


void start_perf_counters() {
    std::vector<std::pair<perf_event_attr, const char*>> events;

    auto add_event = [&](uint32_t type, uint64_t config, const char* label) {
        perf_event_attr pe{};
        pe.type = type;
        pe.size = sizeof(pe);
        pe.config = config;
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;
        events.emplace_back(pe, label);
    };

    // 일반 HW 이벤트
    add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "Instructions");
    add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "CPU Cycles");
    add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, "Branch Instructions");
    add_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, "Branch Misses");

    // 캐시 관련 이벤트 (L1D, L3)
    add_event(PERF_TYPE_HW_CACHE, make_hw_cache_config(0, 0, 0), "L1D Access");
    add_event(PERF_TYPE_HW_CACHE, make_hw_cache_config(0, 0, 1), "L1D Miss");
    add_event(PERF_TYPE_HW_CACHE, make_hw_cache_config(2, 0, 0), "L3 Access");
    add_event(PERF_TYPE_HW_CACHE, make_hw_cache_config(2, 0, 1), "L3 Miss");

    for (auto& [pe, label] : events) {
        int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
        if (fd == -1) {
            perror("perf_event_open");
            continue;
        }
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        perf_counters.push_back({fd, 0, pe});
    }
}

void stop_perf_counters() {
    static const char* labels[] = {
        "Instructions", "CPU Cycles", "Branch Instructions", "Branch Misses",
        "L1D Access", "L1D Miss", "L3 Access", "L3 Miss"
    };

    for (size_t i = 0; i < perf_counters.size(); ++i) {
        ioctl(perf_counters[i].fd, PERF_EVENT_IOC_DISABLE, 0);
        read(perf_counters[i].fd, &perf_counters[i].value, sizeof(uint64_t));
        close(perf_counters[i].fd);
        printf("[PERF] %s: %lu\n", labels[i], perf_counters[i].value);
    }
    perf_counters.clear();
}

double get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<double>(tv.tv_sec * 1e6 + tv.tv_usec);
}

std::string LoadJSONFromFile(const std::string& path) {
    std::ifstream fs(path);
    std::string data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return data;
}

std::string LoadTXTFromFile(const std::string& path) {
    std::ifstream fs(path);
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
    //print_cpu_usage("bpe 호출");
    //stop_perf_counters(); // perf 카운터 중지 및 결과 출력
    bpe_time = bpe_time + (token_end_us - token_start_us);
    // 로그 메시지의 단위를 마이크로초(µs)로 정확히 표시
    std::cout << "[LOG] 누적 토큰화 처리 시간: " << bpe_time << " µs (≈ " << bpe_time / 1000.0 << " ms)\n";

    return token_ids;
}
