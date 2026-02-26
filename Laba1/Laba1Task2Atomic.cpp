.cp//============================================================================
// Name        : Laba1Task2Atomic.cpp
// Author      : Garshin Ivan
// Description : Конкурентное накопление суммы массива с std::atomic<long long>
//- Написать программу, которая вычисляет сумму элементов большого массива (10^7 элементов).
//- Разделить вычисление на K потоков.
//============================================================================

#include <boost/thread.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

// Функция для потока по сумме
void sumInRange(int start, int end, std::atomic<long long>& result, const std::vector<int>& nums) {
    int tempSumInRange = 0;

    for (int i = start; i <= end; i++) {
        tempSumInRange += nums[i];
    }

    result += tempSumInRange;
}

// Функция суммы многопоточная
void multipleThreaded(int K, const std::vector<int>& nums) {
    boost::thread_group threads;

    std::atomic<long long> result(0);

    int rangeSize = nums.size() / K;
    int remainder = nums.size() % K;

    auto startTime = std::chrono::high_resolution_clock::now();

    int currentStart = 0;
    for (int i = 0; i < K; ++i) {
        int currentEnd = currentStart + rangeSize - 1;

        // Распределяем остаток по первым потокам
        if (remainder > 0) {
        	currentEnd++;
        	remainder--;
        }

        threads.create_thread(boost::bind(sumInRange,
                                         currentStart,
                                         currentEnd,
                                         boost::ref(result),
                                         boost::cref(nums)));

        currentStart = currentEnd + 1;
    }

    threads.join_all();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Многопоточный режим. Количество потоков: " << K << std::endl;
    std::cout << "Сумма массива: " << result << std::endl;
    std::cout << "Время выполнения: " << duration.count() << " мс" << std::endl;
}

// Генератор вектора с случайными числами
void vectorGenerator(int size, std::vector<int>& vec) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, 1000000); // Диапазон [0, 999]

    for (int i = 0; i < size; i++) {
        vec[i] = dist(gen);
    }
}

int main() {
    int N = 10000000; // Количество чисел в массиве
    std::vector<int> nums(N);

    vectorGenerator(N, nums);

    // Однопоточная проверка для верификации
    int expectedSum = 0;
    for (int num : nums) {
        expectedSum += num;
    }
    std::cout << "Ожидаемая сумма: " << expectedSum << std::endl;
    std::cout << std::endl;

    // Количество потоков
    for (int K: {1, 2, 4, 8}) {
        multipleThreaded(K, nums);
    }

    return 0;
}
