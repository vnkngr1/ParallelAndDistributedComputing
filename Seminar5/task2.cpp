#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <random>
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

class ParkingLot {
 public:
  explicit ParkingLot(int capacity)
      : capacity_(capacity),
        semaphore_(capacity),
        occupied_(0),
        vip_waiting_(0),
        normal_waiting_(0) {}

  void park(bool is_vip, int timeout_ms) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      if (is_vip) {
        ++vip_waiting_;
      } else {
        ++normal_waiting_;
      }
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    bool timed_out = false;

    while (true) {
      {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return is_vip || vip_waiting_ == 0; });
      }

      if (!is_vip && !timed_out) {
        auto now = std::chrono::steady_clock::now();
        if (now < deadline) {
          auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
          if (!semaphore_.try_acquire_for(remaining)) {
            timed_out = true;
            std::lock_guard<std::mutex> lk(mtx_);
            std::cout << std::this_thread::get_id() << " тип=обычная парковка таймаут" << std::endl;
            std::this_thread::yield();
            continue;
          }
        } else {
          timed_out = true;
          std::lock_guard<std::mutex> lk(mtx_);
          std::cout << std::this_thread::get_id() << " тип=обычная парковка таймаут" << std::endl;
          std::this_thread::yield();
          continue;
        }
      } else {
        semaphore_.acquire();
      }

      {
        std::lock_guard<std::mutex> lk(mtx_);
        if (is_vip) {
          --vip_waiting_;
        } else {
          --normal_waiting_;
        }
        ++occupied_;
        print_state_nolock(is_vip ? "VIP" : "обычная", "парковка");
      }
      cv_.notify_all();
      std::this_thread::yield();
      return;
    }
  }

  void leave(bool was_vip) {
    {
      std::lock_guard<std::mutex> lk(mtx_);
      if (occupied_ > 0) --occupied_;
      print_state_nolock(was_vip ? "VIP" : "обычная", "выезд");
    }
    semaphore_.release();
    cv_.notify_all();
    std::this_thread::yield();
  }

  void set_capacity(int new_capacity) {
    if (new_capacity < 0) new_capacity = 0;
    int old_capacity;
    {
      std::lock_guard<std::mutex> lk(mtx_);
      old_capacity = capacity_;
      capacity_ = new_capacity;
    }

    int delta = new_capacity - old_capacity;
    if (delta > 0) {
      semaphore_.release(delta);
      cv_.notify_all();
      return;
    }
    if (delta < 0) {
      int need = -delta;
      for (int i = 0; i < need; ++i) semaphore_.acquire();
    }
  }

 private:
  void print_state_nolock(const char* type, const char* action) {
    int free = std::max(0, capacity_ - occupied_);
    std::cout << std::this_thread::get_id() << " тип=" << type << " действие=" << action
              << " занято=" << occupied_ << " свободно=" << free << std::endl;
  }

  int capacity_;
  CountingSemaphore semaphore_;
  std::mutex mtx_;
  std::condition_variable cv_;
  int occupied_;
  int vip_waiting_;
  int normal_waiting_;
};

int main() {
  using namespace std::chrono_literals;

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  ParkingLot lot(3);
  std::atomic<bool> stop{false};

  auto car = [&](bool vip) {
    std::mt19937 rng(static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<int> stay_ms(200, 700);
    while (!stop.load()) {
      lot.park(vip, 300);
      std::this_thread::sleep_for(std::chrono::milliseconds(stay_ms(rng)));
      lot.leave(vip);
      std::this_thread::yield();
    }
  };

  for (int i = 0; i < 6; ++i) std::thread(car, false).detach();
  for (int i = 0; i < 2; ++i) std::thread(car, true).detach();

  std::this_thread::sleep_for(3s);
  lot.set_capacity(5);
  std::this_thread::sleep_for(3s);
  lot.set_capacity(2);
  std::this_thread::sleep_for(3s);

  stop.store(true);
  std::this_thread::sleep_for(600ms);
  return 0;
}

