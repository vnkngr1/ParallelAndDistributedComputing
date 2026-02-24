//============================================================================
// Name        : Laba1Task1.cpp
// Author      : Garshin Ivan
// Description : Параллельный поиск простых чисел с Boost.Thread
//- Реализовать многопоточный алгоритм поиска простых чисел в диапазоне [1, N], используя K потоков.
//- Каждому потоку передаётся свой поддиапазон, который он проверяет на простые числа.
//- Главный поток собирает результаты и выводит общее количество найденных простых чисел.
//- Замерить и сравнить время выполнения программы в однопоточном и многопоточном режимах.
//============================================================================

#include <boost/thread.hpp>
#include <iostream>

// Функция-проверка простоты числа
bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

// Функция для еденичного потока по поиску простых чисел в заданном диапазоне
void findPrimesInRange(int start, int end, std::vector<int>& primes, boost::mutex& mutex) {
    std::vector<int> localPrimes;

    for (int i = start; i <= end; i++) {
        if (isPrime(i)) {
            localPrimes.push_back(i);
        }
    }

    // Блокируем мьютекс для добавления результатов в общий вектор
    boost::lock_guard<boost::mutex> lock(mutex);
    primes.insert(primes.end(), localPrimes.begin(), localPrimes.end());
}

// Функция поиска для одного потока
void singleThreaded(int N) {

	std::vector<int> primes;
	auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= N; ++i) {
        if (isPrime(i)) {
            primes.push_back(i);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	std::cout << "Одинопоточный режим" << std::endl;
	std::cout << "Найдено простых чисел: " << primes.size() << std::endl;
	std::cout << "Время выполнения: " << duration.count() << " мс" << std::endl;

}

// Функция поиска многопоточная
void multipleThreaded(int N, int K) {
	std::vector<int> primes;
	boost::thread_group threads;
	boost::mutex mutex;

	int rangeSize = N / K;
	int remainder = N % K;

	auto startTime = std::chrono::high_resolution_clock::now();

	int currentStart = 1;
	for (int i = 0; i < K; ++i) {
		int currentEnd = currentStart + rangeSize - 1;

		// Опустошаем остаток по потокам
		if (remainder > 0) {
			currentEnd++;
			remainder--;
		}

		threads.create_thread(boost::bind(findPrimesInRange,
										 currentStart,
										 currentEnd,
										 boost::ref(primes),
										 boost::ref(mutex)));

		currentStart = currentEnd + 1;
	}

	threads.join_all();

	auto endTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	std::cout << "Многопоточный режим. Количество потоков: " << K << std::endl;
	std::cout << "Найдено простых чисел: " << primes.size() << std::endl;
	std::cout << "Время выполнения: " << duration.count() << " мс" << std::endl;
}

int main() {

	int N = 1000000; // Верхний порог диапозона

	// Многопоточный режим
	for (int K: {2, 4, 8}) {
		multipleThreaded(N, K);
	}

	// Однопоточный режим
	singleThreaded(N);

	return 0;
}
