#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <stdexcept>
#include <atomic>
#include <vector>
#include <thread>
#include <random>
#include <mutex>

#include <windows.h>

namespace asio = boost::asio;
using executor_type = asio::io_context::executor_type;
using strand_type   = asio::strand<executor_type>;

class BankAccount {
public:
    explicit BankAccount(asio::io_context& ioc)
        : strand_(asio::make_strand(ioc))
        , balance_(0)
    {}

    // пополнение счёта
    asio::awaitable<void> async_deposit(int64_t amount) {
        co_await asio::post(strand_, asio::use_awaitable);
        balance_ += amount;
    }

    // снятие средств
    asio::awaitable<void> async_withdraw(int64_t amount) {
        co_await asio::post(strand_, asio::use_awaitable);

        if (amount > balance_) {
            throw std::invalid_argument("Insufficient funds");
        }
        balance_ -= amount;
    }

    // получение текущего баланса
    asio::awaitable<int64_t> async_get_balance() {
        co_await asio::post(strand_, asio::use_awaitable);
        co_return balance_;
    }

private:
    strand_type strand_;
    int64_t     balance_;
};

// корутина одного клиента
asio::awaitable<void> client_coroutine(
    BankAccount&       account,
    int                client_id,
    std::atomic<int>&  done_counter,
    std::atomic<int64_t>& total_deposited,
    std::atomic<int64_t>& total_withdrawn,
    std::mutex&        cout_mutex) {

    constexpr int    N_OPS   = 10;     // депозитов и снятий на каждого
    constexpr int64_t MAX_SUM = 100;   // максимальная сумма одной операции

    std::mt19937_64 rng(
        std::hash<std::thread::id>{}(std::this_thread::get_id())
        ^ static_cast<uint64_t>(client_id));
    std::uniform_int_distribution<int64_t> dist(1, MAX_SUM);

    int64_t my_deposited = 0;
    int64_t my_withdrawn = 0;

    for (int i = 0; i < N_OPS; ++i) {
        int64_t dep = dist(rng);
        co_await account.async_deposit(dep);
        my_deposited += dep;
    }

    for (int i = 0; i < N_OPS; ++i) {
        int64_t wdr = dist(rng);
        try {
            co_await account.async_withdraw(wdr);
            my_withdrawn += wdr;
        } catch (const std::invalid_argument&) {
            std::lock_guard lock(cout_mutex);
            std::cout << "[client " << client_id
                      << "] недостаточно средств для снятия " << wdr << "\n";
        }
    }

    total_deposited.fetch_add(my_deposited, std::memory_order_relaxed);
    total_withdrawn.fetch_add(my_withdrawn, std::memory_order_relaxed);

    done_counter.fetch_add(1, std::memory_order_acq_rel);
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    constexpr int N_CLIENTS = 100;
    constexpr int N_THREADS = 4;

    asio::io_context ioc;

    BankAccount account(ioc);

    std::atomic<int>     done_counter{0};
    std::atomic<int64_t> total_deposited{0};
    std::atomic<int64_t> total_withdrawn{0};
    std::mutex           cout_mutex;

    for (int i = 0; i < N_CLIENTS; ++i) {
        asio::co_spawn(
            ioc,
            client_coroutine(account, i,
                             done_counter,
                             total_deposited,
                             total_withdrawn,
                             cout_mutex),
            [&cout_mutex](std::exception_ptr ep) {
                if (ep) {
                    try { std::rethrow_exception(ep); }
                    catch (const std::exception& e) {
                        std::lock_guard lock(cout_mutex);
                        std::cerr << "[!] Неперехваченное исключение: "
                                  << e.what() << "\n";
                    }
                }
            });
    }

    std::vector<std::thread> pool;
    pool.reserve(N_THREADS);
    for (int t = 0; t < N_THREADS; ++t) {
        pool.emplace_back([&ioc]{ ioc.run(); });
    }
    for (auto& t : pool) t.join();

    asio::io_context ioc2;

    int64_t expected = total_deposited.load() - total_withdrawn.load();

    std::cout << "\n=== Итоги ===\n"
              << "Клиентов запущено : " << N_CLIENTS        << "\n"
              << "Итого пополнено   : " << total_deposited   << "\n"
              << "Итого снято       : " << total_withdrawn   << "\n"
              << "Ожидаемый баланс  : " << expected          << "\n";

    int64_t real_balance = 0;
    {
        asio::io_context ioc_final;
        asio::co_spawn(
            ioc_final,
            [&]() -> asio::awaitable<void> {
                real_balance = co_await account.async_get_balance();
            },
            asio::detached);
        ioc_final.run();
    }

    std::cout << "Реальный баланс   : " << real_balance << "\n";

    if (real_balance == expected) {
        std::cout << "✓ Баланс совпадает с расчётным — гонок нет!\n";
    } else {
        std::cout << "✗ ОШИБКА: расхождение " << (real_balance - expected) << "\n";
    }

    return 0;
}