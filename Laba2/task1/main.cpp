/*
 * Лабораторная работа №2 — Вариант 2
 * Задача 1: Система интеллектуального контроля доступа
 *   - 5 турникетов, 20 сотрудников конкурируют за вход
 *   - Высокоприоритетные: id % 5 == 0 (5, 10, 15, 20)
 *   - Очередь > 5 → открывается дополнительный турникет
 *   - Турникет #3 выходит из строя через ~3 с → аварийное перенаправление
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <string>
#include <windows.h>
#include "employee.h"

static constexpr int INITIAL_TURNSTILES  = 5;  // начальное количество турникетов
static constexpr int MAX_EXTRA           = 2;  // количество дополнительных турникетов
static constexpr int NUM_EMPLOYEES       = 20; // общее число сотрудников
static constexpr int BREAKDOWN_COUNTDOWN = 5;  // количество секунд до поломки турникета

static std::mutex               g_mtx;
static std::condition_variable  g_cv;

static int                           g_slots = INITIAL_TURNSTILES; // свободных слотов
static int                           g_extra = 0;                  // открыто дополнительных турникетов
static std::priority_queue<Employee> g_queue;                      // очередь ожидания

static std::mutex g_log_mtx; // Мьютекс для вывода

static void log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    std::cout << msg << std::endl;
}

// Симуляция поломки турникета
// Турникет #3 ломается -> аварийная система уменьшает
// доступную ёмкость и уведомляет всех ожидающих о перенаправлении.
static void simulate_breakdown() {
    std::this_thread::sleep_for(std::chrono::seconds(BREAKDOWN_COUNTDOWN));

    {
        std::unique_lock<std::mutex> lk(g_mtx);
        // Убираем один слот
        if (g_slots > 0) {
            --g_slots;
        }
        log("\n[!!!] АВАРИЯ: Турникет #3 сломался!\n"
            "      Аварийная система перенаправила поток сотрудников.\n"
            "      Доступных слотов: " + std::to_string(g_slots) + "\n");
    }
    g_cv.notify_all(); // будим всех для переоценки состояния
}

// Поток сотрудника
static void employee_func(int id, bool high) {
    const std::string name = "Сотрудник " + std::to_string(id) + (high ? " [ПРИОРИТЕТ]" : "           ");

    // Прибытие: встаёт в очередь
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_queue.push({id, high});
        int qsz = static_cast<int>(g_queue.size());

        log(name + " прибыл. Ожидают в очереди: " + std::to_string(qsz));

        // Если очередь переполнена — открываем дополнительный турникет
        if (qsz > 5 && g_extra < MAX_EXTRA) {
            ++g_extra;
            ++g_slots; // добавляем один слот
            log("  [+] Очередь > 5! Открыт доп. турникет #" +
                std::to_string(INITIAL_TURNSTILES + g_extra) +
                ". Свободных слотов: " + std::to_string(g_slots));
        }
    }
    g_cv.notify_all();

    // Ожидание: блокируемся, пока не окажемся во главе очереди
    // И не появится свободный слот (приоритет уже встроен в priority_queue)
    {
        std::unique_lock<std::mutex> lk(g_mtx);
        g_cv.wait(lk, [&] {
            return g_slots > 0 &&
                   !g_queue.empty() &&
                   g_queue.top().id == id; // мы — первые по приоритету
        });

        --g_slots;     // занимаем слот
        g_queue.pop(); // убираем себя из очереди
        log(name + " --> проходит через турникет.");
    }
    g_cv.notify_all(); // уведомляем следующего

    // Проход через турникет (имитация времени сканирования пропуска)
    std::this_thread::sleep_for(
        std::chrono::milliseconds(high ? 200 : 450));
    log(name + " [OK] вошёл в здание.");

    // Освобождение слота
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        ++g_slots;
    }
    g_cv.notify_all();
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    log("Система интеллектуального контроля доступа\n");
    log("   Турникетов: " + std::to_string(INITIAL_TURNSTILES));
    log("   Сотрудников: " + std::to_string(NUM_EMPLOYEES));
    log("   Высокий приоритет: id кратный 5  (5,10,15,20)");
    log("   Турникет #3 сломается через ~3 секунды\n");

    // Поток симуляции аварии
    std::thread breakdown_thread(simulate_breakdown);

    // Сотрудники прибывают с небольшой задержкой — имитация утреннего потока
    std::vector<std::thread> employees;
    employees.reserve(NUM_EMPLOYEES);
    for (int i = 1; i <= NUM_EMPLOYEES; ++i) {
        employees.emplace_back(employee_func, i, (i % 5 == 0));
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }

    for (auto& t : employees)  t.join();
    breakdown_thread.join();

    log("\n   Все сотрудники вошли в здание.");
    return 0;
}