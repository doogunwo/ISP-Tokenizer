#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>


#define SHM_READ_KEY 0x01
#define SHM_WRITE_KEY 0x02
#define SHM_SIZE 12288  // 공유 메모리 크기
#define MSG_KEY 1234    // 메시지 큐 키

struct msg_buffer{
    long msg_type;
    char msg_text[10];
};

void write_file_to_shared_memory(char* shm_ptr, const char* file_path){
    FILE* file = fopen(file_path, "r");
    char buffer[SHM_SIZE] = {0};
    fread(buffer, 1, SHM_SIZE -1, file);
    fclose(file);
    
    strncpy(shm_ptr, buffer, SHM_SIZE);
    printf("[SPDK] txt -> SHM");
}

char* init_shm(int shm_key){
    int shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);
    char* shm_ptr = (char*)shmat(shm_id, NULL, 0);
    return shm_ptr;
}

int init_msg_queue(){
    int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    return msg_id;
}

void write_to_shared_memory(char* shm_ptr, const char* data) {
    if (strlen(data) > SHM_SIZE) {
        fprintf(stderr, "[SPDK] 경고: 공유 메모리 크기를 초과하는 데이터!\n");
        return;
    }
    strncpy(shm_ptr, data, SHM_SIZE);
}

void send_bpe_command(int msg_id) {
    struct msg_buffer msg;
    msg.msg_type = 1;
    strcpy(msg.msg_text, "\xd4");

    if (msgsnd(msg_id, &msg, sizeof(msg.msg_text), 0) == -1)
        perror("[SPDK] BPE -> MSG send Fail");
    else
        printf("[SPDK] BPE 프로세스 실행 요청 (0xd4)\n");
}

void receive_bpe_response(int msg_id) {
    struct msg_buffer msg;
    if (msgrcv(msg_id, &msg, sizeof(msg.msg_text), 2, 0) == -1) {
        perror("[SPDK] BPE 응답 수신 실패");
        return;
    }

    if (strcmp(msg.msg_text, "\xd5") == 0)
        printf("[SPDK] BPE 프로세스 완료 응답 수신 (0xd5)\n");
    else
        fprintf(stderr, "[SPDK] 알 수 없는 메시지 수신: %s\n", msg.msg_text);
}

// BPE가 처리한 결과를 공유 메모리에서 읽기
void read_from_shared_memory(char* shm_ptr) {
    size_t read_token_count;
    memcpy(&read_token_count, shm_ptr, sizeof(size_t));

    int32_t* read_token_ids = (int32_t*)(shm_ptr + sizeof(size_t));

    printf("[SPDK] 읽은 토큰 개수: %ld\n", read_token_count);
    printf("[SPDK] 저장된 토큰 일부: ");
    for (size_t i = 0; i < (read_token_count > 10 ? 10 : read_token_count); i++) {
        printf("%d ", read_token_ids[i]);
    }
    printf("...\n");
}

int main() {
    // 공유 메모리 및 메시지 큐 초기화
    char* shm_read_ptr = init_shm(SHM_READ_KEY);
    char* shm_write_ptr = init_shm(SHM_WRITE_KEY);
    int msg_id = init_msg_queue();

    write_file_to_shared_memory(shm_read_ptr, "../test/wiki_corpus.txt");
    printf("[SPDK] 공유 메모리에 테스트 데이터 저장 완료\n");

    // BPE 프로세스 실행 요청 메시지 전송
    send_bpe_command(msg_id);

    // BPE 프로세스가 완료될 때까지 대기
    receive_bpe_response(msg_id);

    // BPE가 처리한 데이터 읽기
    read_from_shared_memory(shm_write_ptr);

    return 0;
}