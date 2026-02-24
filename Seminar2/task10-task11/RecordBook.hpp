#ifndef RECORDBOOK_HPP
#define RECORDBOOK_HPP

#include <string>
#include <vector>

class RecordBook {
private:
    std::string recordNumber;
    std::vector<double> grades;

public:
    RecordBook();
    explicit RecordBook(const std::string& number);

    void addGrade(double grade);
    double getAverage() const;

    inline std::string getRecordNumber() const { return recordNumber; }
    const std::vector<double>& getGrades() const { return grades; }

    void print() const;

    size_t getGradeCount() const { return grades.size(); }
};

#endif // RECORDBOOK_HPP
