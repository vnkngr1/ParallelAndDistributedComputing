#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename T>
class Account {
private:
    T balance;
    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    Account() : balance(0) {}

    Account(T initial_balance) : balance(initial_balance) {}

    Account(const Account& other) {
        std::lock_guard<std::mutex> lock(other.mtx);
        balance = other.balance;
    }

    Account& operator=(const Account& other) {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(other.mtx);
            balance = other.balance;
        }
        return *this;
    }

    Account(Account&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mtx);
        balance = other.balance;
    }

    Account& operator=(Account&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(other.mtx);
            balance = other.balance;
        }
        return *this;
    }

    T get_balance() const {
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }

    void deposit(T amount) {
        std::lock_guard<std::mutex> lock(mtx);
        balance += amount;
        cv.notify_all();
    }

    bool withdraw(T amount) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this, amount]() { return balance >= amount; });
        balance -= amount;
        return true;
    }

    std::mutex& get_mutex() {
        return mtx;
    }

    std::condition_variable& get_cv() {
        return cv;
    }

    T get_balance_unsafe() const {
        return balance;
    }

    void withdraw_unsafe(T amount) {
        balance -= amount;
    }
};

template <typename T>
class Bank {
private:
    std::vector<Account<T>> accounts;
    std::atomic<size_t> completed_transfers{0};
    mutable std::mutex print_mutex;

public:
    Bank(const std::vector<T>& initial_balances) {
        accounts.reserve(initial_balances.size());
        for (T balance : initial_balances) {
            accounts.emplace_back(balance);
        }
    }

    inline bool transfer_impl(int from, int to, T amount) {
        if (from == to || amount < 0) return false;

        int first = std::min(from, to);
        int second = std::max(from, to);

        std::unique_lock<std::mutex> lock1(accounts[first].get_mutex());
        std::unique_lock<std::mutex> lock2(accounts[second].get_mutex());

        if (from < to) {
            accounts[from].get_cv().wait(lock1, [this, from, amount]() {
                return accounts[from].get_balance_unsafe() >= amount;
            });
        } else {
            accounts[from].get_cv().wait(lock2, [this, from, amount]() {
                return accounts[from].get_balance_unsafe() >= amount;
            });
        }

        accounts[from].withdraw_unsafe(amount);
        accounts[to].deposit(amount);

        return true;
    }

    void transfer(int from, int to, T amount) {
        bool success = transfer_impl(from, to, amount);

        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << "Thread " << std::this_thread::get_id()
                  << ": Transfer " << amount << " from " << from
                  << " to " << to << (success ? " SUCCESS" : " FAILED") << std::endl;

        ++completed_transfers;
    }

    T get_total_balance() const {
        T total = 0;
        for (const auto& acc : accounts) {
            total += acc.get_balance();
        }
        return total;
    }

    void print_balances() const {
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << "Balances: ";
        for (size_t i = 0; i < accounts.size(); ++i) {
            std::cout << "[" << i << "]=" << accounts[i].get_balance() << " ";
        }
        std::cout << std::endl;
    }
};

int main() {
    std::vector<double> initial = {1000.0, 500.0, 300.0, 200.0};
    Bank<double> bank(initial);

    std::cout << "Initial total: " << bank.get_total_balance() << std::endl;

    std::thread t1([&bank]() {
        bank.transfer(0, 1, 100.0);
    });

    std::thread t2([&bank]() {
        bank.transfer(1, 2, 50.0);
    });

    std::thread t3([&bank]() {
        bank.transfer(2, 3, 30.0);
    });

    std::thread t4([&bank]() {
        bank.transfer(0, 3, 200.0);
    });

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    bank.print_balances();
    std::cout << "Final total: " << bank.get_total_balance() << std::endl;

    return 0;
}
