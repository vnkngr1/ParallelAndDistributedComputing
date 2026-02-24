#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>

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

std::vector<std::pair<int, double>> convert(
    int N, int M, const std::vector<std::vector<double>>& grades) {

    std::vector<std::pair<int, double>> result;

    for (int i = 0; i < N; i++) {
        double sum = 0;
        for (double grade : grades[i]) {
            sum += grade;
        }
        double average = sum / M;

        result.push_back({i, average});
    }

    return result;
}

void sortStudentsByAverage(std::vector<std::pair<int, double>>& students) {
	std::sort(students.begin(), students.end(),
		[](const std::pair<int, double>& a, const std::pair<int, double>& b) {
			if (a.second != b.second) {
				return a.second > b.second;
			}
			return a.first < b.first;
		});
}

void printSortedStudents(const std::vector<std::pair<int, double>>& students) {
    std::cout << "Отсортированный список студентов" << std::endl;
    std::cout << std::fixed << std::setprecision(2);

    for (const auto& [index, average] : students) {
        std::cout << "Студент " << index + 1 << ": средний балл = "
                  << average << std::endl;
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

	std::vector<std::pair<int,double>> studentAverages = convert(N, M, grades);

	sortStudentsByAverage(studentAverages);

	printSortedStudents(studentAverages);

	grades.clear();
	studentAverages.clear();

}
