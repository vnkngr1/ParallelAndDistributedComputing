#include <iostream>
#include <vector>
#include <atomic>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono.hpp>
#include <boost/random.hpp>
#include <windows.h>

const int NUM_MACHINES = 3;
const int NUM_WORKERS  = 5;

// Разделяемые ресурсы
boost::mutex              machines_mutex;
boost::mutex              console_mutex;
boost::condition_variable machine_available_cv;
std::atomic<int>          free_machines(NUM_MACHINES);

// Рабочий
void worker(int id)
{
    boost::random::mt19937 rng(id * 42 +
        static_cast<unsigned>(boost::chrono::system_clock::now()
            .time_since_epoch().count()));
    boost::random::uniform_int_distribution<> dist(1, 3);

    // Ожидает свободный станок
    {
        boost::unique_lock<boost::mutex> lock(machines_mutex);

        machine_available_cv.wait(lock, [] {
            return free_machines.load(std::memory_order_acquire) > 0;
        });

        free_machines.fetch_sub(1, std::memory_order_acq_rel);

        {
            boost::lock_guard<boost::mutex> clog(console_mutex);
            std::cout << "[Рабочий " << id << "] занял станок. "
                      << "Свободно: " << free_machines.load() << std::endl;
        }
    }

    // Работает на станке
    int work_time = dist(rng);
    {
        boost::lock_guard<boost::mutex> clog(console_mutex);
        std::cout << "[Рабочий " << id << "] работает " << work_time << " сек." << std::endl;
    }
    boost::this_thread::sleep_for(boost::chrono::seconds(work_time));

    // Освобождает станок
    {
        boost::lock_guard<boost::mutex> lock(machines_mutex);

        free_machines.fetch_add(1, std::memory_order_acq_rel);

        {
            boost::lock_guard<boost::mutex> clog(console_mutex);
            std::cout << "[Рабочий " << id << "] освободил станок. "
                      << "Свободно: " << free_machines.load() << std::endl;
        }
    }

    machine_available_cv.notify_all();
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "Производственная система: "
              << NUM_MACHINES << " станка, "
              << NUM_WORKERS  << " рабочих" << std::endl;

    std::vector<boost::thread> threads;
    threads.reserve(NUM_WORKERS);
    for (int i = 1; i <= NUM_WORKERS; ++i) threads.emplace_back(worker, i);

    for (auto& t : threads) t.join();

    std::cout << "Все рабочие завершили работу" << std::endl;
    return 0;
}