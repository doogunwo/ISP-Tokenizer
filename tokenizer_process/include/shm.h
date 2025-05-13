#pragma once

#include <cstddef>
#include <string>
#include <vector>

#define SHM_READ_KEY 0x11
#define SHM_WRITE_KEY 0x22
#define MSG_KEY 1000          // SPDK Mock과 동일한 키 사용
#define SHM_SIZE 131072       // 공유 메모리 크기

char* init_shm(int key, size_t size = SHM_SIZE);
void detach_shm(char* ptr, int key, size_t size = SHM_SIZE);
std::string read_from_spdk(char* shm_read_ptr);
void write_to_shm(char* shm_ptr, const void* src, size_t len);