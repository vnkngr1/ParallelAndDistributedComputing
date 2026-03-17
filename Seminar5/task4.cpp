#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class CountingSemaphore {
 public:
  explicit CountingSemaphore(int initial) : count_(initial) {}

  void release(int n = 1) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      count_ += n;
    }
    cv_.notify_all();
  }

  void acquire() {
    std::unique_lock<std::mutex> lk(mtx_);
    cv_.wait(lk, [&] { return count_ > 0; });
    --count_;
  }

  template <class Rep, class Period>
  bool try_acquire_for(const std::chrono::duration<Rep, Period>& d) {
    std::unique_lock<std::mutex> lk(mtx_);
    if (!cv_.wait_for(lk, d, [&] { return count_ > 0; })) return false;
    --count_;
    return true;
  }

 private:
  std::mutex mtx_;
  std::condition_variable cv_;
  int count_;
};

class PrinterQueue {
 public:
  explicit PrinterQueue(int n_printers)
      : n_printers_(n_printers),
        semaphore_(n_printers),
        seq_(0),
        stop_(false) {
    dispatcher_ = std::thread([&] { dispatch_loop(); });
    dispatcher_.detach();
  }

  void submit(std::string doc, int priority, int timeout_ms) {
    Job j;
    j.doc = std::move(doc);
    j.priority = priority;
    j.timeout_ms = timeout_ms;
    j.seq = seq_.fetch_add(1);
    {
      std::lock_guard<std::mutex> lk(mtx_);
      jobs_.push(std::move(j));
    }
    cv_.notify_one();
  }

  void stop() {
    stop_.store(true);
    cv_.notify_all();
  }

 private:
  struct Job {
    std::string doc;
    int priority;
    int timeout_ms;
    long long seq;
  };

  struct Cmp {
    bool operator()(const Job& a, const Job& b) const {
      if (a.priority != b.priority) return a.priority < b.priority;
      return a.seq > b.seq;
    }
  };

  void dispatch_loop() {
    using namespace std::chrono_literals;
    std::mt19937 rng(static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<int> maybe_cancel(0, 9);

    while (!stop_.load()) {
      Job job;
      {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return stop_.load() || !jobs_.empty(); });
        if (stop_.load()) return;
        job = jobs_.top();
        jobs_.pop();
      }

      auto got = semaphore_.try_acquire_for(std::chrono::milliseconds(job.timeout_ms));
      if (!got) {
        std::lock_guard<std::mutex> lk(mtx_);
        std::cout << std::this_thread::get_id() << " приоритет=" << job.priority
                  << " документ=" << job.doc << " статус=таймаут_возврат" << std::endl;
        jobs_.push(std::move(job));
        cv_.notify_one();
        std::this_thread::yield();
        continue;
      }

      std::thread([this, j = std::move(job), &rng, maybe_cancel]() mutable {
        using namespace std::chrono_literals;
        {
          std::lock_guard<std::mutex> lk(mtx_);
          std::cout << std::this_thread::get_id() << " приоритет=" << j.priority
                    << " документ=" << j.doc << " статус=печать" << std::endl;
        }

        int total_ms = 500 + (j.priority >= 5 ? 200 : 0);
        int step_ms = 50;
        bool interrupted = false;
        for (int t = 0; t < total_ms; t += step_ms) {
          if (maybe_cancel(rng) == 0) interrupted = true;
          if (interrupted) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
          std::this_thread::yield();
        }

        if (interrupted) {
          {
            std::lock_guard<std::mutex> lk(mtx_);
            std::cout << std::this_thread::get_id() << " приоритет=" << j.priority
                      << " документ=" << j.doc << " статус=прервано_возврат" << std::endl;
            jobs_.push(std::move(j));
          }
          semaphore_.release();
          cv_.notify_one();
          return;
        }

        {
          std::lock_guard<std::mutex> lk(mtx_);
          std::cout << std::this_thread::get_id() << " приоритет=" << j.priority
                    << " документ=" << j.doc << " статус=готово" << std::endl;
        }
        semaphore_.release();
      }).detach();

      std::this_thread::yield();
      std::this_thread::sleep_for(10ms);
    }
  }

  int n_printers_;
  CountingSemaphore semaphore_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::priority_queue<Job, std::vector<Job>, Cmp> jobs_;
  std::atomic<long long> seq_;
  std::atomic<bool> stop_;
  std::thread dispatcher_;
};

int main() {
  using namespace std::chrono_literals;

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  PrinterQueue pq(2);

  std::thread([&] {
    for (int i = 0; i < 10; ++i) {
      int pr = (i % 3 == 0) ? 10 : 1;
      pq.submit("doc" + std::to_string(i), pr, 200);
      std::this_thread::sleep_for(120ms);
      std::this_thread::yield();
    }
  }).detach();

  std::this_thread::sleep_for(8s);
  pq.stop();
  std::this_thread::sleep_for(300ms);
  return 0;
}

