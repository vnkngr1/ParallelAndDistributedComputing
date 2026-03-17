#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
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

 private:
  std::mutex mtx_;
  std::condition_variable cv_;
  int count_;
};

struct Task {
  int id{};
  int required_slots{};
  int duration_ms{};
  int priority{};
  std::chrono::steady_clock::time_point submitted_at{};

  void execute() const {
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
  }
};

struct TaskCmp {
  bool operator()(const Task& a, const Task& b) const {
    if (a.priority != b.priority) return a.priority < b.priority;
    return a.id > b.id;
  }
};

class TaskScheduler {
 public:
  TaskScheduler(int total_slots, int workers)
      : resource_semaphore_(total_slots),
        completed_tasks_(0),
        stop_(false),
        total_wait_ms_(0),
        started_tasks_(0) {
    for (int i = 0; i < workers; ++i) {
      std::thread([&] { worker(); }).detach();
    }
  }

  void submit(Task task) {
    task.submitted_at = std::chrono::steady_clock::now();
    {
      std::lock_guard<std::mutex> lk(queue_mutex_);
      queue_.push(task);
    }
    cv_.notify_one();
  }

  int completed_tasks() const { return completed_tasks_.load(); }

  double average_wait_ms() const {
    long long started = started_tasks_.load();
    if (started == 0) return 0.0;
    return static_cast<double>(total_wait_ms_.load()) / static_cast<double>(started);
  }

  void stop() {
    stop_.store(true);
    cv_.notify_all();
  }

 private:
  inline void execute_task(const Task& task) { task.execute(); }

  void acquire_slots(int n) {
    for (int i = 0; i < n; ++i) resource_semaphore_.acquire();
  }

  void release_slots(int n) { resource_semaphore_.release(n); }

  void worker() {
    while (!stop_.load()) {
      Task task;
      {
        std::unique_lock<std::mutex> lk(queue_mutex_);
        cv_.wait(lk, [&] { return stop_.load() || !queue_.empty(); });
        if (stop_.load()) return;
        task = queue_.top();
        queue_.pop();
      }

      acquire_slots(task.required_slots);

      auto start = std::chrono::steady_clock::now();
      auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(start - task.submitted_at).count();
      total_wait_ms_.fetch_add(wait_ms);
      started_tasks_.fetch_add(1);

      {
        std::lock_guard<std::mutex> lk(console_mtx_);
        std::cout << std::this_thread::get_id() << " задача=" << task.id << " ресурсы="
                  << task.required_slots << " статус=старт" << std::endl;
      }

      execute_task(task);

      {
        std::lock_guard<std::mutex> lk(console_mtx_);
        std::cout << std::this_thread::get_id() << " задача=" << task.id << " ресурсы="
                  << task.required_slots << " статус=готово" << std::endl;
      }

      release_slots(task.required_slots);
      completed_tasks_.fetch_add(1);
      std::this_thread::yield();
    }
  }

  std::priority_queue<Task, std::vector<Task>, TaskCmp> queue_;
  CountingSemaphore resource_semaphore_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  std::mutex console_mtx_;
  std::atomic<int> completed_tasks_;
  std::atomic<bool> stop_;
  std::atomic<long long> total_wait_ms_;
  std::atomic<long long> started_tasks_;
};

int main() {
  using namespace std::chrono_literals;

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  TaskScheduler sched(6, 3);

  std::mt19937 rng(123);
  std::uniform_int_distribution<int> slots(1, 3);
  std::uniform_int_distribution<int> dur(150, 450);
  std::uniform_int_distribution<int> pr(1, 10);

  for (int i = 1; i <= 15; ++i) {
    Task t;
    t.id = i;
    t.required_slots = slots(rng);
    t.duration_ms = dur(rng);
    t.priority = pr(rng);
    sched.submit(t);
    std::this_thread::sleep_for(60ms);
    std::this_thread::yield();
  }

  std::this_thread::sleep_for(6s);
  sched.stop();
  std::this_thread::sleep_for(300ms);
  std::cout << "завершено=" << sched.completed_tasks() << std::endl;
  std::cout << "среднее_ожидание_мс=" << sched.average_wait_ms() << std::endl;
  return 0;
}

