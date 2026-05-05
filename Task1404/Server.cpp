#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <windows.h>

using boost::asio::ip::tcp;

int global_message_count = 0;

std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() {
        auto ep = socket_.remote_endpoint();
        std::cout << "[+] Беспилотник подключился: "
                  << ep.address().to_string() << ":" << ep.port() << std::endl;
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());

        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            boost::asio::bind_executor(
                *strand,
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        std::string msg(data_, length);

                        ++global_message_count;

                        std::cout << ">> Получено от беспилотника: " << msg
                                  << "   (всего сообщений: "
                                  << global_message_count << ")\n";

                        do_write(length);   // эхо обратно
                    } else {
                        std::cout << "[-] Сессия завершена: "
                                  << ec.message() << std::endl;
                    }
                }));
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());

        boost::asio::async_write(
            socket_,
            boost::asio::buffer(data_, length),
            boost::asio::bind_executor(
                *strand,
                [this, self](boost::system::error_code ec,
                              std::size_t /*bytes_transferred*/) {
                    if (!ec) {
                        do_read();
                    }
                }));
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length]{};
};

class Server {
public:
    Server(boost::asio::io_context& io_ctx, short port)
        : acceptor_(io_ctx, tcp::endpoint(tcp::v4(), port)) {
        std::cout << "[*] Сервер запущен на порту " << port << std::endl;
        do_accept();
    }

private:
    void do_accept() {

        acceptor_.async_accept(
            boost::asio::make_strand(acceptor_.get_executor()),
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                } else {
                    std::cerr << "Ошибка accept: " << ec.message() << std::endl;
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        boost::asio::io_context io_ctx;

        strand = std::make_shared<
            boost::asio::strand<boost::asio::io_context::executor_type>>(
            io_ctx.get_executor());

        Server server(io_ctx, 12345);

        const int num_threads = 4;
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&io_ctx]() { io_ctx.run(); });
        }

        std::cout << "[*] Запущено потоков: " << num_threads << std::endl;
        std::cout << "[*] Ожидаем подключений беспилотников...\n";

        for (auto& t : threads) {
            t.join();
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка сервера: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}