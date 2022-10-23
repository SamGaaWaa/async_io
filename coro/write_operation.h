#ifndef __WRITE_OPERATION_H__
#define __WRITE_OPERATION_H__

#include <unistd.h>
#include <tuple>
#include <coroutine>

#include "exception.h"


namespace async::net::tcp {

    template<class Socket>
    struct write_operation {

        write_operation(char* buff, size_t size, Socket& socket)noexcept:
            _buffer{ buff }, _size{ size }, _socket{ socket } {}


        bool await_ready()noexcept {
            auto res = ::write(_socket.native_handle(), (void*)_buffer, _size);
            if (res == -1) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//write阻塞，放进epoll监听
                    _state = LISTENING;
                    return false;
                }
                else {//write出错
                    _result = -1;
                    _error = error_code{};
                    _state = ERROR;
                    return true;
                }
            }
            else {//write成功
                _result = res;
                _error = { 0 };
                _state = READY;
                return true;
            }
        }

        bool await_suspend(std::coroutine_handle<> h)noexcept {
            auto err = _socket.async_write(_buffer, _size, h);
            if (err) {              //加入epoll出错，说明不会在其他线程恢复，*this不会被析构
                _state = ERROR;
                _result = -1;
                _error = err;
                // std::cerr << err.what() << '\n';
                return false;
            }
            return true;
        }

        auto await_resume()noexcept {
            if (_state == READY or _state == ERROR) {
                return std::tuple{ _result, _error };
            }
            auto res = ::write(_socket.native_handle(), (void*)_buffer, _size);
            if (res == -1) {
                _result = -1;
                _error = error_code{};
            }
            else {//write成功
                _result = res;
                _error = { 0 };
            }
            return std::tuple{ _result, _error };
        }

        private:

        char* _buffer;
        size_t  _size;
        int _result;
        error_code _error;
        enum { READY, ERROR, LISTENING }_state;
        Socket& _socket;
    };

}
#endif