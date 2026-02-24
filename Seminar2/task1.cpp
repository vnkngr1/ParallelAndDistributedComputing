#include <iostream>

double ArithmeticMean(int N, double* grades) {

	double sum = 0;
	for (int i = 0; i < N; i++) {
		sum += grades[i];
	}
	return sum / N;

}

double max(int N, double* grades) {

	double result = 0;
	for (int i = 0; i < N; i++) {
		if (grades[i] > result) {
			result = grades[i];
		}
	}
	return result;

}

double min(int N, double* grades) {

	double result = 6;
	for (int i = 0; i < N; i++) {
		if (grades[i] < result) {
			result = grades[i];
		}
	}
	return result;

}

int count(int N, double* grades, double goal) {

	int result = 0;
	for (int i = 0; i < N; i++) {
		if (grades[i] > goal) {
			result++;
		}
	}
	return result;

}

int main() {

    int N;

    while (true) {
		std:: cout << "Введите количество студентов: ";
		std::cin >> N;
		if (N <= 0) {
			std::cout << "Введено неверное количество студентов" << std::endl;
		}
		else {
			break;
		}
    }

	double* grades = new double[N];

	for (int i = 0; i < N; i++) {
		while (true) {
			std:: cout << "Введите средний балл студента " << (i + 1) << " - ";
			std::cin >> grades[i];
			if (grades[i] < 0 || grades[i] > 5) {
				std::cout << "Введено неверный средний балл студента" << std::endl;
			}
			else {
				break;
			}
		}
	}

	std::cout << "Среднее арифмитическое - " << ArithmeticMean(N, grades) << std::endl;
	std::cout << "Максимальная оценка - " <<  max(N, grades) << std::endl;
	std::cout << "Минимальная оцекна - " << min(N, grades) << std::endl;

	double goal = 4.0;
	std::cout << "Количество оценок выше " << goal << " - " << count(N, grades, goal) << std::endl;

	delete[] grades;
	grades = nullptr;

}
