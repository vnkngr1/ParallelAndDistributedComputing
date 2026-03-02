#include <iostream>
#include <vector>

class VirtualThread {
private:
    std::vector<int> numbers = {5, 7, 10, 12};
    int currentIndex = 0;
    int id;

    long long calculateFactorial(int n) {
        long long res = 1;
        for (int i = 1; i <= n; ++i) res *= i;
        return res;
    }

public:
    VirtualThread(int threadId) : id(threadId) {}

    bool hasTasks() {
        return currentIndex < numbers.size();
    }

    void run() {
        if (hasTasks()) {
            int n = numbers[currentIndex];
            std::cout << "Виртуальный поток " << id << " вычисляет " << n << "! = " << calculateFactorial(n) << std::endl;
            currentIndex++;
        }
    }
};

class HyperThreadingSimulator {
private:
    VirtualThread vt1;
    VirtualThread vt2;

public:
    HyperThreadingSimulator() : vt1(1), vt2(2) {}

    void execute() {
        while (vt1.hasTasks() || vt2.hasTasks()) {
            if (vt1.hasTasks()) vt1.run();
            if (vt2.hasTasks()) vt2.run();
        }
    }
};

int main() {
    HyperThreadingSimulator simulator;
    simulator.execute();
    return 0;
}
