#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <iomanip>

class Student {
private:
    std::string name;
    std::vector<double> grades;

public:
    Student() : name("Unknown") {}

    explicit Student(const std::string& studentName) : name(studentName) {}

    void addGrade(double grade) {
        grades.push_back(grade);
    }

    double getAverage() const {
        if (grades.empty()) return 0.0;
        double sum = std::accumulate(grades.begin(), grades.end(), 0.0);
        return sum / grades.size();
    }

    void printInfo() const {
        std::cout << "Студент: " << name << std::endl;
        std::cout << "Оценки: ";
        for (double g : grades) {
            std::cout << g << " ";
        }
        std::cout << std::endl;
        std::cout << "Средний балл: " << std::fixed << std::setprecision(2) << getAverage() << std::endl;
    }
};

int main() {
    Student s1("Иван Петров");
    s1.addGrade(4.5);
    s1.addGrade(3.8);
    s1.addGrade(5.0);
    s1.printInfo();

    Student s2;
    s2.addGrade(3.0);
    s2.addGrade(4.0);
    s2.printInfo();

    Student s3("Анна Смирнова");
    s3.addGrade(5.0);
    s3.addGrade(4.9);
    s3.printInfo();

    return 0;
}
