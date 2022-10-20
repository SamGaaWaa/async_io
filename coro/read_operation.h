#ifndef __READ_OPERATION_H__
#define __READ_OPERATION_H__

#include <coroutine>
#include <stddef.h>
#include <unistd.h>
#include <tuple>
#include <iostream>

#include "exception.h"
#include "socket.h"

namespace async::net::tcp {

    template<class Socket>
    struct read_operation {

        read_operation(char* buff, size_t max_size, Socket& socket)noexcept:
            _buffer{ buff },
            _max_size{ max_size },
            _socket{ socket } {}

        bool await_ready()noexcept {
            std::cout << "await_ready.\n";
            auto res = ::read(_socket.native_handle(), (void*)_buffer, _max_size);
            if (res == -1) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//read阻塞，放进epoll监听
                    _state = LISTENING;
                    return false;
                }
                else {//read出错
                    _result = -1;
                    _error = error_code{};
                    _state = ERROR;
                    return true;
                }
            }
            else {//读取成功
                _result = res;
                _error = { 0 };
                _state = READY;
                return true;
            }
        }

        bool await_suspend(std::coroutine_handle<> h)noexcept {
            std::cout << "await_suspend.\n";
            auto err = _socket.async_read(_buffer, _max_size, h);
            if (err) {              //加入epoll出错，说明不会在其他线程恢复，*this不会被析构
                _state = ERROR;
                _result = -1;
                _error = err;
                std::cerr << err.what() << '\n';
                return false;
            }
            std::cout << "Waiting.\n";
            return true;
        }

        auto await_resume()noexcept {
            std::cout << "await_resume.\n";
            if (_state == READY or _state == ERROR) {
                return std::tuple{ _result, _error };
            }
            auto res = ::read(_socket.native_handle(), (void*)_buffer, _max_size);
            if (res == -1) {
                _result = -1;
                _error = error_code{};
            }
            else {//读取成功
                _result = res;
                _error = { 0 };
            }
            return std::tuple{ _result, _error };
        }

        private:
        char* _buffer;
        size_t      _max_size;
        int         _result;
        error_code  _error;
        enum { READY, ERROR, LISTENING }_state;
        Socket& _socket;
    };

}
#endif