#include <iostream>
#include <vector>
#include <atomic>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/chrono.hpp>
#include <boost/random.hpp>
#include <windows.h>

const int NUM_BLANKS   = 10;
const int NUM_MACHINES = 3;
const int NUM_WORKERS = 5;

boost::interprocess::interprocess_semaphore machine_sem(NUM_MACHINES);
boost::interprocess::interprocess_semaphore blank_sem(NUM_BLANKS);

std::atomic<int> processed(0);
boost::mutex     console_mutex;

void worker(int id)
{
    boost::random::mt19937 rng(
        id * 2654435761u ^
        static_cast<unsigned>(boost::chrono::high_resolution_clock::now()
            .time_since_epoch().count())
    );
    boost::random::uniform_int_distribution<> dist(1, 2);

    while (true)
    {
        blank_sem.wait();

        int current = processed.fetch_add(1, std::memory_order_acq_rel);
        if (current >= NUM_BLANKS)
        {
            blank_sem.post();
            break;
        }

        machine_sem.wait();

        int work_time = dist(rng);

        {
            boost::lock_guard<boost::mutex> clog(console_mutex);
            std::cout << "[Рабочий " << id << "] загрузил заготовку #" << (current + 1)
                      << " в машину. Обработка " << work_time << " сек." << std::endl;
        }

        boost::this_thread::sleep_for(boost::chrono::seconds(work_time));

        {
            boost::lock_guard<boost::mutex> clog(console_mutex);
            std::cout << "[Рабочий " << id << "] заготовка #" << (current + 1)
                      << " обработана, возвращена на склад." << std::endl;
        }

        machine_sem.post();
        blank_sem.post();
    }

    {
        boost::lock_guard<boost::mutex> clog(console_mutex);
        std::cout << "[Рабочий " << id << "] завершил работу." << std::endl;
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "Склад: " << NUM_BLANKS << " заготовок, "
              << NUM_MACHINES << " машины" << std::endl << std::endl;

    std::vector<boost::thread> threads;
    threads.reserve(NUM_WORKERS);

    for (int i = 1; i <= NUM_WORKERS; ++i)
        threads.emplace_back(worker, i);

    for (auto& t : threads)
        t.join();

    std::cout << std::endl << "Все заготовки обработаны." << std::endl;
    return 0;
}