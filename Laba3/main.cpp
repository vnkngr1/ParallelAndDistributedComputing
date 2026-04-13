#include <iostream>
#include <boost/asio.hpp>
#include <windows.h>
#include "Server.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        boost::asio::io_context io_context;
        Server s(io_context, 12345);
        std::cout << "SERVER READY" << std::endl;
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}