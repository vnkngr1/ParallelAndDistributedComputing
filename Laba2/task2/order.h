//
// Created by ваня on 06.04.2026.
//

#ifndef CLIONPROJECT_ORDER_H
#define CLIONPROJECT_ORDER_H

struct Order {
    int id;
    int priority; // 1=срочный, 2=обычный, 3=плановый  (меньше → выше приоритет)
    int duration; // время обработки в секундах

    // Компаратор для max-heap: меньший номер приоритета — «больший» элемент
    bool operator<(const Order& o) const {
        return priority > o.priority;
    }
};

#endif //CLIONPROJECT_ORDER_H