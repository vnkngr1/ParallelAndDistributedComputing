#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>

template <typename T>
class Logger {
private:
    std::ofstream log_file;
    std::mutex mtx;

public:
    Logger(const std::string& filename) {
        log_file.open(filename, std::ios::app);
    }

    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    inline void write_impl(const std::string& message) {
        std::lock_guard<std::mutex> lock(mtx);

        std::cout << "Thread " << std::this_thread::get_id()
                  << ": " << message << std::endl;

        if (log_file.is_open()) {
            log_file << "Thread " << std::this_thread::get_id()
                     << ": " << message << std::endl;
        }
    }

    void log(const T& message) {
        std::string msg_str = to_string_helper(message);
        write_impl(msg_str);
    }

private:
    template <typename U>
    std::string to_string_helper(const U& val) {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }

    std::string to_string_helper(const std::string& val) {
        return val;
    }

    std::string to_string_helper(const char* val) {
        return std::string(val);
    }
};

struct Person {
    std::string name;
    int age;

    friend std::ostream& operator<<(std::ostream& os, const Person& p) {
        os << "Person{name=" << p.name << ", age=" << p.age << "}";
        return os;
    }
};

int main() {
    Logger<std::string> logger("log.txt");

    std::thread t1([&logger]() {
        logger.log("Message from thread 1");
    });

    std::thread t2([&logger]() {
        logger.log("Message from thread 2");
    });

    std::thread t3([&logger]() {
        logger.log(std::string("Message from thread 3"));
    });

    t1.detach();
    t2.detach();
    t3.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Logger<int> int_logger("log_int.txt");
    int_logger.log(42);
    int_logger.log(100);

    Logger<Person> person_logger("log_person.txt");
    Person p{"Alice", 30};
    person_logger.log(p);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return 0;
}
