#include "Group.hpp"
#include "Student.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

Group::Group(const std::string& groupName) : name(groupName) {}

void Group::addStudent(Student* student) {
    students.push_back(student);
}

void Group::removeStudent(const std::string& studentName) {
    auto it = std::find_if(students.begin(), students.end(),
        [&](Student* s) { return s->getName() == studentName; });
    if (it != students.end()) {
        students.erase(it);
    }
}

Student* Group::findStudent(const std::string& studentName) const {
    auto it = std::find_if(students.begin(), students.end(),
        [&](Student* s) { return s->getName() == studentName; });
    return (it != students.end()) ? *it : nullptr;
}

double Group::getGroupAverage() const {
    if (students.empty()) return 0.0;
    double sum = 0.0;
    for (auto s : students) {
        sum += s->getAverage();
    }
    return sum / students.size();
}

void Group::sortStudentsByAverage(bool descending) {
    std::sort(students.begin(), students.end(),
        [descending](Student* a, Student* b) {
            if (descending)
                return a->getAverage() > b->getAverage();
            else
                return a->getAverage() < b->getAverage();
        });
}

void Group::filterByThreshold(double threshold) {
    students.erase(std::remove_if(students.begin(), students.end(),
        [threshold](Student* s) { return s->getAverage() < threshold; }),
        students.end());
}

void Group::print() const {
    std::cout << "Группа: " << name << std::endl;
    std::cout << "Студенты (" << students.size() << "):\n";
    for (const auto& s : students) {
        s->print();
        std::cout << "----------" << std::endl;
    }
    std::cout << "Средний балл группы: " << std::fixed << std::setprecision(2) << getGroupAverage() << std::endl;
}
