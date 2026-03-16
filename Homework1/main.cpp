#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

// Структура для хранения статистики по одному файлу
struct FileStats {
    std::string filename;
    long long   word_count   = 0;
    long long   char_count   = 0;
    bool        success      = false;
};

// Счётчик обработанных файлов.
std::atomic<int> g_processed_count{0};

// Мьютекс и condition_variable для очереди задач
std::mutex              g_queue_mutex;
std::condition_variable g_queue_cv;
std::queue<std::string> g_task_queue;        // очередь имён файлов
bool                    g_all_pushed = false; // флаг: все файлы добавлены в очередь

// Мьютекс для защиты вектора результатов
std::mutex              g_results_mutex;
std::vector<FileStats>  g_results;

// Мьютекс для безопасного вывода
std::mutex g_cout_mutex;

// Вывод в консоль из нескольких потоков
template<typename... Args>
void safe_print(Args&&... args) {
    std::lock_guard<std::mutex> lock(g_cout_mutex);
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}

// Обработка одного файла
FileStats process_file(const std::string& filepath) {

    constexpr std::size_t BLOCK_SIZE = 4096; // размер одного блока (байт)

    FileStats stats;
    stats.filename = filepath;

    std::ifstream infile(filepath, std::ios::binary);
    if (!infile.is_open()) {
        safe_print("[ОШИБКА] Не удалось открыть файл: ", filepath);
        return stats;
    }

    // Читаем файл блоками и считаем символы и слова
    std::string buffer(BLOCK_SIZE, '\0');
    bool in_word = false;

    while (infile.read(&buffer[0], BLOCK_SIZE) || infile.gcount() > 0) {
        std::size_t bytes_read = static_cast<std::size_t>(infile.gcount());
        stats.char_count += static_cast<long long>(bytes_read);

        for (std::size_t i = 0; i < bytes_read; ++i) {
            char c = buffer[i];
            bool is_space = (c == ' ' || c == '\n' || c == '\r' || c == '\t');
            if (!is_space && !in_word) {
                in_word = true;
                ++stats.word_count;
            } else if (is_space) {
                in_word = false;
            }
        }
    }
    infile.close();

    // Запись результата в processed_<имя>.txt
    std::string out_name = "processed_" + fs::path(filepath).filename().string();
    std::ofstream outfile(out_name);
    if (outfile.is_open()) {
        outfile << "Файл:           " << filepath        << "\n";
        outfile << "Количество слов: " << stats.word_count << "\n";
        outfile << "Количество символов: " << stats.char_count << "\n";
        outfile.close();
    }

    stats.success = true;
    return stats;
}

//  Функция рабочего потока
void worker_thread(int thread_id) {
    safe_print("[Поток ", thread_id, "] запущен");

    while (true) {
        std::string filepath;

        {
            // Блок до тех пор, пока очередь не пуста или пока не выставлен флаг завершения
            std::unique_lock<std::mutex> lock(g_queue_mutex);
            g_queue_cv.wait(lock, [] {
                return !g_task_queue.empty() || g_all_pushed;
            });

            // Если очередь пуста и все задачи уже добавлены — выход
            if (g_task_queue.empty() && g_all_pushed) {
                safe_print("[Поток ", thread_id, "] завершает работу (очередь пуста)");
                break;
            }

            // Поток берёт задачу из очереди
            filepath = g_task_queue.front();
            g_task_queue.pop();
        }
        // Когда мьютекс освобождён обработка файла параллельно

        safe_print("[Поток ", thread_id, "] обрабатывает: ", filepath);
        FileStats stats = process_file(filepath);

        // Сохранение результат в общий вектор
        {
            std::lock_guard<std::mutex> lock(g_results_mutex);
            g_results.push_back(stats);
        }

        int count = g_processed_count.fetch_add(1) + 1;
        safe_print("[Поток ", thread_id, "] завершил: ", filepath,
                   " (обработано файлов: ", count, ")");
    }
}

int main(int argc, char* argv[]) {

    SetConsoleOutputCP(CP_UTF8);   // Консоль переключается на русскую кодировку
    SetConsoleCP(CP_UTF8);

    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <файл1> <файл2> ...\n";
        return 1;
    }

    // Список файлов из аргументов командной строки
    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i) {
        files.push_back(argv[i]);
    }

    // Количество потоков = min(число файлов, число ядер процессора)
    const int POOL_SIZE = std::min(
        static_cast<int>(files.size()),
        static_cast<int>(std::thread::hardware_concurrency())
    );
    safe_print("Запускаем пул из ", POOL_SIZE, " потоков для обработки ",
               files.size(), " файлов");

    // Измерение времени (std::chrono)
    auto start_time = std::chrono::steady_clock::now();

    // Рабочие потоки
    std::vector<std::thread> thread_pool;
    thread_pool.reserve(POOL_SIZE);
    for (int i = 0; i < POOL_SIZE; ++i) {
        thread_pool.emplace_back(worker_thread, i + 1);
    }

    // Заполняем очередь задач
    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        for (const auto& f : files) {
            g_task_queue.push(f);
        }
        g_all_pushed = true; // Все файлы добавлены
    }
    g_queue_cv.notify_all(); // Будим все ожидающие потоки

    for (auto& t : thread_pool) {
        t.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();

    // Итоговая статистика
    std::cout << "  ИТОГОВАЯ СТАТИСТИКА" << std::endl;
    std::cout << "Обработано файлов : " << g_processed_count.load() << std::endl;
    std::cout << "Общее время       : " << elapsed_ms << " мс" << std::endl;
    std::cout << std::endl;

    // Статистика по каждому файлу
    {
        std::lock_guard<std::mutex> lock(g_results_mutex);
        for (const auto& s : g_results) {
            if (s.success) {
                std::cout << "  " << s.filename
                          << " — слов: "    << s.word_count
                          << ", символов: " << s.char_count << "\n";
            } else {
                std::cout << "  " << s.filename << " — [ОШИБКА ПРИ ОБРАБОТКЕ]\n";
            }
        }
    }

    return 0;
}