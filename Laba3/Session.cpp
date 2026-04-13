#include "Session.h"
#include <algorithm>
#include <sstream>

Session::Session(tcp::socket socket)
    : socket_(std::move(socket)),
      strand_(socket_.get_executor()),
      timer_(socket_.get_executor()) {}

void Session::start() { do_read(); }

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(raw_buffer_, 1024),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                input_accumulator_.append(raw_buffer_, length);
                size_t pos;
                while ((pos = input_accumulator_.find('\n')) != std::string::npos) {
                    std::string line = input_accumulator_.substr(0, pos);
                    input_accumulator_.erase(0, pos + 1);
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    if (!line.empty()) process_request(line);
                }
                do_read();
            }
        });
}

std::string Session::to_upper_utf8(std::string s) {
    std::string res = "";
    for (size_t i = 0; i < s.length(); ++i) {
        unsigned char c1 = (unsigned char)s[i];
        if (c1 == 0xD0 && i + 1 < s.length()) {
            unsigned char c2 = (unsigned char)s[i+1];
            if (c2 >= 0xB0 && c2 <= 0xBF) { res += (char)0xD0; res += (char)(c2 - 0x20); }
            else { res += (char)c1; res += (char)c2; }
            i++;
        } else if (c1 == 0xD1 && i + 1 < s.length()) {
            unsigned char c2 = (unsigned char)s[i+1];
            if (c2 >= 0x80 && c2 <= 0x8F) { res += (char)0xD0; res += (char)(c2 + 0x10); }
            else { res += (char)c1; res += (char)c2; }
            i++;
        } else { res += (char)toupper(c1); }
    }
    return res;
}

void Session::process_request(const std::string& msg) {
    if (msg.substr(0, 5) == "wait ") {
        std::istringstream iss(msg.substr(5));
        int sec; std::string text;
        if (iss >> sec && std::getline(iss, text)) {
            delayed_response(sec, text); return;
        }
    }

    if (is_number_sequence(msg)) {
        std::istringstream iss(msg);
        int val; std::vector<int> nums;
        while (iss >> val) nums.push_back(val);
        if (nums.size() == 1) {
            int n = nums[0];
            auto self(shared_from_this());
            boost::asio::post(strand_, [this, self, n]() { compute_factorial(n); });
        } else {
            long long sum = 0;
            for (int n : nums) sum += n;
            do_write("Sum: " + std::to_string(sum) + "\r\n");
        }
    } else {
        do_write("Upper: " + to_upper_utf8(msg) + "\r\n");
    }
}

void Session::compute_factorial(int n) {
    if (n < 0 || n > 20) { do_write("Fact error: 0-20\r\n"); return; }
    unsigned long long res = 1;
    for(int i = 1; i <= n; ++i) res *= i;
    do_write("Fact(" + std::to_string(n) + ") = " + std::to_string(res) + "\r\n");
}

void Session::delayed_response(int seconds, const std::string& text) {
    auto self(shared_from_this());
    timer_.expires_after(std::chrono::seconds(seconds));
    timer_.async_wait([this, self, text](const boost::system::error_code& ec) {
        if (!ec) do_write("Delayed: " + text + "\r\n");
    });
}

void Session::do_write(const std::string& response) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(response), [this, self](...){});
}

bool Session::is_number_sequence(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) {
        return std::isdigit(c) || std::isspace(c) || c == '-' || c == '+';
    });
}