#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <thread>
#include <random>

struct Task {
    int value;
    int priority;
    int duration_ms;
    int steps;
    int currentStep = 0;

    int getResult() const {
        return value * value;
    }
};

class VirtualThread {
private:
    std::vector<Task> tasks;
    int id;

public:
    VirtualThread(int threadId) : id(threadId) {}

    void addTask(const Task& t) {
        tasks.push_back(t);
    }

    bool hasTasks() const {
        return !tasks.empty();
    }

    void runStep() {
        if (tasks.empty()) return;

        auto it = std::max_element(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
            return a.priority < b.priority;
        });

        it->currentStep++;
        std::cout << "Виртуальный поток " << id << " выполняет шаг " << it->currentStep
                  << "/" << it->steps << " задачи " << it->value
                  << " с приоритетом " << it->priority << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(it->duration_ms / it->steps));

        if (it->currentStep >= it->steps) {
            std::cout << "Виртуальный поток " << id << " завершил задачу " << it->value
                      << ": результат = " << it->getResult() << std::endl;
            tasks.erase(it);
        }
    }
};

class HyperThreadingSimulator {
private:
    VirtualThread vt1;
    VirtualThread vt2;

public:
    HyperThreadingSimulator() : vt1(1), vt2(2) {}

    void setupRandomTasks() {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> distVal(1, 50);
        std::uniform_int_distribution<> distDur(200, 1000);
        std::uniform_int_distribution<> distPri(1, 10);
        std::uniform_int_distribution<> distStep(2, 5);

        for (int i = 0; i < 3; ++i) {
            vt1.addTask({distVal(gen), distPri(gen), distDur(gen), distStep(gen)});
            vt2.addTask({distVal(gen), distPri(gen), distDur(gen), distStep(gen)});
        }
    }

    void execute() {
        while (vt1.hasTasks() || vt2.hasTasks()) {
            if (vt1.hasTasks()) vt1.runStep();
            if (vt2.hasTasks()) vt2.runStep();
        }
    }
};

int main() {
    HyperThreadingSimulator simulator;
    simulator.setupRandomTasks();
    simulator.execute();
    return 0;
}
