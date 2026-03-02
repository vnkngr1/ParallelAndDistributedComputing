#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

void bubbleSort(std::vector<int>& arr) {
    int n = arr.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) std::swap(arr[j], arr[j + 1]);
        }
    }
}

void insertionSort(std::vector<int>& arr) {
    int n = arr.size();
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

void measureTime(std::string name, std::vector<int> arr, void (*sortFunc)(std::vector<int>&)) {
    auto start = std::chrono::high_resolution_clock::now();
    sortFunc(arr);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << name << ": " << duration.count() << " ms" << std::endl;
}

int main() {
    const int N = 10000;
    std::vector<int> original(N);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100000);

    for (int& x : original) x = dis(gen);

    std::cout << "Сравнение алгоритмов (N = " << N << "):" << std::endl;

    auto start_std = std::chrono::high_resolution_clock::now();
    std::vector<int> std_arr = original;
    std::sort(std_arr.begin(), std_arr.end());
    auto end_std = std::chrono::high_resolution_clock::now();
    std::cout << "std::sort: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_std - start_std).count() << " ms" << std::endl;

    measureTime("Bubble Sort", original, bubbleSort);
    measureTime("Insertion Sort", original, insertionSort);

    return 0;
}
