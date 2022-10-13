
/**
 *  echo server
 *  @author SamGaaWaa
 */

#include "socket.h"
#include "proactor.h"
#include "exception.h"

#include <iostream>
#include <array>
#include <memory>
#include <ranges>

class connection: public std::enable_shared_from_this<connection> {
    public:
    explicit connection(async::net::tcp::socket<> socket)noexcept:_socket{ std::move(socket) } {}

    void start() {
        do_read(_buffer.data(), _buffer.size());
    }

    private:
    void do_read(char* buff, size_t size) {
        auto self{ shared_from_this() };
        _socket.async_read(buff, size, [this, self, buff](int s, async::error_code err) {
            if (err or s == 0) {
                std::cout << "read error: " << err.what() << '\n';
                return;
            }
        for (auto i{ 0 }; i < s; ++i)
            if (std::isalpha(buff[i]))
                buff[i] = std::toupper(buff[i]);
        do_write(buff, (size_t)s);
            });
    }

    void do_write(char* buff, size_t size) {
        auto self{ shared_from_this() };
        _socket.async_write(buff, size, [this, self, buff, size](int s, async::error_code err) {
            if (err or s == 0) {
                std::cout << "write error: " << err.what() << '\n';
                return;
            }
            else if (s < size) {
                do_write(buff + s, size - s);
            }
            else
                do_read(_buffer.data(), _buffer.size());
            });
    }

    async::net::tcp::socket<> _socket;
    std::array<char, 1024> _buffer;
};

class server {
    public:

    explicit server(async::net::tcp::proactor& executor, const char* ip, int port):_socket{ executor } {
        auto err = _socket.bind(ip, port);
        if (err) {
            std::cerr << "bind error: " << err.what() << '\n';
            throw err;
        }

        err = _socket.listen();
        if (err) {
            std::cerr << "listen error\n" << err.what() << '\n';
            throw err;
        }
        do_accept();
    }



    private:
    void do_accept() {
        _socket.async_accept([this](async::net::tcp::socket<> client, async::error_code err) {
            if (!err and client.valid()) {
                std::make_shared<connection>(std::move(client))->start();
            }
            else {
                std::cout << "accept error: " << err.what() << " : " << client.native_handle() << '\n';
            }
        do_accept();
            });
    }

    async::net::tcp::socket<> _socket;
};


int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "args error\n";
        return -1;
    }
    const char* ip = argv[1];
    int port = std::atoi((const char*)argv[2]);

    async::net::tcp::proactor executor;
    server s{ executor, ip, port };

    executor.run();
}