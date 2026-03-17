#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
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
  explicit CountingSemaphore(int initial = 0) : count_(initial) {}
  CountingSemaphore(const CountingSemaphore&) = delete;
  CountingSemaphore& operator=(const CountingSemaphore&) = delete;

  CountingSemaphore(CountingSemaphore&& other) noexcept {
    std::lock_guard<std::mutex> lk(other.mtx_);
    count_ = other.count_;
    other.count_ = 0;
  }

  CountingSemaphore& operator=(CountingSemaphore&& other) noexcept {
    if (this == &other) return *this;
    int v = 0;
    {
      std::lock_guard<std::mutex> lk(other.mtx_);
      v = other.count_;
      other.count_ = 0;
    }
    {
      std::lock_guard<std::mutex> lk(mtx_);
      count_ = v;
    }
    cv_.notify_all();
    return *this;
  }

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
  int count_{0};
};

template <class T>
class SemaphoreBuffer {
 public:
  SemaphoreBuffer(int k_buffers, int capacity_each)
      : buffers_(static_cast<size_t>(k_buffers)),
        mtx_(static_cast<size_t>(k_buffers)),
        capacity_each_(capacity_each),
        produce_timeouts_(0),
        consume_timeouts_(0) {
    empty_.reserve(static_cast<size_t>(k_buffers));
    full_.reserve(static_cast<size_t>(k_buffers));
    for (int i = 0; i < k_buffers; ++i) {
      empty_.emplace_back(capacity_each_);
      full_.emplace_back(0);
    }
  }

  bool produce(T value, int buffer_index, int timeout_ms) {
    auto& empty = empty_[static_cast<size_t>(buffer_index)];
    if (!empty.try_acquire_for(std::chrono::milliseconds(timeout_ms))) {
      produce_timeouts_.fetch_add(1);
      std::lock_guard<std::mutex> lk(console_mtx_);
      std::cout << std::this_thread::get_id() << " буфер=" << buffer_index
                << " действие=положить таймаут=" << timeout_ms << std::endl;
      std::this_thread::yield();
      return false;
    }

    {
      std::lock_guard<std::mutex> lk(mtx_[static_cast<size_t>(buffer_index)]);
      buffers_[static_cast<size_t>(buffer_index)].push_back(std::move(value));
    }
    full_[static_cast<size_t>(buffer_index)].release();

    {
      std::lock_guard<std::mutex> lk(console_mtx_);
      std::cout << std::this_thread::get_id() << " буфер=" << buffer_index
                << " действие=положить успешно" << std::endl;
    }
    std::this_thread::yield();
    return true;
  }

  bool consume(int buffer_index, int timeout_ms, T& out) {
    auto& full = full_[static_cast<size_t>(buffer_index)];
    if (!full.try_acquire_for(std::chrono::milliseconds(timeout_ms))) {
      consume_timeouts_.fetch_add(1);
      std::lock_guard<std::mutex> lk(console_mtx_);
      std::cout << std::this_thread::get_id() << " буфер=" << buffer_index
                << " действие=взять таймаут=" << timeout_ms << std::endl;
      std::this_thread::yield();
      return false;
    }

    {
      std::lock_guard<std::mutex> lk(mtx_[static_cast<size_t>(buffer_index)]);
      out = std::move(buffers_[static_cast<size_t>(buffer_index)].front());
      buffers_[static_cast<size_t>(buffer_index)].pop_front();
    }
    empty_[static_cast<size_t>(buffer_index)].release();

    {
      std::lock_guard<std::mutex> lk(console_mtx_);
      std::cout << std::this_thread::get_id() << " буфер=" << buffer_index
                << " действие=взять успешно" << std::endl;
    }
    std::this_thread::yield();
    return true;
  }

  long long produce_timeouts() const { return produce_timeouts_.load(); }
  long long consume_timeouts() const { return consume_timeouts_.load(); }
  int buffers_count() const { return static_cast<int>(buffers_.size()); }

 private:
  std::vector<std::deque<T>> buffers_;
  std::vector<CountingSemaphore> empty_;
  std::vector<CountingSemaphore> full_;
  std::vector<std::mutex> mtx_;
  int capacity_each_;

  std::mutex console_mtx_;
  std::atomic<long long> produce_timeouts_;
  std::atomic<long long> consume_timeouts_;
};

int main() {
  using namespace std::chrono_literals;

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  SemaphoreBuffer<int> sb(3, 4);
  std::atomic<bool> stop{false};

  auto producer = [&] {
    std::mt19937 rng(static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<int> buf_pick(0, sb.buffers_count() - 1);
    std::uniform_int_distribution<int> val_pick(1, 1000);
    std::uniform_int_distribution<int> sleep_ms(30, 90);

    while (!stop.load()) {
      int first = buf_pick(rng);
      bool ok = false;
      int v = val_pick(rng);
      for (int t = 0; t < sb.buffers_count(); ++t) {
        int idx = (first + t) % sb.buffers_count();
        if (sb.produce(v, idx, 120)) {
          ok = true;
          break;
        }
      }
      if (!ok) std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms(rng)));
    }
  };

  auto consumer = [&] {
    std::mt19937 rng(static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<int> buf_pick(0, sb.buffers_count() - 1);
    std::uniform_int_distribution<int> sleep_ms(40, 110);

    while (!stop.load()) {
      int first = buf_pick(rng);
      bool ok = false;
      int out = 0;
      for (int t = 0; t < sb.buffers_count(); ++t) {
        int idx = (first + t) % sb.buffers_count();
        if (sb.consume(idx, 120, out)) {
          ok = true;
          break;
        }
      }
      if (!ok) std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms(rng)));
    }
  };

  for (int i = 0; i < 3; ++i) std::thread(producer).detach();
  for (int i = 0; i < 4; ++i) std::thread(consumer).detach();

  std::this_thread::sleep_for(6s);
  stop.store(true);
  std::this_thread::sleep_for(500ms);

  std::cout << "таймаут_положить=" << sb.produce_timeouts() << std::endl;
  std::cout << "таймаут_взять=" << sb.consume_timeouts() << std::endl;
  return 0;
}

