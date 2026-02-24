#include "Student.hpp"
#include "iostream"

Student::Student() : Person("Unknown"), recordBook() {}

Student::Student(const std::string& name) : Person(name), recordBook() {}

Student::Student(const std::string& name, const std::string& recNumber)
    : Person(name), recordBook(recNumber) {}

void Student::addGrade(double grade) {
    recordBook.addGrade(grade);
}

double Student::getAverage() const {
    return recordBook.getAverage();
}

std::string Student::getRecordNumber() const {
    return recordBook.getRecordNumber();
}

void Student::print() const {
    Person::print();
    std::cout << "Тип: Студент" << std::endl;
    recordBook.print();
}
