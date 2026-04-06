#ifndef CLIONPROJECT_EMPLOYEE_H
#define CLIONPROJECT_EMPLOYEE_H

struct Employee {
    int  id;
    bool highPriority;

    // Оператор для max-heap: высокоприоритетные — наверх;
    // при одинаковом приоритете — меньший id идёт первым.
    bool operator<(const Employee& o) const {
        if (highPriority != o.highPriority)
            return !highPriority;   // !true < true → high уходит «вверх» кучи
        return id > o.id;
    }
};

#endif //CLIONPROJECT_EMPLOYEE_H