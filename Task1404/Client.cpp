#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <windows.h>

using boost::asio::ip::tcp;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(boost::asio::io_context& io_ctx,
           const tcp::resolver::results_type& endpoints)
        : socket_(io_ctx), endpoints_(endpoints) {}

    void connect() { do_connect(endpoints_); }

    void send_message(const std::string& message) {
        auto self(shared_from_this());

        boost::asio::async_write(
            socket_,
            boost::asio::buffer(message),
            [this, self, message](boost::system::error_code ec,
                                   std::size_t /*bytes*/) {
                if (!ec) {
                    std::cout << ">> Отправлено: " << message << std::endl;
                    do_read();
                } else {
                    std::cerr << "Ошибка отправки: " << ec.message() << std::endl;
                }
            });
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints) {
        auto self(shared_from_this());

        boost::asio::async_connect(
            socket_, endpoints,
            [this, self](boost::system::error_code ec, tcp::endpoint ep) {
                if (!ec) {
                    std::cout << "[+] Подключился к серверу: "
                              << ep.address().to_string()
                              << ":" << ep.port() << std::endl;
                    std::cout << "    Введите координаты (пусто — выход):\n";
                } else {
                    std::cerr << "Ошибка подключения: " << ec.message() << std::endl;
                }
            });
    }

    void do_read() {
        auto self(shared_from_this());

        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout << "<< Эхо от сервера: "
                              << std::string(data_, length) << std::endl;
                } else {
                    std::cerr << "Ошибка приёма: " << ec.message() << std::endl;
                }
            });
    }

    tcp::socket socket_;
    tcp::resolver::results_type endpoints_;
    enum { max_length = 1024 };
    char data_[max_length]{};
};

int main(int argc, char* argv[]) {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    const std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    const std::string port = (argc > 2) ? argv[2] : "12345";

    try {
        boost::asio::io_context io_ctx;

        tcp::resolver resolver(io_ctx);
        auto endpoints = resolver.resolve(host, port);

        auto client = std::make_shared<Client>(io_ctx, endpoints);
        client->connect();

        std::thread io_thread([&io_ctx]() { io_ctx.run(); });

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        std::string message;
        while (true) {
            std::cout << "Координаты (lat,lon): ";
            if (!std::getline(std::cin, message) || message.empty()) {
                break;
            }
            client->send_message(message);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        io_ctx.stop();
        io_thread.join();

        std::cout << "[*] Клиент завершён.\n";
    } catch (const std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}