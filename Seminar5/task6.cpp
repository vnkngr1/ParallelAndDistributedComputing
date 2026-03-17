#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <unordered_map>
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

struct FileChunk {
  int chunk_id{};
  int file_id{};
  size_t size{};

  void download() const {
    std::this_thread::sleep_for(std::chrono::milliseconds(40 + static_cast<int>(size % 80)));
  }
};

struct FileDownloadState {
  int file_id{};
  int total_chunks{};
  std::atomic<int> downloaded_chunks{0};
  std::atomic<bool> active{false};
  std::mutex mtx;

  bool is_complete() const { return downloaded_chunks.load() >= total_chunks; }
  void mark_chunk_downloaded() { downloaded_chunks.fetch_add(1); }
};

class DownloadManager {
 public:
  DownloadManager(int max_active_files, int max_active_chunks)
      : active_downloads_(max_active_files),
        chunk_downloads_(max_active_chunks),
        completed_files_(0),
        stop_(false) {}

  void add_file(int file_id, int chunks_count) {
    auto st = std::make_shared<FileDownloadState>();
    st->file_id = file_id;
    st->total_chunks = chunks_count;
    {
      std::lock_guard<std::mutex> lk(state_mtx_);
      states_[file_id] = st;
    }

    std::lock_guard<std::mutex> lk(queue_mutex_);
    for (int i = 0; i < chunks_count; ++i) {
      FileChunk c;
      c.file_id = file_id;
      c.chunk_id = i;
      c.size = static_cast<size_t>(100 + i * 13);
      queue_.push(std::move(c));
    }
    queue_cv_.notify_all();
  }

  void start_workers(int n) {
    for (int i = 0; i < n; ++i) std::thread([&] { download_worker(); }).detach();
  }

  void stop() {
    stop_.store(true);
    queue_cv_.notify_all();
  }

  int completed_files() const { return completed_files_.load(); }

 private:
  inline void process_chunk(const FileChunk& chunk) { chunk.download(); }

  std::shared_ptr<FileDownloadState> get_state(int file_id) {
    std::lock_guard<std::mutex> lk(state_mtx_);
    auto it = states_.find(file_id);
    if (it == states_.end()) return {};
    return it->second;
  }

  void ensure_file_active(const std::shared_ptr<FileDownloadState>& st) {
    if (!st) return;
    std::unique_lock<std::mutex> lk(st->mtx);
    if (st->active.load()) return;
    lk.unlock();
    active_downloads_.acquire();
    lk.lock();
    if (!st->active.load()) {
      st->active.store(true);
      {
        std::lock_guard<std::mutex> clk(console_mtx_);
        std::cout << std::this_thread::get_id() << " файл=" << st->file_id << " статус=файл_активен"
                  << std::endl;
      }
    } else {
      active_downloads_.release();
    }
  }

  void maybe_file_complete(const std::shared_ptr<FileDownloadState>& st) {
    if (!st) return;
    if (!st->is_complete()) return;
    std::unique_lock<std::mutex> lk(st->mtx);
    if (!st->active.load()) return;
    st->active.store(false);
    lk.unlock();
    active_downloads_.release();
    completed_files_.fetch_add(1);
    {
      std::lock_guard<std::mutex> clk(console_mtx_);
      std::cout << std::this_thread::get_id() << " файл=" << st->file_id << " статус=файл_завершен"
                << std::endl;
    }
  }

  void download_worker() {
    while (!stop_.load()) {
      FileChunk chunk;
      {
        std::unique_lock<std::mutex> lk(queue_mutex_);
        queue_cv_.wait(lk, [&] { return stop_.load() || !queue_.empty(); });
        if (stop_.load()) return;
        chunk = std::move(queue_.front());
        queue_.pop();
      }

      auto st = get_state(chunk.file_id);
      ensure_file_active(st);

      chunk_downloads_.acquire();
      {
        std::lock_guard<std::mutex> clk(console_mtx_);
        std::cout << std::this_thread::get_id() << " файл=" << chunk.file_id << " часть=" << chunk.chunk_id
                  << " статус=скачивание" << std::endl;
      }

      process_chunk(chunk);

      chunk_downloads_.release();

      if (st) st->mark_chunk_downloaded();

      {
        std::lock_guard<std::mutex> clk(console_mtx_);
        std::cout << std::this_thread::get_id() << " файл=" << chunk.file_id << " часть=" << chunk.chunk_id
                  << " статус=готово" << std::endl;
      }

      maybe_file_complete(st);
      std::this_thread::yield();
    }
  }

  std::queue<FileChunk> queue_;
  CountingSemaphore active_downloads_;
  CountingSemaphore chunk_downloads_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;

  std::mutex state_mtx_;
  std::unordered_map<int, std::shared_ptr<FileDownloadState>> states_;

  std::mutex console_mtx_;
  std::atomic<int> completed_files_;
  std::atomic<bool> stop_;
};

int main() {
  using namespace std::chrono_literals;

#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  DownloadManager dm(2, 3);
  dm.start_workers(5);

  dm.add_file(1, 6);
  dm.add_file(2, 5);
  dm.add_file(3, 7);

  std::this_thread::sleep_for(6s);
  dm.stop();
  std::this_thread::sleep_for(300ms);
  std::cout << "завершено_файлов=" << dm.completed_files() << std::endl;
  return 0;
}

