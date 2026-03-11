#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

template <typename Key, typename Value>
class Cache {
private:
    std::map<Key, Value> data;
    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    inline void set(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mtx);

        bool is_new = data.find(key) == data.end();
        data[key] = value;

        std::cout << "Thread " << std::this_thread::get_id()
                  << ": SET [" << key << "] = " << value << std::endl;

        if (is_new) {
            cv.notify_all();
        }
    }

    inline Value get(const Key& key) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this, &key]() {
            return data.find(key) != data.end();
        });

        Value result = data[key];

        std::cout << "Thread " << std::this_thread::get_id()
                  << ": GET [" << key << "] = " << result << std::endl;

        return result;
    }

    void print_all() const {
        std::lock_guard<std::mutex> lock(mtx);

        std::cout << "Cache contents:" << std::endl;
        for (const auto& [k, v] : data) {
            std::cout << "  [" << k << "] = " << v << std::endl;
        }
    }
};

int main() {
    Cache<std::string, int> cache;

    std::thread t1([&cache]() {
        std::this_thread::yield();
        cache.set("key1", 100);
        cache.set("key2", 200);
    });

    std::thread t2([&cache]() {
        int val = cache.get("key1");
        (void)val;
    });

    std::thread t3([&cache]() {
        std::this_thread::yield();
        cache.set("key3", 300);
    });

    std::thread t4([&cache]() {
        int val = cache.get("key2");
        (void)val;
    });

    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    cache.print_all();

    return 0;
}
