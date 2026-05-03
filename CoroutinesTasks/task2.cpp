#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include <iostream>
#include <string>
#include <variant>
#include <chrono>

#include <windows.h>

namespace asio = boost::asio;
using tcp      = asio::ip::tcp;
using namespace asio::experimental::awaitable_operators;
using namespace std::chrono_literals;

// принимает одно подключение и периодически шлёт сообщения
asio::awaitable<void> mock_server(unsigned short port,
                                  std::string    source_name,
                                  int            message_count) {
    auto ex = co_await asio::this_coro::executor;

    tcp::acceptor acceptor(ex, tcp::endpoint(tcp::v4(), port));
    acceptor.set_option(asio::socket_base::reuse_address(true));

    tcp::socket client = co_await acceptor.async_accept(asio::use_awaitable);

    for (int i = 1; i <= message_count; ++i) {
        asio::steady_timer timer(ex);
        timer.expires_after(port == 7001 ? 400ms : 700ms);
        co_await timer.async_wait(asio::use_awaitable);

        std::string msg = "[" + source_name + "] сообщение #" + std::to_string(i) + "\n";
        co_await asio::async_write(client, asio::buffer(msg), asio::use_awaitable);
    }

    client.close();
    std::cout << "[server:" << port << "] завершил работу\n";
}

// мультиплексер
struct ReadResult {
    std::string name;
    std::string data;
};

asio::awaitable<ReadResult> read_from(tcp::socket& sock, std::string name) {
    char buf[4096];
    auto [ec, n] = co_await sock.async_read_some(
        asio::buffer(buf),
        asio::as_tuple(asio::use_awaitable));

    if (ec) co_return ReadResult{std::move(name), {}};
    co_return ReadResult{std::move(name), std::string(buf, n)};
}

asio::awaitable<void> multiplexer(tcp::socket sock1, tcp::socket sock2) {
    bool open1 = true, open2 = true;

    while (open1 || open2) {
        if (open1 && open2) {
            auto result = co_await (
                read_from(sock1, "sock1") ||
                read_from(sock2, "sock2")
            );
            std::visit([&](ReadResult& r) {
                if (r.data.empty()) {
                    std::cout << "[" << r.name << "] соединение закрыто\n";
                    if (r.name == "sock1") open1 = false;
                    else                   open2 = false;
                } else {
                    std::cout << r.data;
                }
            }, result);
        } else if (open1) {
            auto r = co_await read_from(sock1, "sock1");
            if (r.data.empty()) { open1 = false; std::cout << "[sock1] соединение закрыто\n"; }
            else std::cout << r.data;
        } else {
            auto r = co_await read_from(sock2, "sock2");
            if (r.data.empty()) { open2 = false; std::cout << "[sock2] соединение закрыто\n"; }
            else std::cout << r.data;
        }
    }

    std::cout << "\n[multiplexer] оба источника закрыты — выходим.\n";
}

// подключение к локальному серверу
asio::awaitable<tcp::socket> async_connect(unsigned short port) {
    auto ex = co_await asio::this_coro::executor;
    tcp::socket sock(ex);

    asio::steady_timer t(ex);
    t.expires_after(50ms);
    co_await t.async_wait(asio::use_awaitable);

    co_await sock.async_connect(
        tcp::endpoint(asio::ip::make_address("127.0.0.1"), port),
        asio::use_awaitable);

    co_return std::move(sock);
}

// главная корутина
asio::awaitable<void> run_demo() {
    auto ex = co_await asio::this_coro::executor;

    // запускаем мини-сервера
    asio::co_spawn(ex, mock_server(7001, "SOURCE-A", 5), asio::detached);
    asio::co_spawn(ex, mock_server(7002, "SOURCE-B", 3), asio::detached);

    asio::steady_timer delay(ex);
    delay.expires_after(100ms);
    co_await delay.async_wait(asio::use_awaitable);

    tcp::socket sock1 = co_await async_connect(7001);
    tcp::socket sock2 = co_await async_connect(7002);

    std::cout << "[demo] подключено к обоим источникам\n\n";

    co_await multiplexer(std::move(sock1), std::move(sock2));
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        asio::io_context ioc;
        asio::co_spawn(ioc, run_demo(),
            [](std::exception_ptr ep) {
                if (ep) std::rethrow_exception(ep);
            });
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
}