#include <iostream>
#include <vector>
#include <limits>
#include "Student.hpp"
#include "Teacher.hpp"
#include "Group.hpp"
#include "FileManager.hpp"

void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printStatistics(const Group& group) {
    int excellent = 0;
    int atRisk = 0;
    for (Student* s : group.getStudents()) {
        double avg = s->getAverage();
        if (avg >= 4.5) excellent++;
        else if (avg < 3.0) atRisk++;
    }
    std::cout << "\n--- СТАТИСТИКА ГРУППЫ ---\n";
    std::cout << "Отличников (ср. >= 4.5): " << excellent << std::endl;
    std::cout << "В группе риска (ср. < 3.0): " << atRisk << std::endl;
}

void menu() {
    std::cout << "\n===== МЕНЮ =====\n";
    std::cout << "1. Показать группу\n";
    std::cout << "2. Добавить студента (создать нового)\n";
    std::cout << "3. Удалить студента по имени\n";
    std::cout << "4. Найти студента по имени\n";
    std::cout << "5. Сортировать студентов по среднему баллу\n";
    std::cout << "6. Фильтровать (удалить с баллом ниже порога)\n";
    std::cout << "7. Сохранить группу в файл\n";
    std::cout << "8. Загрузить группу из файла\n";
    std::cout << "9. Показать статистику\n";
    std::cout << "0. Выход\n";
    std::cout << "Выбор: ";
}

int main() {
    std::vector<Student*> allStudents; // владеем всеми студентами
    Group group("ИУ8-31");

    // Несколько предустановленных студентов
    Student* s1 = new Student("Иван Петров", "123456");
    s1->addGrade(4.5);
    s1->addGrade(3.8);
    s1->addGrade(5.0);
    allStudents.push_back(s1);
    group.addStudent(s1);

    Student* s2 = new Student("Петр Сидоров", "234567");
    s2->addGrade(3.0);
    s2->addGrade(4.0);
    s2->addGrade(3.5);
    allStudents.push_back(s2);
    group.addStudent(s2);

    Student* s3 = new Student("Анна Смирнова", "345678");
    s3->addGrade(5.0);
    s3->addGrade(4.9);
    s3->addGrade(5.0);
    allStudents.push_back(s3);
    group.addStudent(s3);

    int choice;
    do {
        menu();
        std::cin >> choice;
        if (std::cin.fail()) {
            clearInput();
            std::cout << "Ошибка ввода. Попробуйте снова.\n";
            continue;
        }
        switch (choice) {
            case 1:
                group.print();
                break;
            case 2: {
                std::string name, recNum;
                std::cout << "Имя студента: ";
                clearInput();
                std::getline(std::cin, name);
                std::cout << "Номер зачётки: ";
                std::getline(std::cin, recNum);
                Student* s = new Student(name, recNum);
                int gradeCount;
                std::cout << "Сколько оценок добавить? ";
                std::cin >> gradeCount;
                if (std::cin.fail() || gradeCount < 0) {
                    std::cout << "Неверное количество.\n";
                    delete s;
                    break;
                }
                for (int i = 0; i < gradeCount; ++i) {
                    double g;
                    std::cout << "Оценка " << i+1 << ": ";
                    std::cin >> g;
                    if (std::cin.fail() || g < 0 || g > 5) {
                        std::cout << "Неверная оценка. Пропускаем.\n";
                        clearInput();
                        continue;
                    }
                    s->addGrade(g);
                }
                allStudents.push_back(s);
                group.addStudent(s);
                std::cout << "Студент добавлен.\n";
                break;
            }
            case 3: {
                std::string name;
                std::cout << "Имя студента для удаления: ";
                clearInput();
                std::getline(std::cin, name);
                group.removeStudent(name);
                std::cout << "Удалён из группы (если был).\n";
                break;
            }
            case 4: {
                std::string name;
                std::cout << "Имя студента для поиска: ";
                clearInput();
                std::getline(std::cin, name);
                Student* found = group.findStudent(name);
                if (found) {
                    found->print();
                } else {
                    std::cout << "Студент не найден.\n";
                }
                break;
            }
            case 5:
                group.sortStudentsByAverage(true);
                std::cout << "Группа отсортирована по убыванию среднего балла.\n";
                break;
            case 6: {
                double thresh;
                std::cout << "Введите порог: ";
                std::cin >> thresh;
                if (std::cin.fail()) {
                    std::cout << "Неверный ввод.\n";
                    clearInput();
                    break;
                }
                group.filterByThreshold(thresh);
                std::cout << "Студенты с баллом ниже " << thresh << " удалены из группы.\n";
                break;
            }
            case 7: {
                std::string fname;
                std::cout << "Имя файла: ";
                clearInput();
                std::getline(std::cin, fname);
                FileManager::saveGroup(fname, group);
                break;
            }
            case 8: {
                std::string fname;
                std::cout << "Имя файла: ";
                clearInput();
                std::getline(std::cin, fname);
                Group newGroup("loaded");
                std::vector<Student*> loadedStudents;
                if (FileManager::loadGroup(fname, newGroup, loadedStudents)) {
                    for (Student* s : allStudents) delete s;
                    allStudents.clear();
                    allStudents = loadedStudents;
                    group = newGroup;
                    std::cout << "Загруженная группа:\n";
                    newGroup.print();
                    std::cout << "Для простоты старая группа осталась без изменений.\n";
                }
                break;
            }
            case 9:
                printStatistics(group);
                break;
            case 0:
                std::cout << "Выход.\n";
                break;
            default:
                std::cout << "Неверный пункт.\n";
        }
    } while (choice != 0);

    for (Student* s : allStudents) {
        delete s;
    }
    return 0;
}
