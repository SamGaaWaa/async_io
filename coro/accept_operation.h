#ifndef __ACCEPT_OPERATION_H__
#define __ACCEPT_OPERATION_H__

#include "exception.h"

#include "sys/socket.h"

#include <unistd.h>
#include <coroutine>
#include <variant>
#include "experimental/socket"

namespace async::net::tcp {

    template<class Socket>
    struct accept_operation {
        using socket_type = Socket;

        accept_operation(socket_type& socket)noexcept:_socket{ socket }, _result{} {}

        bool await_ready()noexcept {
            auto res = ::accept(_socket.native_handle(), NULL, NULL);
            if (res == -1) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//accept阻塞，放进epoll监听
                    return false;
                }
                //出错
                _result = error_code{};
                return true;
            }
            else {//accept成功
                _result.template emplace<1>(_socket.get_executor(), res);
                return true;
            }
        }

        bool await_suspend(std::coroutine_handle<> h)noexcept {
            auto err = _socket.async_accept(h);
            if (err) {      //加入epoll出错，*this安全
                _result = err;
                return false;
            }
            return true;
        }

        std::variant<socket_type, error_code> await_resume()noexcept {
            if (_result.index() == 0) {
                auto res = ::accept(_socket.native_handle(), NULL, NULL);
                if (res == -1) {
                    return { error_code{} };
                }
                return { socket_type{_socket.get_executor(), res} };
            }
            if (auto s = std::get_if<socket_type>(&_result)) {
                return { std::move(*s) };
            }
            return { *std::get_if<error_code>(&_result) };
        }

        private:
        std::variant<std::monostate, socket_type, error_code> _result;
        socket_type& _socket;

    };



}

#endif