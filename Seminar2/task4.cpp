#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <numeric>

class StudentGrades {
private:
    std::vector<std::vector<double>> grades;

public:
    StudentGrades(const std::vector<std::vector<double>>& data) : grades(data) {}

    double calculateAverage(int studentIndex) const {
        if (grades[studentIndex].empty()) return 0.0;
        double sum = std::accumulate(grades[studentIndex].begin(),
                                      grades[studentIndex].end(), 0.0);
        return sum / grades[studentIndex].size();
    }

    std::vector<std::pair<int, double>> getStudentAverages() const {
        std::vector<std::pair<int, double>> result;
        result.reserve(grades.size());

        for (int i = 0; i < grades.size(); i++) {
            result.emplace_back(i, calculateAverage(i));
        }
        return result;
    }

    void filterByThreshold(double threshold) {
        auto studentData = getStudentAverages();

        auto newEnd = std::remove_if(grades.begin(), grades.end(),
            [this, threshold, index = 0](const auto&) mutable {
                bool shouldRemove = calculateAverage(index++) < threshold;
                return shouldRemove;
            });

        grades.erase(newEnd, grades.end());

        std::cout << "Удалены студенты со средним баллом ниже " << threshold << std::endl;
    }

    void calculateStatistics() const {
        int excellent = 0;      // отличники
        int atRisk = 0;         // близкие к отчислению

        for (int i = 0; i < grades.size(); i++) {
            double avg = calculateAverage(i);
            if (avg >= 4.5) excellent++;
            if (avg < 3.0) atRisk++;
        }

        std::cout << "Статистика" << std::endl;
        std::cout << "Количество отличников (≥ 4.5): " << excellent << std::endl;
        std::cout << "Количество студентов близких к отчислению (< 3.0): " << atRisk << std::endl;
    }

    void printAllStudents() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Список студентов:" << std::endl;

        for (int i = 0; i < grades.size(); i++) {
            std::cout << "Студент " << i + 1 << ": средний балл - "
                      << calculateAverage(i) << std::endl;
        }
    }
};

int main() {
    std::vector<std::vector<double>> grades = {
        {4.5, 4.8, 5.0, 4.7},
        {3.5, 4.0, 3.8, 4.2},
        {2.5, 3.0, 2.8, 3.2},
        {4.0, 4.5, 4.0, 4.5},
        {5.0, 5.0, 4.9, 5.0},
        {2.0, 2.5, 3.0, 2.5},
        {3.8, 4.0, 3.9, 4.1},
        {2.8, 2.9, 3.1, 2.7}
    };

    StudentGrades studentGrades(grades);

    std::cout << "Исходные данные" << std::endl;
    studentGrades.printAllStudents();
    studentGrades.calculateStatistics();

    std::cout << "После фильтрации" << std::endl;
    studentGrades.filterByThreshold(3.5);
    studentGrades.printAllStudents();
    studentGrades.calculateStatistics();

    return 0;
}
