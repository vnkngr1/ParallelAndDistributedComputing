#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>

#include <windows.h>

constexpr int    NUM_CHILDREN = 5;
constexpr char   SHM_NAME[]   = "/task_shm";
constexpr char   SEM_BARRIER[]= "/sem_barrier";
constexpr char   SEM_MUTEX[]  = "/sem_mutex";

struct SharedData {
    int  results[NUM_CHILDREN];
    int  barrier_count;
    bool all_done;
};

static sem_t* open_sem(const char* name, unsigned int init_val) {
    sem_t* sem = sem_open(name, O_CREAT, 0600, init_val);
    if (sem == SEM_FAILED) {
        std::cerr << "sem_open(" << name << ") failed: "
                  << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return sem;
}

static int do_work(int id) {
    int sum = 0;
    for (int i = 1; i <= (id + 1) * 10; ++i)
        sum += i * i;

    sleep(id % 3 + 1);
    return sum;
}

// каждый процесс захватывает mutex, инкрементирует barrier_count.
// последний пришедший сбрасывает счётчик и вызывает sem_post(barrier) ровно (N-1) раз, чтобы разбудить остальных.
// все остальные блокируются на sem_wait(barrier).
static void barrier_wait(SharedData* shm, sem_t* sem_barrier, sem_t* sem_mutex) {
    // войти в мьютекс
    sem_wait(sem_mutex);
    shm->barrier_count++;
    int cnt = shm->barrier_count;
    sem_post(sem_mutex);

    if (cnt == NUM_CHILDREN) {
        // если последний, то сбрасываем счётчик и будим остальных
        sem_wait(sem_mutex);
        shm->barrier_count = 0;
        sem_post(sem_mutex);

        for (int i = 0; i < NUM_CHILDREN - 1; ++i)
            sem_post(sem_barrier);
    } else {
        sem_wait(sem_barrier);
    }
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_BARRIER);
    sem_unlink(SEM_MUTEX);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) {
        std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
        return 1;
    }

    SharedData* shm = static_cast<SharedData*>(
        mmap(nullptr, sizeof(SharedData),
             PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shm == MAP_FAILED) {
        std::cerr << "mmap failed: " << strerror(errno) << std::endl;
        return 1;
    }

    std::memset(shm, 0, sizeof(SharedData));

    sem_t* sem_barrier = open_sem(SEM_BARRIER, 0);
    sem_t* sem_mutex   = open_sem(SEM_MUTEX,   1);

    std::cout << "[Родитель] PID=" << getpid()
              << ", создаю " << NUM_CHILDREN << " дочерних процессов...\n\n";

    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "fork() failed: " << strerror(errno) << std::endl;
            return 1;
        }

        if (pid == 0) {

            std::cout << "[Процесс " << i << "] PID=" << getpid()
                      << " начинает работу...\n";

            // выполнение вычислений
            int result = do_work(i);

            std::cout << "[Процесс " << i << "] завершил вычисления,"
                      << " результат=" << result
                      << ". Жду остальных на барьере...\n";

            // барьер. ждём, пока все процессы завершат вычисления
            barrier_wait(shm, sem_barrier, sem_mutex);

            std::cout << "[Процесс " << i << "] прошёл барьер."
                      << " Записываю результат в shm.\n";

            // записываем результат в разделяемую память (mutex не нужен, т.к. каждый пишет в свою ячейку)
            shm->results[i] = result;

            // закрытие ресурса дочернего процесса
            sem_close(sem_barrier);
            sem_close(sem_mutex);
            munmap(shm, sizeof(SharedData));
            close(shm_fd);

            exit(EXIT_SUCCESS);
        }
    }


    for (int i = 0; i < NUM_CHILDREN; ++i) {
        int status;
        pid_t finished = wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            std::cout << "[Родитель] PID " << finished << " завершился успешно.\n";
        else
            std::cout << "[Родитель] PID " << finished << " завершился с ошибкой.\n";
    }


    std::cout << "\n=== Итоги (собраны родителем) ===\n";
    long long total = 0;
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        std::cout << "  Процесс " << i
                  << ": результат = " << shm->results[i] << std::endl;
        total += shm->results[i];
    }
    std::cout << "  Общая сумма всех результатов: " << total << std::endl;


    sem_close(sem_barrier);   sem_unlink(SEM_BARRIER);
    sem_close(sem_mutex);     sem_unlink(SEM_MUTEX);
    munmap(shm, sizeof(SharedData));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    std::cout << "\n[Родитель] Готово. Ресурсы освобождены.\n";
    return 0;
}