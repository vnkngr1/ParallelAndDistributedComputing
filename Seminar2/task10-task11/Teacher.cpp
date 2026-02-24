#include "Teacher.hpp"
#include <iostream>

Teacher::Teacher(const std::string& name, const std::string& subject)
    : Person(name), subject(subject) {}

void Teacher::print() const {
    Person::print();
    std::cout << "Тип: Учитель" << std::endl;
    std::cout << "Предмет: " << subject << std::endl;
}
