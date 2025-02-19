#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <atomic>
#include <vector>  // 

#include "../include/tokenizers_cpp.h"
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <pthread.h> //spinlock
#include <nlohmann/json.hpp>
#include <memory>

#define SHM_READ_KEY 0x01
#define SHM_WRITE_KEY 0x02
#define SHM_SIZE 12288  // 공유 메모리 크기
#define MSG_KEY 1234  //  SPDK Mock과 동일한 키 사용

using json = nlohmann::json;
pthread_spinlock_t spinlock;

// byte_level_bpe.cpp에 정의된 함수들 (extern으로 선언)
extern std::string LoadJSONFromFile(const std::string& path);
extern std::string LoadTXTFromFile(const std::string& path);
extern std::vector<int32_t> token(const std::string vocab_blob, const std::string merges_blob, const std::string added_token, const std::string text);

//  공유 메모리 초기화
char* init_shm(int shm_key) {
    int shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("공유 메모리 생성 실패");
        exit(1);
    }
    char* shm_ptr = static_cast<char*>(shmat(shm_id, NULL, 0));  // 
    if (shm_ptr == (char *)(-1)) {
        perror("공유 메모리 연결 실패");
        exit(1);
    }
    return shm_ptr;
}

// 공유 메모리 해제
void detach_shm(char* shm_ptr, int shm_key) {
    shmdt(shm_ptr);
    shmctl(shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666), IPC_RMID, NULL);
}

//  SPDK → BPE (읽기용 공유 메모리에서 데이터 읽기)
std::string read_from_spdk(char* shm_read_ptr) {
    return std::string(shm_read_ptr, SHM_SIZE);
}

// SPDK로부터 명령 수신 0xd4 받으면 BPE 시작
bool receive_spdk_command(int msg_id){
    struct msg_buffer {
        long msg_type;
        char msg_text[10];
    }msg;

    msgrcv(msg_id, &msg, sizeof(msg.msg_text), 1, 0);
    return (strcmp(msg.msg_text, "\xd4") ==0);
}

//  BPE → SPDK (쓰기용 공유 메모리에 데이터 저장)
void write_to_spdk(char* shm_write_ptr, const std::vector<int32_t>& token_ids) {
    size_t token_count = token_ids.size();
    size_t byte_size = token_count * sizeof(int32_t);

    memset(shm_write_ptr, 0, SHM_SIZE);
    memcpy(shm_write_ptr, &token_count, sizeof(size_t));
    memcpy(shm_write_ptr + sizeof(size_t), token_ids.data(), byte_size);
    std::cout <<"[BPE] saved token counts : " << token_count << std::endl;
}

//  SPDK 메시지 큐로 완료 신호 전송
void send_spdk_response(int msg_id) {
    struct msg_buffer {
        long msg_type;
        char msg_text[10];
    } response;

    response.msg_type = 2;
    strcpy(response.msg_text, "\xd5");

    if (msgsnd(msg_id, &response, sizeof(response.msg_text), 0) == -1)perror("[BPE] SPDK <- MSG send Fail");
    else std::cout << "[BPE] SPDK <- 0xd5 Finish" << std::endl;  //  세미콜론 추가
}

//  BPE 토큰화 및 공유 메모리 쓰기 함수
void bpe_worker(char* shm_read_ptr, char* shm_write_ptr, 
                const std::string& vocab, const std::string& merges) {
    std::string added_token = R"({
        "[PAD]": 0,
        "[UNK]": 1,
        "[CLS]": 2,
        "[SEP]": 3,
        "[MASK]": 4
    })";

    std::cout << "[BPE] SPDK로부터 데이터 읽기..." << std::endl;
    std::string input_text = read_from_spdk(shm_read_ptr);

    std::cout << "[BPE] BPE 토큰화 수행 중..." << std::endl;
    std::vector<int32_t> token_ids = token(vocab, merges, added_token, std::string(input_text));

    std::cout << "[BPE] BPE 토큰화 완료, 공유 메모리에 저장 중..." << std::endl;
    write_to_spdk(shm_write_ptr, token_ids);
}

int main() {
    // 2개 공유 메모리 연결
    char* shm_read_ptr  = init_shm(SHM_READ_KEY);
    char* shm_write_ptr = init_shm(SHM_WRITE_KEY);

    int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);

    // 스핀락
    pthread_spin_init(&spinlock, 0);

    // 모델 로드
    std::string json_blob = LoadJSONFromFile("../model/byte_level_bpe_model.json");
    json j;
    try{
        j= json::parse(json_blob);
    }
    catch(const json::parse_error& e){
        std::cerr << "json parsing error" << e.what() << std::endl;
        return -1;
    }


    json model = j["model"];

    std::string vocab_blob = model["vocab"].dump();
    
    std::string merges_blob = LoadTXTFromFile("../model/merges.txt");

    //----------------
    while (true) {
        std::cout << "[MAIN] SPDK Wait..." << std::endl;

        while (true) {
            pthread_spin_lock(&spinlock);
            if (receive_spdk_command(msg_id)) break;
            pthread_spin_unlock(&spinlock);
            usleep(10);
        }

        std::cout << "[MAIN] SPDK 메시지 수신! BPE 프로세스 실행 시작..." << std::endl;
        bpe_worker(shm_read_ptr, shm_write_ptr, vocab_blob, merges_blob);

        send_spdk_response(msg_id);  // BPE -> SPDK
        pthread_spin_unlock(&spinlock);
    }

    // 공유 메모리 해제
    pthread_spin_destroy(&spinlock);
    detach_shm(shm_read_ptr, SHM_READ_KEY);
    detach_shm(shm_write_ptr, SHM_WRITE_KEY);

    return 0;
}
