#ifndef TEACHER_HPP
#define TEACHER_HPP

#include "Person.hpp"
#include <string>

class Teacher : public Person {
private:
    std::string subject;

public:
    Teacher(const std::string& name, const std::string& subject);
    void print() const override;
};

#endif // TEACHER_HPP
