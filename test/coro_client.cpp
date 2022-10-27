#include "socket.h"
#include "endpoint.h"
#include "coro/task.h"

#include <array>
#include <iostream>
#include <string_view>

using namespace async::net;

auto getline(char* begin, size_t size)noexcept {
    char c;
    char* iter = begin;
    while (iter != (begin + size) and (c = std::getchar()) != '\n')
        *iter++ = c;
    return iter - begin;
}

async::task echo_session(tcp::socket<> socket) {
    endpoint ep{ "127.0.0.1", 8022 };

    auto err = co_await socket.async_connect(ep);
    if (err) {
        std::cerr << "Connect fail. " << err.what() << '\n';
        co_return;
    }

    std::array<char, 1024> buffer;

    while (true) {
        auto count = getline(buffer.data(), buffer.size() - 1);
        if (count == 0)
            continue;
        auto [read, err] = co_await socket.async_write(buffer.data(), count);
        if (err) {
            std::cerr << "Write error. " << err.what() << '\n';
            co_return;
        }

        auto result = co_await socket.async_read(buffer.data(), buffer.size() - 1);
        if (auto err = std::get<1>(result)) {
            std::cerr << "Read error. " << err.what() << '\n';
            co_return;
        }
        buffer[std::get<0>(result)] = '\0';
        std::cout << ">>>" << buffer.data() << '\n';
    }

}

int main() {
    tcp::proactor executor;
    tcp::socket<> socket{ executor };

    auto task = echo_session(std::move(socket));
    executor.run();
}