#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <sys/time.h>
#include <sstream>
#include <sys/resource.h>
#define SHM_READ_KEY 0x11
#define SHM_WRITE_KEY 0x22
#define MSG_KEY 1000  // SPDK Mock과 동일한 키 사용


#define SHM_SIZE 131072  // 공유 메모리 크기

//#define SHM_SIZE 131072  // 공유 메모리 크기

std::string clean_invalid_utf8(const std::string& str) {
    std::string result;
    size_t i = 0;
    while (i < str.size()) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        size_t len = 0;

        if (c <= 0x7F) len = 1; // 0xxxxxxx
        else if ((c & 0xE0) == 0xC0) len = 2; // 110xxxxx
        else if ((c & 0xF0) == 0xE0) len = 3; // 1110xxxx
        else if ((c & 0xF8) == 0xF0) len = 4; // 11110xxx
        else {
            ++i; // invalid leading byte, skip
            continue;
        }

        if (i + len > str.size()) break;

        bool valid = true;
        for (size_t j = 1; j < len; ++j) {
            if ((static_cast<unsigned char>(str[i + j]) & 0xC0) != 0x80) {
                valid = false;
                break;
            }
        }

        if (valid)
            result.append(str.substr(i, len));

        i += len;
    }
    return result;
}

using json = nlohmann::json;
static double global_total_start = 0.0;
extern double get_time_in_us();
double get_elapsed() {
    return get_time_in_us() - global_total_start;
}

// byte_level_bpe.cpp에 정의된 함수들 (extern으로 선언)
extern std::string LoadJSONFromFile(const std::string& path);
extern std::string LoadTXTFromFile(const std::string& path);
extern std::vector<int> token(const std::string vocab_blob, const std::string merges_blob, const std::string added_token, const std::string text);
extern void print_cpu_usage(const std::string& label);
size_t count_raw_tokens(const std::string& text) {
    std::istringstream iss(text);
    std::string token;
    size_t count = 0;
    while (iss >> token) {
        ++count;
    }
    return count;
}


// 공유 메모리 초기화
char* init_shm(int shm_key) {
    int shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("공유 메모리 생성 실패");
        exit(1);
    }
    char* shm_ptr = static_cast<char*>(shmat(shm_id, NULL, 0));
    if (shm_ptr == (char *)(-1)) {
        perror("공유 메모리 연결 실패");
        exit(1);
    }
    return shm_ptr;
}



// SPDK → BPE (읽기용 공유 메모리에서 데이터 읽기)
std::string read_from_spdk(char* shm_read_ptr) {
    std::string data(shm_read_ptr, SHM_SIZE);
    return data;
}

uint32_t write_to_spdk(char* shm_write_ptr, const std::vector<int16_t>& token_ids) {
    size_t token_count = token_ids.size();
    uint32_t byte_size = token_count * sizeof(int16_t);  // 유효 바이트 수
    memcpy(shm_write_ptr, token_ids.data(), byte_size);
    std::cout << "[BPE] 출력 토큰 ID (" << token_count << "개): ";
    std::cout << "...\n";
    return byte_size;
}

// SPDK로부터 명령 수신
int receive_spdk_command(int msg_id) {
    struct msg_buffer {
        long msg_type;
        int cdw13;
    } msg;
    

    if (msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 1, 0) == -1) {
        perror("[BPE] SPDK 요청 수신 실패");
        return -1; // 실패 시 -1 반환
    }
    //print_cpu_usage("실제 호출 시작");
    
    return msg.cdw13; // 성공 시 cdw13 반환
}

// SPDK 메시지 큐로 완료 신호 전송
void send_spdk_response(int msg_id, int cdw13, uint32_t byte_size) {
    struct msg_buffer {
        long msg_type;
        int cdw13;
        uint32_t byte_size;
    } response;

    response.msg_type = 2;
    response.cdw13 = cdw13;
    response.byte_size = byte_size;
    
    if (msgsnd(msg_id, &response, sizeof(response) - sizeof(long), 0) == -1) {
        perror("[ERROR] 메시지 전송 실패");
    }

}


// BPE 토큰화 및 공유 메모리 쓰기 함수
uint32_t bpe_worker(char* shm_read_ptr, char* shm_write_ptr, 
    const std::string& vocab, const std::string& merges) {
    std::string added_token = R"({
    "[PAD]": 0,
    "[UNK]": 1,
    "[CLS]": 2,
    "[SEP]": 3,
    "[MASK]": 4
    })";
    std::string input_text = read_from_spdk(shm_read_ptr);
    //std::cout << input_text;
    //------------------------------------------------------
    std::string cleaned = clean_invalid_utf8(input_text);
    std::vector<int> raw_ids = token(vocab, merges, added_token, cleaned);
    std::vector<int16_t> token_ids(raw_ids.begin(), raw_ids.end());
    //------------------------------------------------------
    uint32_t byte_size = write_to_spdk(shm_write_ptr, token_ids);
    
    
    return byte_size;
}


void message_loop(const std::string& vocab_blob, const std::string& merges_blob, int msg_id) {
    while (true) {
        int cdw13 = receive_spdk_command(msg_id);
        if (cdw13 == -1) continue;

        char* shm_read_ptr = init_shm(SHM_READ_KEY + cdw13);
        char* shm_write_ptr = init_shm(SHM_WRITE_KEY + cdw13);

        uint32_t byte_size = bpe_worker(shm_read_ptr, shm_write_ptr, vocab_blob, merges_blob);
        send_spdk_response(msg_id, cdw13, byte_size);

    }
}

int main() {
    
    std::string model_path = "../model/byte_level_bpe_model.json";
    std::string merges_path = "../model/merges.txt";

    // 메시지 큐
    int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) {
        perror("[ERROR] 메시지 큐 생성 실패");
        return 1;
    }

    // 모델 파싱
    std::string json_blob = LoadJSONFromFile(model_path);
    json j = json::parse(json_blob);
    json model = j["model"];
    std::string vocab_blob = model["vocab"].dump();
    std::string merges_blob = LoadTXTFromFile(merges_path);

    // 메시지 루프 시작
    global_total_start = get_time_in_us();
    message_loop(vocab_blob, merges_blob, msg_id);

    return 0;
}
