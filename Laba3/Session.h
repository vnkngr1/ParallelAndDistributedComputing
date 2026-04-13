#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket);
    void start();

private:
    void do_read();
    void process_request(const std::string& msg);
    void do_write(const std::string& response);

    void compute_factorial(int n);
    void delayed_response(int seconds, const std::string& text);

    bool is_number_sequence(const std::string& s);
    std::string to_upper_utf8(std::string s);

    tcp::socket socket_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::steady_timer timer_;
    char raw_buffer_[1024];
    std::string input_accumulator_;
};

#endif