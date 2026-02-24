#include "RecordBook.hpp"
#include <iostream>
#include <numeric>
#include <iomanip>

RecordBook::RecordBook() : recordNumber("000000") {}

RecordBook::RecordBook(const std::string& number) : recordNumber(number) {}

void RecordBook::addGrade(double grade) {
    grades.push_back(grade);
}

double RecordBook::getAverage() const {
    if (grades.empty()) return 0.0;
    double sum = std::accumulate(grades.begin(), grades.end(), 0.0);
    return sum / grades.size();
}

void RecordBook::print() const {
    std::cout << "Зачётная книжка №" << recordNumber << std::endl;
    std::cout << "Оценки: ";
    for (double g : grades) {
        std::cout << g << " ";
    }
    std::cout << std::endl;
    std::cout << "Средний балл: " << std::fixed << std::setprecision(2) << getAverage() << std::endl;
}
