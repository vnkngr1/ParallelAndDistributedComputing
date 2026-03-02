#include <iostream>
#include <chrono>

int main() {
    long long input_seconds;
    std::cout << "Введите количество секунд: ";
    std::cin >> input_seconds;

    std::chrono::seconds total_sec(input_seconds);

    auto h = std::chrono::duration_cast<std::chrono::hours>(total_sec);
    total_sec -= h;
    auto m = std::chrono::duration_cast<std::chrono::minutes>(total_sec);
    total_sec -= m;
    auto s = total_sec;

    std::cout << h.count() << " час " << m.count() << " минута " << s.count() << " секунда" << std::endl;

    return 0;
}
