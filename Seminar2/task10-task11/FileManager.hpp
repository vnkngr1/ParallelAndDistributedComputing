#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <string>
#include <vector>   

class Group;        
class Student;     

class FileManager {
public:
    static bool saveGroup(const std::string& filename, const Group& group);
    static bool loadGroup(const std::string& filename, Group& group, std::vector<Student*>& outNewStudents);
};

#endif // FILEMANAGER_HPP
