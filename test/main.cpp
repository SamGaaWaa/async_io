// #include "event_loop.h"

// #include "sys/epoll.h"
// #include "sys/socket.h"
// #include "sys/types.h"
// #include "netinet/in.h"
// #include "arpa/inet.h"
// #include "unistd.h"
// #include "fcntl.h"


// #include <iostream>
// #include <cstdlib>
// #include <cstring>
// #include <thread>
// #include <string>
// #include <chrono>
// #include <vector>
// #include <array>

// void set_nonblocking(int fd) {
//     auto flag = ::fcntl(fd, F_GETFL, 0);
//     flag |= O_NONBLOCK;
//     ::fcntl(fd, F_SETFL, flag);
// }


// int main(int argc, char** argv) {
//     if (argc != 3) {
//         std::cerr << "args error\n";
//         return -1;
//     }
//     const char* ip = argv[1];
//     int port = std::atoi((const char*)argv[2]);


//     int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
//     if (listen_fd == -1) {
//         std::cerr << "create socket failed\n";
//         return -1;
//     }

//     sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     inet_aton(ip, &addr.sin_addr);
//     addr.sin_port = port;

//     int res = ::bind(listen_fd, (const sockaddr*)&addr, sizeof(addr));
//     if (res == EACCES) {
//         std::cerr << "EACCES\n";
//         return -1;
//     }
//     else if (res == EADDRINUSE) {
//         std::cerr << "EADDRINUSE\n";
//         return -1;
//     }

//     res = ::listen(listen_fd, 1000);
//     if (res == -1) {
//         std::cerr << "listen failed\n";
//         return -1;
//     }

//     epoll_event ev;
//     auto ev_loop = async::net::event_loop::get_event_loop();

//     ev.events = EPOLLIN;
//     ev.data.fd = listen_fd;
//     if (ev_loop.event_add(listen_fd, &ev)) {
//         std::cerr << "epoll_ctl add failed, fd=" << listen_fd << std::endl;
//         return -1;
//     }

//     socklen_t addrlen;
//     while (true) {
//         auto events = ev_loop.select(1024);

//         for (auto& event : events) {
//             if (event.data.fd == listen_fd) {
//                 auto conn_sock = ::accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
//                 if (conn_sock == -1) {
//                     std::cerr << "accept error\n";
//                     return -1;
//                 }

//                 set_nonblocking(conn_sock);

//                 ev.events = EPOLLIN;
//                 ev.data.fd = conn_sock;
//                 if (ev_loop.event_add(conn_sock, &ev)) {
//                     std::cerr << "epoll_ctl add failed, fd=" << conn_sock << std::endl;
//                     return -1;
//                 }
//             }
//             else {
//                 if (event.events & EPOLLIN) {
//                     std::array<char, 8> tmp;
//                     res = ::read(event.data.fd, (void*)tmp.data(), tmp.size() - 1);
//                     if (res > 0) {
//                         tmp[res] = '\0';
//                         std::cout << "recv: " << tmp.data() << std::endl;
//                     }

//                     /**
//                      *  测试epoll_ctl在水平模式下是否会丢失数据
//                      *  结果表明，不会！
//                      */
//                     ev.events = EPOLLIN;
//                     ev.data.fd = event.data.fd;
//                     ev_loop.event_add(event.data.fd, &ev);
//                 }
//             }


//         }

//     }

// }





// #include "socket.h"
// #include "proactor.h"
// #include "exception.h"

// #include <iostream>
// #include <array>
// #include <memory>

// int main() {
//     async::net::tcp::proactor executor;
//     async::net::tcp::socket sock{ executor };

//     auto err = sock.bind("127.0.0.1", 8022);
//     if (err) {
//         std::cerr << "bind error\n" << err.what() << '\n';
//         return -1;
//     }

//     err = sock.listen();
//     if (err) {
//         std::cerr << "listen error\n" << err.what() << '\n';
//         return -1;
//     }

//     auto s_move = std::move(sock);

//     std::array<char, 1024> buffer;

//     s_move.async_accept([&](async::net::tcp::socket<> socket, async::error_code err) {
//         if (socket.valid()) {
//             auto client = std::make_shared<async::net::tcp::socket<>>(std::move(socket));

//             client->async_read(buffer.data(), buffer.size() - 1, [&, client](int size, async::error_code err) {
//                 if (!err and size > 0) {
//                     buffer[size] = '\0';
//                     std::cout << "Has received " << size << " bytes: " << buffer.data() << '\n';
//                 }
//                 });
//         }
//         else {
//             std::cout << "accept error: " << err.what() << '\n';
//         }
//         });
//     std::cout << "no blocks\n";

//     executor.run();

// }


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