#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main() {
    int n;

    if (!(std::cin >> n)) {
        return 1;
    }

    while (n > 0) {
        std::cout << "Осталось: " << n << " секунд" << std::endl;
        std::this_thread::sleep_for(1s);
        n--;
    }

    std::cout << "Время вышло!" << std::endl;

    return 0;
}
