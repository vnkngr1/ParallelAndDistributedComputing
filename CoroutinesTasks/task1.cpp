//    скрипт в командную строку для теста
//
//    $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", 55555)
//    $stream = $client.GetStream()
//    $writer = New-Object System.IO.StreamWriter($stream)
//    $reader = New-Object System.IO.StreamReader($stream)
//    $writer.AutoFlush = $true
//
//    # Отправляем несколько сообщений и читаем эхо
//    foreach ($msg in @("Привет!", "Тест 123", "Это эхо-сервер")) {
//        $writer.WriteLine($msg)
//        Start-Sleep -Milliseconds 100
//        $response = $reader.ReadLine()
//        Write-Host "Отправлено : $msg"
//        Write-Host "Получено   : $response"
//        Write-Host "---"
//    }
//
//    $client.Close()
//    Write-Host "Соединение закрыто."

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/write.hpp>
#include <iostream>

#include <windows.h>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// корутина одной сессии
asio::awaitable<void> echo_session(tcp::socket sock) {
    // адрес клиента
    const auto remote = sock.remote_endpoint();
    std::cout << "[+] Новое подключение: " << remote << "\n";

    char data[1024];

    try {
        for (;;) {
            auto [ec, n] = co_await sock.async_read_some(
                asio::buffer(data),
                asio::as_tuple(asio::use_awaitable));

            if (ec == asio::error::eof) {
                std::cout << "[-] Клиент отключился: " << remote << "\n";
                break;
            }

            if (ec) throw boost::system::system_error(ec);

            // эхо
            co_await asio::async_write(
                sock,
                asio::buffer(data, n),
                asio::use_awaitable);
        }
    } catch (const std::exception& e) {
        std::cerr << "[!] Ошибка в сессии " << remote
                  << ": " << e.what() << "\n";
    }
}

// корутина на прием входящих соединений
asio::awaitable<void> listener(unsigned short port) {
    auto executor = co_await asio::this_coro::executor;

    tcp::acceptor acceptor(executor,
                           tcp::endpoint(tcp::v4(), port));
    acceptor.set_option(asio::socket_base::reuse_address(true));

    std::cout << "[*] Сервер запущен, слушает порт " << port << "\n";

    for (;;) {
        tcp::socket sock = co_await acceptor.async_accept(asio::use_awaitable);

        asio::co_spawn(executor,
                       echo_session(std::move(sock)),
                       asio::detached);
    }
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        asio::io_context ioc;
        asio::co_spawn(ioc, listener(55555), asio::detached);
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
}