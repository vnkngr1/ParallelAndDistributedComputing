#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

class TaskTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;

public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        end_time = std::chrono::high_resolution_clock::now();
    }

    long long getDuration() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    }

    void measureSorting(std::vector<int>& vec) {
        start();
        std::sort(vec.begin(), vec.end());
        stop();
        std::cout << "Время выполнения сортировки: " << getDuration() << " миллисекунд" << std::endl;
    }
};

int main() {
    TaskTimer timer;
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(1, 1000000);

    for (int i = 0; i < 2; ++i) {
        std::vector<int> data(100000);
        for (int& x : data) x = dis(gen);

        timer.measureSorting(data);
    }

    return 0;
}
