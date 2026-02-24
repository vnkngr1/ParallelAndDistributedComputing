#include "FileManager.hpp"
#include "Group.hpp"
#include "Student.hpp"
#include <fstream>
#include <iostream>
#include <cstring>

bool FileManager::saveGroup(const std::string& filename, const Group& group) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Ошибка открытия файла для записи: " << filename << std::endl;
        return false;
    }

    size_t nameSize = group.getName().size();
    file.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize));
    file.write(group.getName().c_str(), nameSize);

    size_t studentCount = group.getStudentCount();
    file.write(reinterpret_cast<const char*>(&studentCount), sizeof(studentCount));

    for (Student* s : group.getStudents()) {

        std::string sName = s->getName();
        size_t sNameSize = sName.size();
        file.write(reinterpret_cast<const char*>(&sNameSize), sizeof(sNameSize));
        file.write(sName.c_str(), sNameSize);

        std::string recNum = s->getRecordNumber();
        size_t recNumSize = recNum.size();
        file.write(reinterpret_cast<const char*>(&recNumSize), sizeof(recNumSize));
        file.write(recNum.c_str(), recNumSize);

        const std::vector<double>& grades = s->getGrades(); 
        size_t gradeCount = grades.size();
        file.write(reinterpret_cast<const char*>(&gradeCount), sizeof(gradeCount));
        file.write(reinterpret_cast<const char*>(grades.data()), gradeCount * sizeof(double));
    }

    file.close();
    std::cout << "Группа сохранена в файл " << filename << std::endl;
    return true;
}

bool FileManager::loadGroup(const std::string& filename, Group& group, std::vector<Student*>& outNewStudents) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Ошибка открытия файла для чтения: " << filename << std::endl;
        return false;
    }

    size_t nameSize;
    file.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
    char* buf = new char[nameSize + 1];
    file.read(buf, nameSize);
    buf[nameSize] = '\0';
    std::string groupName(buf);
    delete[] buf;

    size_t studentCount;
    file.read(reinterpret_cast<char*>(&studentCount), sizeof(studentCount));

    outNewStudents.clear();
    outNewStudents.reserve(studentCount);

    for (size_t i = 0; i < studentCount; ++i) {
        file.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
        buf = new char[nameSize + 1];
        file.read(buf, nameSize);
        buf[nameSize] = '\0';
        std::string sName(buf);
        delete[] buf;

        size_t recNumSize;
        file.read(reinterpret_cast<char*>(&recNumSize), sizeof(recNumSize));
        buf = new char[recNumSize + 1];
        file.read(buf, recNumSize);
        buf[recNumSize] = '\0';
        std::string recNum(buf);
        delete[] buf;

        size_t gradeCount;
        file.read(reinterpret_cast<char*>(&gradeCount), sizeof(gradeCount));
        std::vector<double> grades(gradeCount);
        file.read(reinterpret_cast<char*>(grades.data()), gradeCount * sizeof(double));

        Student* s = new Student(sName, recNum);
        for (double g : grades) {
            s->addGrade(g);
        }
        outNewStudents.push_back(s);
        group.addStudent(s);
    }

    file.close();
    std::cout << "Группа загружена из файла " << filename << std::endl;
    return true;
}
