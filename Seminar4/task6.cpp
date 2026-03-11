#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

template <typename T>
class MatrixProcessor {
private:
    std::vector<std::vector<T>> matrix;
    size_t n_threads;
    mutable std::mutex stats_mutex;
    std::condition_variable cv;
    std::atomic<size_t> completed_threads{0};
    size_t processed_elements = 0;
    mutable std::mutex print_mutex;

public:
    MatrixProcessor(std::vector<std::vector<T>> matrix, size_t n_threads)
        : matrix(std::move(matrix)), n_threads(n_threads) {}

    void apply(std::function<T(T)> func) {
        size_t rows = matrix.size();
        if (rows == 0) return;

        size_t cols = matrix[0].size();
        size_t total_elements = rows * cols;
        size_t elements_per_thread = total_elements / n_threads;
        size_t remainder = total_elements % n_threads;

        for (size_t t = 0; t < n_threads; ++t) {
            size_t start = t * elements_per_thread + std::min(t, remainder);
            size_t end = start + elements_per_thread + (t < remainder ? 1 : 0);

            std::thread([this, start, end, func, t]() {
                {
                    std::lock_guard<std::mutex> lock(print_mutex);
                    std::cout << "Thread " << std::this_thread::get_id()
                              << " started segment [" << start << ", " << end << ")" << std::endl;
                }

                size_t cols = matrix[0].size();
                size_t local_count = 0;

                for (size_t idx = start; idx < end; ++idx) {
                    size_t row = idx / cols;
                    size_t col = idx % cols;

                    matrix[row][col] = func(matrix[row][col]);
                    ++local_count;

                    if (idx % 10 == 0) {
                        std::this_thread::yield();
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(stats_mutex);
                    processed_elements += local_count;
                }

                {
                    std::lock_guard<std::mutex> lock(print_mutex);
                    std::cout << "Thread " << std::this_thread::get_id()
                              << " finished segment, processed " << local_count << " elements" << std::endl;
                }

                size_t completed = ++completed_threads;
                if (completed == n_threads) {
                    cv.notify_one();
                }
            }).detach();
        }

        std::unique_lock<std::mutex> lock(stats_mutex);
        cv.wait(lock, [this]() {
            return completed_threads.load() == n_threads;
        });
    }

    void print_matrix() const {
        std::lock_guard<std::mutex> lock(print_mutex);
        for (const auto& row : matrix) {
            for (const auto& elem : row) {
                std::cout << elem << " ";
            }
            std::cout << std::endl;
        }
    }

    size_t get_processed_count() const {
        std::lock_guard<std::mutex> lock(stats_mutex);
        return processed_elements;
    }
};

int main() {
    std::vector<std::vector<int>> data = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16}
    };

    MatrixProcessor<int> processor(data, 2);

    processor.apply([](int x) { return x * x; });

    std::cout << "Processed elements: " << processor.get_processed_count() << std::endl;
    processor.print_matrix();

    return 0;
}
