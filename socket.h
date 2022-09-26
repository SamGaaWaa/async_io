#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "sys/socket.h"
#include "fcntl.h"
#include "unistd.h"

#include "exception.h"
#include "thread_pool.h"
#include "endpoint.h"

#include <optional>
#include <cstring>
#include <limits>

namespace async::net::tcp {

    template<class Executor = thread_pool, bool Block = false>
    class socket {
        public:
        using executor_type = Executor;
        static constexpr bool block = Block;

        socket(executor_type& executor) :_executor{ executor } {
            _fd = ::socket(PF_INET, SOCK_STREAM, 0);
            if (_fd == -1) {
                throw bad_socket{};
            }
            if constexpr (!Block) {
                set_nonblocking();
            }
        }

        socket(executor_type& executor, int fd) :_executor{ executor }, _fd{ fd } {
            if (fd == -1)
                throw bad_socket{};
            if constexpr (!Block) {
                set_nonblocking();
            }
            else {
                set_blocking();
            }
        };

        socket(const socket&) = delete;
        socket& operator=(const socket&) = delete;

        socket(socket&&)noexcept = default;
        socket& operator=(socket&&)noexcept = default;

        ~socket() { ::close(_fd); }


        int native_handle()const noexcept { return _fd; }
        executor_type& get_executor()noexcept { return _executor; }

        std::optional<error_code> bind(endpoint& address)noexcept {
            auto res = ::bind(_fd, (const sockaddr*)&address.address(), sizeof(address.address()));
            return error_code::get_error_code(res);
        }

        std::optional<error_code> bind(const char* ip, int port)noexcept {
            sockaddr_in address;
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_port = port;
            inet_aton(ip, &address.sin_addr);

            auto res = ::bind(_fd, (const sockaddr*)&address, sizeof(address));
            return error_code::get_error_code(res);
        }

        std::optional<error_code> bind(const std::string& ip, int port)noexcept {
            return bind(ip.c_str(), port);
        }

        std::optional<error_code> listen(int backlog = std::numeric_limits<int>::max())noexcept {
            auto res = ::listen(_fd, backlog);
            return error_code::get_error_code(res);
        }

        std::optional<socket> accept(error_code& code)noexcept {
            static_assert(Block);
            auto res = ::accept(_fd, NULL, NULL);
            auto opt = error_code::get_error_code(res);
            if (opt) {
                code = *opt;
                return std::nullopt;
            }
            return socket{ _executor, res };
        }

        private:

        void set_nonblocking()noexcept {
            auto flag = ::fcntl(_fd, F_GETFL, 0);
            flag |= O_NONBLOCK;
            ::fcntl(_fd, F_SETFL, flag);
        }

        void set_blocking()noexcept {
            auto flag = ::fcntl(_fd, F_GETFL, 0);
            flag &= ~O_NONBLOCK;
            ::fcntl(_fd, F_SETFL, flag);
        }

        int _fd;
        executor_type& _executor;
    };


}

#endif