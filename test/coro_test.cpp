#include "socket.h"
#include "coro/task.h"
#include "proactor.h"
#include "exception.h"

#include <array>
#include <iostream>
#include <thread>


async::task echo_session(async::net::tcp::socket<> socket) {
    std::array<char, 128> buffer;

    std::variant res = co_await socket.async_accept();

    auto client = std::get_if<0>(&res);
    if (!client) {
        std::cerr << "accept error.\n" << std::get<1>(res).what() << '\n';
        co_return;
    }

    while (true) {
        auto [res, err] = co_await client->async_read(buffer.data(), buffer.size() - 1);
        if (!err) {
            if (res == 0) {
                std::cout << "Connection break.\n";
                co_return;
            }
            buffer[res] = '\0';
            std::cout << "coroutine read: " << buffer.data() << '\n';
        }
        else {
            std::cerr << "coroutine error.\n";
            std::cout << "err=" << (int)err << err.what() << '\n';
            continue;
        }

        co_await client->async_write(buffer.data(), res);
    }
}


int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "args error\n";
        return -1;
    }
    const char* ip = argv[1];
    int port = std::atoi((const char*)argv[2]);

    async::net::tcp::proactor executor;
    async::net::tcp::socket<> socket{ executor };

    auto err = socket.bind(ip, port);
    if (err) {
        std::cerr << "Bind error.\n";
        return -1;
    }

    err = socket.listen();
    if (err) {
        std::cerr << "Listen error.\n";
        return -1;
    }

    auto task = echo_session(std::move(socket));

    std::thread other_thread = std::thread{ [&] {executor.run(); } };
    executor.run();
    other_thread.join();

}