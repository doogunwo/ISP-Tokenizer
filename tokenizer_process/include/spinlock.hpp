#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

#include <atomic>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
class SpinLock {
public:
  SpinLock();
  void lock(int core_id);
  void unlock();

  void bind_to_core(int core_id);

private:
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

#endif // SPINLOCK_HPP
//
