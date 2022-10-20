#include "socket.h"
#include "coro/task.h"
#include "proactor.h"
#include "exception.h"

#include <array>
#include <iostream>
#include <thread>

async::task read_task(async::net::tcp::socket<> socket) {
    std::cout << "Start coroutine.\n";
    std::array<char, 128> tmp;
    auto [res, err] = co_await socket.async_read(tmp.data(), tmp.size() - 1);
    if (!err) {
        tmp[res] = '\0';
        std::cout << "coroutine read: " << tmp.data() << '\n';
    }
    else {
        std::cerr << "coroutine error.\n";
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

    socket.async_accept([ ](async::net::tcp::socket<> client, async::error_code err) {
        if (!err) {
            auto task = read_task(std::move(client));
            while (!task.done());
        }
        else {
            std::cerr << "Accept error.\n";
        }
        });

    std::thread other_thread = std::thread{ [&] {executor.run(); } };
    executor.run();
    other_thread.join();

}