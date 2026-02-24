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

    virtual void print() const {
        std::cout << "Имя: " << name << std::endl;
    }
};

class Student : public Person {
private:
    std::vector<double> grades;

public:
    Student() : Person("Unknown") {}

    explicit Student(const std::string& name) : Person(name) {}

    void addGrade(double grade) {
        grades.push_back(grade);
    }

    double getAverage() const {
        if (grades.empty()) return 0.0;
        double sum = std::accumulate(grades.begin(), grades.end(), 0.0);
        return sum / grades.size();
    }

    void print() const override {
        Person::print();
        std::cout << "Тип: Студент" << std::endl;
        std::cout << "Оценки: ";
        for (double g : grades) {
            std::cout << g << " ";
        }
        std::cout << std::endl;
        std::cout << "Средний балл: " << std::fixed << std::setprecision(2) << getAverage() << std::endl;
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

int main() {
    std::vector<Person*> people;

    Student* s1 = new Student("Иван Петров");
    s1->addGrade(4.5);
    s1->addGrade(3.8);
    s1->addGrade(5.0);
    people.push_back(s1);

    Student* s2 = new Student();
    s2->addGrade(3.0);
    s2->addGrade(4.0);
    people.push_back(s2);

    Student* s3 = new Student("Анна Смирнова");
    s3->addGrade(5.0);
    s3->addGrade(4.9);
    people.push_back(s3);

    Teacher* t1 = new Teacher("Мария Ивановна", "Математика");
    people.push_back(t1);

    Teacher* t2 = new Teacher("Петр Сергеевич", "Физика");
    people.push_back(t2);

    std::cout << "Информация о людях" << std::endl;
    for (const auto& person : people) {
        person->print();
        std::cout << "------------------------" << std::endl;
    }

    for (auto& person : people) {
        delete person;
    }
    people.clear();

    return 0;
}
