/*
 * Вариант 2, Задача 2: Гибкая производственная линия
 *
 * Условия:
 *  - 4 станка выполняют заказы с разными уровнями приоритета
 *  - Приоритет: меньшее число = выше приоритет (1=срочный, 2=обычный, 3=плановый)
 *  - Если один станок выходит из строя, заказы автоматически перераспределяются
 *  - Потоки срочных заказов могут прерывать обычные процессы
 *  - Используется очередь с динамическими приоритетами (std::priority_queue)
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <condition_variable>
#include <string>
#include <windows.h>
#include "order.h"

using namespace std::chrono_literals;

// Константы и общее состояние
constexpr int NUM_MACHINES = 4;

std::mutex              qMtx;    // защита очереди заказов
std::mutex              coutMtx; // защита консольного вывода
std::condition_variable orderCv; // пробуждает станки при появлении нового заказа

std::priority_queue<Order> orderQueue;
std::atomic<bool> machineDown[NUM_MACHINES]; // true → станок вышел из строя
std::atomic<bool> allOrdersAdded{false};
std::atomic<int>  doneOrders{0};
int               totalOrders = 0;

// Потокобезопасный вывод
void log(const std::string& s) {
    std::lock_guard<std::mutex> lk(coutMtx);
    std::cout << s << std::endl;
}

//  Добавление заказа в очередь с уведомлением всех станков
void enqueue(Order o) {
    {
        std::lock_guard<std::mutex> lk(qMtx);
        orderQueue.push(o);
    }
    orderCv.notify_one();
}

//  Рабочий поток станка
//   1. Ожидает появления заказа в очереди
//   2. Перед началом работы проверяет наличие более приоритетного заказа
//   3. Обрабатывает заказ посекундно, проверяя аварийный флаг
//   4. При аварии возвращает незавершённый заказ в очередь
void machineWorker(int mid) {
    const std::string mname = "Станок #" + std::to_string(mid + 1);

    while (true) {
        // Проверяем состояние станка до получения заказа
        if (machineDown[mid]) {
            log("   " + mname + " ВЫШЕЛ ИЗ СТРОЯ! "
                "Незавершённые заказы перераспределяются автоматически.");
            return;
        }

        // Ждём заказ из очереди с приоритетами
        Order order{};
        {
            std::unique_lock<std::mutex> lk(qMtx);
            orderCv.wait_for(lk, 200ms, [] {
                return !orderQueue.empty() || allOrdersAdded.load();
            });

            if (orderQueue.empty()) {
                if (allOrdersAdded && doneOrders >= totalOrders) break;
                continue;
            }

            order = orderQueue.top();
            orderQueue.pop();
        }

        // Если взяли обычный/плановый заказ, но в очереди
        // появился срочный — возвращаем текущий и берём срочный
        if (order.priority > 1) {
            bool urgentPending = false;
            {
                std::lock_guard<std::mutex> lk(qMtx);
                urgentPending = !orderQueue.empty() &&
                                orderQueue.top().priority == 1;
            }
            if (urgentPending) {
                log("   [" + mname + "] Срочный заказ в очереди! "
                    "Заказ #" + std::to_string(order.id) +
                    " [пр." + std::to_string(order.priority) +
                    "] возвращён — беру срочный.");
                enqueue(order);
                // Следующая итерация возьмёт срочный заказ первым
                continue;
            }
        }

        // Снова проверяем состояние станка после получения заказа
        if (machineDown[mid]) {
            log("   " + mname + " вышел из строя перед началом заказа #" +
                std::to_string(order.id) + ". Возвращаю в очередь.");
            enqueue(order);
            return;
        }

        std::string priStr = order.priority == 1 ? "СРОЧНЫЙ " :
                             order.priority == 2 ? "обычный " : "плановый";
        log("  [" + mname + "] -> Начало заказа #" + std::to_string(order.id) +
            " [" + priStr + "] — длительность: " +
            std::to_string(order.duration) + " сек");

        // Обработка заказа посекундно (позволяет реагировать на аварию)
        bool interrupted = false;
        for (int t = 0; t < order.duration; ++t) {
            if (machineDown[mid]) {
                log("   " + mname + " упал во время заказа #" +
                    std::to_string(order.id) + " (через " +
                    std::to_string(t) + " с). Возвращаю в очередь.");
                enqueue(order); // возврат незавершённого заказа
                interrupted = true;
                break;
            }
            std::this_thread::sleep_for(1s);
        }

        if (interrupted) return;

        log("  [" + mname + "] Заказ #" + std::to_string(order.id) +
            " [" + priStr + "] выполнен");
        doneOrders++;
        orderCv.notify_all();
    }
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    log("ГИБКАЯ ПРОИЗВОДСТВЕННАЯ ЛИНИЯ\n");
    log("  Станков: " + std::to_string(NUM_MACHINES));
    log("  Приоритет: 1=СРОЧНЫЙ, 2=обычный, 3=плановый\n");

    for (int i = 0; i < NUM_MACHINES; ++i) machineDown[i] = false;

    // Заказы для обработки
    std::vector<Order> orders = {
        {1,  2, 3},  // обычный,  3 сек
        {2,  3, 2},  // плановый, 2 сек
        {3,  1, 1},  // СРОЧНЫЙ,  1 сек
        {4,  2, 2},  // обычный,  2 сек
        {5,  3, 2},  // плановый, 2 сек
        {6,  1, 2},  // СРОЧНЫЙ,  2 сек
        {7,  2, 3},  // обычный,  3 сек
        {8,  3, 1},  // плановый, 1 сек
        {9,  1, 2},  // СРОЧНЫЙ,  2 сек
        {10, 2, 2},  // обычный,  2 сек
    };
    totalOrders = static_cast<int>(orders.size());

    // Запуск станков
    std::vector<std::thread> machines;
    for (int i = 0; i < NUM_MACHINES; ++i)
        machines.emplace_back(machineWorker, i);

    // Добавляем заказы с небольшими паузами (имитация реального потока)
    for (auto& o : orders) {
        std::string priStr = o.priority == 1 ? "СРОЧНЫЙ " :
                             o.priority == 2 ? "обычный " : "плановый";
        log("Заказ #" + std::to_string(o.id) + " [" + priStr + "] добавлен в очередь");
        enqueue(o);

        // Симуляция поломки станка #2 после добавления 4-го заказа
        if (o.id == 4) {
            std::this_thread::sleep_for(300ms);
            machineDown[1] = true;
            log("\n   СОБЫТИЕ: Станок #2 выходит из строя!\n");
            orderCv.notify_all();
        }

        std::this_thread::sleep_for(300ms);
    }

    allOrdersAdded = true;
    orderCv.notify_all();

    for (auto& m : machines) m.join();

    log("\n Выполнено заказов: " + std::to_string(doneOrders.load()) + " из " + std::to_string(totalOrders));
    return 0;
}