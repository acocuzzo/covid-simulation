#ifndef MUTEX_H_
#define MUTEX_H_

#include <condition_variable>
#include <functional>
#include <shared_mutex>

#include "thread_safety.h"

/*
Mutex mu;
bool done = false GUARDED_BY(mu);
int returnvalue GUARDED_BY(mu);

auto fn = [&mu, &done, &returnvalue]()
{
    auto myval = some_function();
    MutexLock l(&mu);
    done = true;
    returnvalue = myval;
};
_executor->execute(std::move(fn));

ReaderMutexLock l(&mu);
mu.await(&done);
// Now returnvalue can be used safely
*/

class CAPABILITY("mutex") Mutex {
 public:
  Mutex() = default;
  ~Mutex() = default;

  Mutex(Mutex&) = delete;
  Mutex operator=(Mutex&) = delete;

  void lock() ACQUIRE() { _mu.lock(); }
  void unlock() RELEASE() {
    _mu.unlock();
    _cv.notify_all();
  }
  void readerLock() ACQUIRE_SHARED() { _mu.lock_shared(); }
  void readerUnlock() RELEASE_SHARED() { _mu.unlock_shared(); }

  void await(std::function<bool()> cond_) ASSERT_SHARED_CAPABILITY(this) {
    struct Locker {
      explicit Locker(Mutex* mu_) : _mu(mu_) {}
      void lock() ACQUIRE_SHARED(_mu) { _mu->readerLock(); }
      void unlock() RELEASE_SHARED(_mu) { _mu->readerUnlock(); }

      Mutex* _mu;
    };
    Locker l(this);
    _cv.wait(l, cond_);
  }

  void await(bool* cond_) ASSERT_SHARED_CAPABILITY(this) {
    return await([cond_]() { return *cond_; });
  }

 private:
  std::shared_mutex _mu;
  std::condition_variable_any _cv;
};

class SCOPED_CAPABILITY MutexLock {
 public:
  MutexLock(Mutex* mu) ACQUIRE(mu) : _mu(mu) { _mu->lock(); }
  ~MutexLock() RELEASE(_mu) { _mu->unlock(); }

 private:
  Mutex* _mu;
};

class SCOPED_CAPABILITY ReaderMutexLock {
 public:
  ReaderMutexLock(Mutex* mu) ACQUIRE_SHARED(mu) : _mu(mu) { _mu->readerLock(); }
  ~ReaderMutexLock() RELEASE_SHARED(_mu) { _mu->readerUnlock(); }

 private:
  Mutex* _mu;
};

#endif  // MUTEX_H_