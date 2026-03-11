#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename T>
class ParallelSum {
private:
    std::vector<T> data;
    size_t n_threads;
    T total_sum;
    std::mutex sum_mutex;
    std::condition_variable cv;
    std::atomic<size_t> completed_threads{0};

    inline T sum_segment(const std::vector<T>& segment) {
        T local_sum = 0;
        for (const auto& elem : segment) {
            local_sum += elem;
        }
        return local_sum;
    }

public:
    ParallelSum(const std::vector<T>& data, size_t n_threads)
        : data(data), n_threads(n_threads), total_sum(0) {}

    T compute_sum() {
        size_t data_size = data.size();
        size_t segment_size = data_size / n_threads;
        size_t remainder = data_size % n_threads;

        for (size_t i = 0; i < n_threads; ++i) {
            size_t start = i * segment_size + std::min(i, remainder);
            size_t end = start + segment_size + (i < remainder ? 1 : 0);

            std::thread([this, start, end, i]() {
                T local_sum = 0;
                for (size_t j = start; j < end; ++j) {
                    local_sum += data[j];

                    if (j % 100 == 0) {
                        std::this_thread::yield();
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(sum_mutex);
                    total_sum += local_sum;
                }

                size_t completed = ++completed_threads;

                if (completed == n_threads) {
                    cv.notify_one();
                }
            }).detach();
        }

        std::unique_lock<std::mutex> lock(sum_mutex);
        cv.wait(lock, [this]() {
            return completed_threads.load() == n_threads;
        });

        return total_sum;
    }
};

int main() {
    std::vector<int> int_data(1000);
    for (int i = 0; i < 1000; ++i) {
        int_data[i] = i + 1;
    }

    ParallelSum<int> ps_int(int_data, 4);
    std::cout << "Int sum: " << ps_int.compute_sum() << std::endl;
    // Ожидается: 500500

    std::vector<double> double_data = {1.5, 2.5, 3.5, 4.5, 5.5};
    ParallelSum<double> ps_double(double_data, 2);
    std::cout << "Double sum: " << ps_double.compute_sum() << std::endl;
    // Ожидается: 17.5

    return 0;
}
