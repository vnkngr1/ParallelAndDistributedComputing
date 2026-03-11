#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename T>
class Buffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    std::mutex mtx;
    std::condition_variable cv_produce;
    std::condition_variable cv_consume;
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t count = 0;
    std::atomic<bool> done{false};

public:
    Buffer(size_t capacity) : capacity(capacity) {
        buffer.resize(capacity);
    }

    void produce(T value) {
        std::unique_lock<std::mutex> lock(mtx);

        cv_produce.wait(lock, [this]() {
            return count < capacity;
        });

        buffer[write_pos] = value;
        write_pos = (write_pos + 1) % capacity;
        ++count;

        std::cout << "Thread " << std::this_thread::get_id()
                  << ": Produced " << value << " (count=" << count << ")" << std::endl;

        cv_consume.notify_one();
    }

    T consume() {
        std::unique_lock<std::mutex> lock(mtx);

        cv_consume.wait(lock, [this]() {
            return count > 0 || done.load();
        });

        if (count == 0) return T{};

        T value = buffer[read_pos];
        read_pos = (read_pos + 1) % capacity;
        --count;

        std::cout << "Thread " << std::this_thread::get_id()
                  << ": Consumed " << value << " (count=" << count << ")" << std::endl;

        cv_produce.notify_one();

        return value;
    }

    void set_done() {
        done = true;
        cv_consume.notify_all();
    }
};

int main() {
    Buffer<int> buffer(5);

    std::thread producer1([&buffer]() {
        for (int i = 0; i < 5; ++i) {
            buffer.produce(i * 10);
            if (i % 2 == 0) std::this_thread::yield();
        }
    });

    std::thread producer2([&buffer]() {
        for (int i = 5; i < 10; ++i) {
            buffer.produce(i * 10);
            if (i % 2 == 0) std::this_thread::yield();
        }
    });

    std::thread consumer1([&buffer]() {
        for (int i = 0; i < 5; ++i) {
            buffer.consume();
            if (i % 3 == 0) std::this_thread::yield();
        }
    });

    std::thread consumer2([&buffer]() {
        for (int i = 0; i < 5; ++i) {
            buffer.consume();
            if (i % 3 == 0) std::this_thread::yield();
        }
    });

    producer1.join();
    producer2.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    buffer.set_done();

    consumer1.join();
    consumer2.join();

    return 0;
}
