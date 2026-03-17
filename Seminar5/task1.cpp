#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <vector>
#include <windows.h>

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

template <class T>
class ResourcePool {
 public:
  explicit ResourcePool(std::vector<T> initial)
      : resources_(std::move(initial)),
        semaphore_(static_cast<int>(resources_.size())),
        failed_attempts_(0) {}

  T acquire(int priority, int timeout_ms) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      waiters_.push_back(Waiter{priority, std::this_thread::get_id()});
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    bool counted_failure = false;

    while (true) {
      {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return is_my_turn_nolock(priority, std::this_thread::get_id()); });
      }

      auto now = std::chrono::steady_clock::now();
      if (!counted_failure && now < deadline) {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (!semaphore_.try_acquire_for(remaining)) {
          failed_attempts_.fetch_add(1);
          counted_failure = true;
          {
            std::lock_guard<std::mutex> lk(mtx_);
            std::cout << std::this_thread::get_id() << " приоритет=" << priority << " захват таймаут"
                      << std::endl;
          }
          std::this_thread::yield();
          continue;
        }
      } else {
        semaphore_.acquire();
      }

      T res{};
      {
        std::lock_guard<std::mutex> lk(mtx_);
        res = resources_.back();
        resources_.pop_back();
        erase_waiter_nolock(priority, std::this_thread::get_id());
        std::cout << std::this_thread::get_id() << " приоритет=" << priority << " захват успешно"
                  << std::endl;
      }
      cv_.notify_all();
      return res;
    }
  }

  void release(T res) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      resources_.push_back(std::move(res));
      std::cout << std::this_thread::get_id() << " освобождение" << std::endl;
    }
    semaphore_.release();
    cv_.notify_all();
  }

  void add_resource(T res) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      resources_.push_back(std::move(res));
    }
    semaphore_.release();
    cv_.notify_all();
  }

  std::optional<T> remove_resource_for(int timeout_ms) {
    if (!semaphore_.try_acquire_for(std::chrono::milliseconds(timeout_ms))) {
      return std::nullopt;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    if (resources_.empty()) {
      semaphore_.release();
      return std::nullopt;
    }
    T res = std::move(resources_.back());
    resources_.pop_back();
    return res;
  }

  int failed_attempts() const { return failed_attempts_.load(); }

 private:
  struct Waiter {
    int priority;
    std::thread::id tid;
  };

  bool is_my_turn_nolock(int priority, std::thread::id tid) const {
    int best = std::numeric_limits<int>::min();
    for (const auto& w : waiters_) best = std::max(best, w.priority);
    if (priority != best) return false;
    for (const auto& w : waiters_) {
      if (w.priority == best && w.tid == tid) return true;
    }
    return false;
  }

  void erase_waiter_nolock(int priority, std::thread::id tid) {
    for (size_t i = 0; i < waiters_.size(); ++i) {
      if (waiters_[i].priority == priority && waiters_[i].tid == tid) {
        waiters_.erase(waiters_.begin() + static_cast<std::ptrdiff_t>(i));
        return;
      }
    }
  }

  std::vector<T> resources_;
  CountingSemaphore semaphore_;
  mutable std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<Waiter> waiters_;
  std::atomic<int> failed_attempts_;
};

int main() {

  SetConsoleOutputCP(CP_UTF8);   // Консоль переключается на русскую кодировку
  SetConsoleCP(CP_UTF8);
  
  using namespace std::chrono_literals;

  ResourcePool<int> pool({1, 2, 3});
  std::atomic<bool> stop{false};

  auto worker = [&](int pr) {
    std::mt19937 rng(static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<int> work_ms(80, 220);

    while (!stop.load()) {
      int res = pool.acquire(pr, 150);
      std::this_thread::sleep_for(std::chrono::milliseconds(work_ms(rng)));
      pool.release(res);
      std::this_thread::yield();
    }
  };

  for (int i = 0; i < 3; ++i) std::thread(worker, 1).detach();
  for (int i = 0; i < 2; ++i) std::thread(worker, 5).detach();
  for (int i = 0; i < 2; ++i) std::thread(worker, 10).detach();

  std::this_thread::sleep_for(3s);
  pool.add_resource(4);
  std::this_thread::sleep_for(3s);
  stop.store(true);
  std::this_thread::sleep_for(500ms);

  std::cout << "неудачных_попыток=" << pool.failed_attempts() << std::endl;
  return 0;
}

