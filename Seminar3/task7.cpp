#include <iostream>
#include <vector>
#include <string>

class VirtualThread {
private:
    std::vector<std::string> tasks = {"Задача А", "Задача В", "Задача С", "Задача D"};
    size_t currentTaskIndex = 0;
    int id;

public:
    VirtualThread(int threadId) : id(threadId) {}

    bool hasTasks() const {
        return currentTaskIndex < tasks.size();
    }

    void runNextTask() {
        if (hasTasks()) {
            std::cout << "Виртуальный поток " << id << " начал " << tasks[currentTaskIndex] << std::endl;
            std::cout << "Виртуальный поток " << id << " закончил " << tasks[currentTaskIndex] << std::endl;
            currentTaskIndex++;
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
            if (vt1.hasTasks()) vt1.runNextTask();
            if (vt2.hasTasks()) vt2.runNextTask();
        }
    }
};

int main() {
    HyperThreadingSimulator simulator;
    simulator.execute();
    return 0;
}
