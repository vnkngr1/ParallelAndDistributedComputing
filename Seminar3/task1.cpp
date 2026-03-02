#include <iostream>
#include <chrono>

long long calculateSum(int n) {
    long long sum = 0;
    for (int i = 1; i <= n; ++i) {
        sum += i;
    }
    return sum;
}

int main() {
    int n = 10000000;

    auto start = std::chrono::high_resolution_clock::now();

    long long result = calculateSum(n);

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Сумма чисел от 1 до " << n << " равна: " << result << std::endl;
    std::cout << "Время выполнения: " << duration.count() << " миллисекунды" << std::endl;

    return 0;
}
