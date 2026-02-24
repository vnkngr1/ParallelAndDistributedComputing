#ifndef STUDENT_HPP
#define STUDENT_HPP

#include "Person.hpp"
#include "RecordBook.hpp"

class Student : public Person {
private:
    RecordBook recordBook;

public:
    Student();
    explicit Student(const std::string& name);
    Student(const std::string& name, const std::string& recNumber);
    
    const std::vector<double>& getGrades() const { return recordBook.getGrades(); }
    
    void addGrade(double grade);
    double getAverage() const;
    std::string getRecordNumber() const;

    void print() const override;
};

#endif // STUDENT_HPP
