#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include "../include/shm.h"


char* init_shm(int key, size_t size) {
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget 실패");
        exit(1);
    }
    char* shm_ptr = (char*)shmat(shmid, nullptr, 0);
    if (shm_ptr == (char*)-1) {
        perror("shmat 실패");
        exit(1);
    }
    return shm_ptr;
}

void detach_shm(char* ptr, int key, size_t size) {
    shmdt(ptr);
    shmctl(shmget(key, size, IPC_CREAT | 0666), IPC_RMID, nullptr);
}

std::string read_from_spdk(char* shm_read_ptr) {
    return std::string(shm_read_ptr, SHM_SIZE);
}

// buffer → SHM으로 데이터 복사
void write_to_shm(char* shm_ptr, const void* src, size_t len) {
    memset(shm_ptr, 0, SHM_SIZE);
    memcpy(shm_ptr, src, std::min(static_cast<size_t>(SHM_SIZE), len));
}
