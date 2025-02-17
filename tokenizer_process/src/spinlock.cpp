#include "../include/spinlock.hpp"
#include <atomic>
#include <pthread.h>
#include <sched.h>

SpinLock::SpinLock(){
  flag.clear();
}

void SpinLock::bind_to_core(int core_id){
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  pthread_t thread = pthread_self();
  pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

void SpinLock::lock(int core_id){
  bind_to_core(core_id);
  while(flag.test_and_set(std::memory_order_acquire)){
    sched_yield();
  }
}

void SpinLock::unlock(){
  flag.clear(std::memory_order_release);
}
