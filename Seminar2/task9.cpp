#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <iomanip>

class Person {
protected:
    std::string name;

public:
    Person(const std::string& name) : name(name) {}
    virtual ~Person() {}
    std::string getName() const { return name; }
    virtual void print() const {
        std::cout << "Имя: " << name << std::endl;
    }
};

class RecordBook {
private:
    std::string recordNumber;
    std::vector<double> grades;

public:
    RecordBook() : recordNumber("000000") {}
    explicit RecordBook(const std::string& number) : recordNumber(number) {}

    void addGrade(double grade) {
        grades.push_back(grade);
    }

    double getAverage() const {
        if (grades.empty()) return 0.0;
        double sum = std::accumulate(grades.begin(), grades.end(), 0.0);
        return sum / grades.size();
    }

    std::string getRecordNumber() const {
        return recordNumber;
    }

    void print() const {
        std::cout << "Зачётная книжка №" << recordNumber << std::endl;
        std::cout << "Оценки: ";
        for (double g : grades) {
            std::cout << g << " ";
        }
        std::cout << std::endl;
        std::cout << "Средний балл: " << std::fixed << std::setprecision(2) << getAverage() << std::endl;
    }
};

class Student : public Person {
private:
    RecordBook recordBook;

public:
    Student() : Person("Unknown"), recordBook() {}
    explicit Student(const std::string& name) : Person(name), recordBook() {}
    Student(const std::string& name, const std::string& recNumber)
        : Person(name), recordBook(recNumber) {}

    void addGrade(double grade) {
        recordBook.addGrade(grade);
    }

    double getAverage() const {
        return recordBook.getAverage();
    }

    void print() const override {
        Person::print();
        std::cout << "Тип: Студент" << std::endl;
        recordBook.print();
    }
};

class Teacher : public Person {
private:
    std::string subject;

public:
    Teacher(const std::string& name, const std::string& subject)
        : Person(name), subject(subject) {}

    void print() const override {
        Person::print();
        std::cout << "Тип: Учитель" << std::endl;
        std::cout << "Предмет: " << subject << std::endl;
    }
};

class Group {
private:
    std::string name;
    std::vector<Student*> students;

public:
    Group(const std::string& groupName) : name(groupName) {}

    void addStudent(Student* student) {
        students.push_back(student);
    }

    void removeStudent(const std::string& studentName) {
        for (auto it = students.begin(); it != students.end(); ++it) {
            if ((*it)->getName() == studentName) {
                students.erase(it);
                return;
            }
        }
    }

    double getGroupAverage() const {
        if (students.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : students) {
            sum += s->getAverage();
        }
        return sum / students.size();
    }

    void print() const {
        std::cout << "Группа: " << name << std::endl;
        std::cout << "Студенты (" << students.size() << "):" << std::endl;
        for (const auto& s : students) {
            s->print();
            std::cout << "----------" << std::endl;
        }
        std::cout << "Средний балл группы: " << std::fixed << std::setprecision(2) << getGroupAverage() << std::endl;
    }
};

int main() {
    Student* s1 = new Student("Иван Петров", "123456");
    s1->addGrade(4.5);
    s1->addGrade(3.8);
    s1->addGrade(5.0);

    Student* s2 = new Student();
    s2->addGrade(3.0);
    s2->addGrade(4.0);

    Student* s3 = new Student("Анна Смирнова", "789012");
    s3->addGrade(5.0);
    s3->addGrade(4.9);

    Group group("ИУ8-31");
    group.addStudent(s1);
    group.addStudent(s2);
    group.addStudent(s3);

    group.print();

    group.removeStudent("Unknown");
    std::cout << "\nПосле удаления студента Unknown:" << std::endl;
    group.print();

    delete s1;
    delete s2;
    delete s3;

    return 0;
}
