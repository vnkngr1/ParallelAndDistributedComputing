#ifndef PERSON_HPP
#define PERSON_HPP

#include <string>

class Person {
protected:
    std::string name;

public:
    explicit Person(const std::string& name);
    virtual ~Person();

    std::string getName() const;
    virtual void print() const;
};

#endif // PERSON_HPP
