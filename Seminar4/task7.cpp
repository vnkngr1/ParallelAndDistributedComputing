#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

template <typename T>
class PriorityQueue {
private:
    struct Item {
        T value;
        int priority;

        bool operator<(const Item& other) const {
            return priority < other.priority;
        }
    };

    std::priority_queue<Item> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{false};
    mutable std::mutex print_mutex;

public:
    void push(T value, int priority) {
        std::lock_guard<std::mutex> lock(mtx);

        queue.push({value, priority});

        std::lock_guard<std::mutex> plock(print_mutex);
        std::cout << "Thread " << std::this_thread::get_id()
                  << ": Pushed value=" << value << " priority=" << priority
                  << " (queue size=" << queue.size() << ")" << std::endl;

        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this]() {
            return !queue.empty() || done.load();
        });

        if (queue.empty()) return T{};

        Item item = queue.top();
        queue.pop();

        std::lock_guard<std::mutex> plock(print_mutex);
        std::cout << "Thread " << std::this_thread::get_id()
                  << ": Popped value=" << item.value << " priority=" << item.priority
                  << " (queue size=" << queue.size() << ")" << std::endl;

        return item.value;
    }

    void set_done() {
        done = true;
        cv.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
};

int main() {
    PriorityQueue<std::string> pq;

    std::thread producer1([&pq]() {
        pq.push("Low priority task", 1);
        std::this_thread::yield();
        pq.push("High priority task", 10);
    });

    std::thread producer2([&pq]() {
        pq.push("Medium priority task", 5);
        std::this_thread::yield();
        pq.push("Critical task", 100);
    });

    std::thread consumer1([&pq]() {
        std::this_thread::yield();
        pq.pop();
        pq.pop();
    });

    std::thread consumer2([&pq]() {
        std::this_thread::yield();
        pq.pop();
        pq.pop();
    });

    producer1.join();
    producer2.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pq.set_done();

    consumer1.join();
    consumer2.join();

    return 0;
}
