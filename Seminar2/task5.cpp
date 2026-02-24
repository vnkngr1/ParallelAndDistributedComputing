#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <ctime>

#pragma pack(push, 1)
struct FileHeader {
    char signature[8];
    uint32_t version;
    uint32_t studentCount;
    uint32_t subjectCount;
};
#pragma pack(pop)

class GradeFileManager {
private:
    static constexpr const char* EXPECTED_SIGNATURE = "GRADES1";
    static constexpr uint32_t CURRENT_VERSION = 1;

public:
    static void printHeaderSize() {
        std::cout << "Размер структуры FileHeader: " << sizeof(FileHeader) << " байт" << std::endl;
        std::cout << "  - signature: " << sizeof(((FileHeader*)0)->signature) << " байт" << std::endl;
        std::cout << "  - version: " << sizeof(((FileHeader*)0)->version) << " байт" << std::endl;
        std::cout << "  - studentCount: " << sizeof(((FileHeader*)0)->studentCount) << " байт" << std::endl;
        std::cout << "  - subjectCount: " << sizeof(((FileHeader*)0)->subjectCount) << " байт" << std::endl;
    }

    static bool saveToFile(const std::string& filename,
                           const std::vector<std::vector<double>>& grades) {

        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Ошибка открытия файла для записи: " << filename << std::endl;
            return false;
        }

        FileHeader header;
        std::strncpy(header.signature, EXPECTED_SIGNATURE, sizeof(header.signature));
        header.version = CURRENT_VERSION;
        header.studentCount = static_cast<uint32_t>(grades.size());
        header.subjectCount = grades.empty() ? 0 : static_cast<uint32_t>(grades[0].size());

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        for (const auto& student : grades) {
            file.write(reinterpret_cast<const char*>(student.data()),
                      student.size() * sizeof(double));
        }

        std::cout << "Данные сохранены в файл: " << filename << std::endl;
        std::cout << "  Студентов: " << header.studentCount << std::endl;
        std::cout << "  Предметов: " << header.subjectCount << std::endl;

        return true;
    }

    static bool loadFromFile(const std::string& filename,
                            std::vector<std::vector<double>>& grades) {

        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Ошибка открытия файла для чтения: " << filename << std::endl;
            return false;
        }

        FileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!file) {
            std::cerr << "Ошибка чтения заголовка файла" << std::endl;
            return false;
        }

        if (std::strncmp(header.signature, EXPECTED_SIGNATURE, sizeof(header.signature)) != 0) {
            std::cerr << "Ошибка: неверная сигнатура файла!" << std::endl;
            std::cerr << "  Ожидалось: " << EXPECTED_SIGNATURE << std::endl;
            std::cerr << "  Получено: " << std::string(header.signature, 8) << std::endl;
            return false;
        }

        if (header.version != CURRENT_VERSION) {
            std::cerr << "Ошибка: неверная версия файла!" << std::endl;
            std::cerr << "  Ожидалось: " << CURRENT_VERSION << std::endl;
            std::cerr << "  Получено: " << header.version << std::endl;
            return false;
        }

        grades.clear();
        grades.reserve(header.studentCount);

        std::vector<double> studentGrades(header.subjectCount);

        for (uint32_t i = 0; i < header.studentCount; ++i) {
            file.read(reinterpret_cast<char*>(studentGrades.data()),
                     header.subjectCount * sizeof(double));

            if (!file) {
                std::cerr << "Ошибка чтения данных студента " << i + 1 << std::endl;
                return false;
            }

            grades.push_back(studentGrades);
        }

        std::cout << "Данные загружены из файла: " << filename << std::endl;
        std::cout << "  Студентов: " << header.studentCount << std::endl;
        std::cout << "  Предметов: " << header.subjectCount << std::endl;

        return true;
    }

    static bool verifyFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Файл не найден: " << filename << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        FileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!file) {
            std::cerr << "Не удалось прочитать заголовок" << std::endl;
            return false;
        }

        if (std::strncmp(header.signature, EXPECTED_SIGNATURE, sizeof(header.signature)) != 0) {
            std::cerr << "Неверная сигнатура" << std::endl;
            return false;
        }

        std::streampos expectedSize = sizeof(header) +
                                      header.studentCount * header.subjectCount * sizeof(double);

        std::cout << "\nПРОВЕРКА ФАЙЛА" << std::endl;
        std::cout << "Сигнатура: " << std::string(header.signature, 8) << std::endl;
        std::cout << "Версия: " << header.version << std::endl;
        std::cout << "Студентов: " << header.studentCount << std::endl;
        std::cout << "Предметов: " << header.subjectCount << std::endl;
        std::cout << "Размер файла: " << fileSize << " байт" << std::endl;
        std::cout << "Ожидаемый размер: " << expectedSize << " байт" << std::endl;

        return fileSize == expectedSize;
    }
};

void printGrades(const std::vector<std::vector<double>>& grades) {
    std::cout << std::fixed << std::setprecision(2);
    for (size_t i = 0; i < grades.size(); ++i) {
        std::cout << "Студент " << i + 1 << ": ";
        for (double grade : grades[i]) {
            std::cout << grade << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::vector<std::vector<double>> grades = {
        {4.5, 3.8, 5.0, 4.2, 4.8},
        {3.5, 4.0, 3.8, 4.5, 3.9},
        {5.0, 4.8, 5.0, 4.9, 5.0},
        {3.0, 3.5, 4.0, 3.2, 3.8},
        {4.0, 4.0, 4.0, 4.0, 4.0}
    };

    std::string filename = "grades.bin";

    std::cout << "РАЗМЕР СТРУКТУРЫ" << std::endl;
    GradeFileManager::printHeaderSize();

    std::cout << "\СОХРАНЕНИЕ ДАННЫХ" << std::endl;
    std::cout << "Исходные данные:" << std::endl;
    printGrades(grades);

    if (!GradeFileManager::saveToFile(filename, grades)) {
        return 1;
    }

    std::cout << "\nПРОВЕРКА ФАЙЛА" << std::endl;
    GradeFileManager::verifyFile(filename);

    std::cout << "\nЧТЕНИЕ ДАННЫХ" << std::endl;
    std::vector<std::vector<double>> loadedGrades;

    if (!GradeFileManager::loadFromFile(filename, loadedGrades)) {
        return 1;
    }

    std::cout << "Загруженные данные:" << std::endl;
    printGrades(loadedGrades);

    std::cout << "\nПРОВЕРКА ОБРАБОТКИ ОШИБОК" << std::endl;

    std::ofstream badFile("bad_signature.bin", std::ios::binary);
    FileHeader badHeader;
    std::strncpy(badHeader.signature, "BADFILE", 8);
    badHeader.version = 1;
    badHeader.studentCount = 5;
    badHeader.subjectCount = 5;
    badFile.write(reinterpret_cast<const char*>(&badHeader), sizeof(badHeader));
    badFile.close();

    std::vector<std::vector<double>> testGrades;
    if (!GradeFileManager::loadFromFile("bad_signature.bin", testGrades)) {
        std::cout << "Ошибка успешно обнаружена!" << std::endl;
    }

    return 0;
}
