#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <vector>

class Student; 

class Group {
private:
    std::string name;
    std::vector<Student*> students;

public:
    explicit Group(const std::string& groupName);

    void addStudent(Student* student);
    void removeStudent(const std::string& studentName);
    Student* findStudent(const std::string& studentName) const;

    double getGroupAverage() const;
    void sortStudentsByAverage(bool descending = true);
    void filterByThreshold(double threshold); 

    void print() const;

    const std::vector<Student*>& getStudents() const { return students; }
    size_t getStudentCount() const { return students.size(); }
    std::string getName() const { return name; }
};

#endif // GROUP_HPP
