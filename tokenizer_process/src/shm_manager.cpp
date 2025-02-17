#include "../include/shm_manager.hpp"
#include <cstdio>
#include <fcntl.h>


size_t align_to_page_size(size_t size) {
    size_t page_size = 4096; // 4KB
    return (size > 0) ? ((size + page_size - 1) / page_size) * page_size : page_size;
}



ShmManager::ShmManager(const char* name, size_t requested_size)
    : name(name), size(align_to_page_size(requested_size)) { // 4KB 정렬 적용
    fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  
    if (ftruncate(fd, size) == -1) { // 정렬된 size 사용
        perror("ftruncate");
        close(fd);
    }

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
    }
}


ShmManager::~ShmManager() {
    if (addr) {
        munmap(addr, size);
    }
    if (fd != -1) {
        close(fd);
    }
}

void* ShmManager::get_addr() {
    return addr;
}

void ShmManager::write(const void* data, size_t size) {
    if (size > this->size) {
        return;
    }
    memcpy(addr, data, size);
}

void ShmManager::read(void* buffer, size_t size) {
    if (size > this->size) {
        return;
    }
    memcpy(buffer, addr, size);
}

