#include <iostream>
#include <vector>

void ArithmeticMeanStudents(int N, int M, const std::vector<std::vector<double>>& grades) {

	for (int i = 0; i < N; i++) {
		std::cout << "Средний балл студента " << (i + 1) << " - ";
		double sum = 0;
		for (int j = 0; j < M; j++) {
			sum += grades[i][j];
		}
		std::cout << (sum / M) << std::endl;
	}

}

void ArithmeticMeanSubject(int N, int M, const std::vector<std::vector<double>>& grades) {

	for (int j = 0; j < M; j++) {
		std::cout << "Средний балл по предмету " << (j + 1) << " - ";
		double sum = 0;
		for (int i = 0; i < N; i++) {
			sum += grades[i][j];
		}
		std::cout << (sum / N) << std::endl;
	}

}

int max(int N, int M, const std::vector<std::vector<double>>& grades) {

	double max_mean = 0;
	int result = 0;

	for (int i = 0; i < N; i++) {

		double sum = 0;
		for (int j = 0; j < M; j++) {
			sum += grades[i][j];
		}

		if ((sum / M) > max_mean) {
			result = i;
			max_mean = sum / M;
		}

	}
	return result;

}

int read(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (value > 0) {
            return value;
        }
        std::cout << "Введено неверное значение" << std::endl;
    }
}

int main() {

	const int N = read("Введите количество студентов: ");
	const int M = read("Введите количество предметов: ");

    std::vector<std::vector<double>> grades;

	for (int i = 0; i < N; i++) {
		std::cout << "Ввод оценок студента " << (i + 1) << std::endl;
		std::vector<double> temp;
		for (int j = 0; j < M; j++) {
			double grade;
			while (true) {
				std:: cout << "Введите оценку за предмет " << (j + 1) << " - ";
				std::cin >> grade;
				if (grade < 0 || grade > 5) {
					std::cout << "Введена неверная оценка" << std::endl;
				}
				else {
					temp.push_back(grade);
					break;
				}
			}
		}
		grades.push_back(temp);
	}

	ArithmeticMeanStudents(N, M, grades);
	std::cout << "Студент с максимальный средним баллом - " << (max(N, M, grades) + 1) << std::endl;
	ArithmeticMeanSubject(N, M, grades);

	grades.clear();

}
