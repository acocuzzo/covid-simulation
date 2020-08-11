#ifndef STATUSOR
#define STATUSOR

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace util {
class ThreadPool {
 public:
  explicit ThreadPool(std::uint32_t threads) : stop(false) {
    workers.reserve(threads);
    for (std::int32_t i = 0; i < threads; ++i) {
      workers.emplace_back(std::bind(&ThreadPool::doWork, this));
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) {
      worker.join();
    }
  }

  inline void enqueue(std::function<void()> new_task) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      tasks.push(std::move(new_task));
    }
    condition.notify_one();
  }

 private:
  inline void doWork() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queue_mutex);
        condition.wait(lock,
                       [this] { return this->stop || !this->tasks.empty(); });
        if (stop && tasks.empty()) {
          return;
        }
        task = std::move(tasks.front());
        tasks.pop();
      }
      task();
    }
  }

  bool stop;
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
};

struct Status {
  enum Code { OK, INTERNAL, NOT_FOUND };

  Status(Code code_in, std::string message_in) {
    code = code_in;
    message = std::move(message_in);
  }

  static Status make_OK() { return Status(Code::OK, ""); }

  bool ok() const { return code == OK; }

  Code code;
  std::string message;
};

template <class T>
class StatusOr {
 public:
  StatusOr(T t_) : _ok(true) {
    new (reinterpret_cast<T*>(_buf)) T(std::move(t_));
  }
  StatusOr(Status s_) : _ok(false) {
    new (reinterpret_cast<Status*>(_buf)) Status(std::move(s_));
  }

  ~StatusOr() {
    if (_ok) {
      reinterpret_cast<T*>(_buf)->~T();
    } else {
      reinterpret_cast<Status*>(_buf)->~Status();
    }
  }

  T& operator*() { return *reinterpret_cast<T*>(_buf); }
  T* operator->() { return reinterpret_cast<T*>(_buf); }
  const T& operator*() const { return *reinterpret_cast<T*>(_buf); }
  const T* operator->() const { return reinterpret_cast<T*>(_buf); }
  bool ok() const { return _ok; }
  const Status& error() const { return *reinterpret_cast<Status*>(_buf); }
  Status& error() { return *reinterpret_cast<Status*>(_buf); }

 private:
  const bool _ok;
  std::byte _buf[std::max(sizeof(T), sizeof(Status))];
};
}  // namespace util

namespace std {
inline ostream& operator<<(ostream& os, const util::Status& status) {
  os << status.code << ": " << status.message;
  return os;
}

inline string to_string(const util::Status::Code& code_) {
  switch (code_) {
    case util::Status::Code::OK:
      return "OK";
    case util::Status::Code::INTERNAL:
      return "INTERNAL";
    case util::Status::Code::NOT_FOUND:
      return "NOT_FOUND";
  }
}
}  // namespace std

#endif