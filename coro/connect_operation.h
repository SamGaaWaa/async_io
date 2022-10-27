#ifndef __CONNECT_OPERATION_H__
#define __CONNECT_OPERATION_H__

#include "sys/socket.h"

#include "endpoint.h"
#include "exception.h"

#include <coroutine>

namespace async::net::tcp {

    template<class Socket>
    struct connect_operation {

        connect_operation(Socket& socket, endpoint& ep)noexcept:_socket{ socket }, _ep{ ep } {}

        bool await_ready()noexcept {
            while (true) {
                auto res = ::connect(_socket.native_handle(), (const sockaddr*)(&_ep.address()), _ep.size());
                if (-1 == res) {
                    if (errno == EINTR)
                        continue;
                    else if (errno == EINPROGRESS or errno == EAGAIN) {//正在连接中，放进epoll监听
                        _error = error_code{ errno };
                        return false;
                    }
                    else {//出错
                        _error = error_code{ errno };
                        return true;
                    }
                }
                else {//连接成功
                    _error = error_code{ 0 };
                    return true;
                }

            }
        }

        bool await_suspend(std::coroutine_handle<> h)noexcept {
            auto err = _socket.async_connect(_ep, h);
            if (err) {//加入epoll出错
                _error = err;
                return false;
            }
            return true;
        }

        error_code await_resume()noexcept {
            if ((int)_error == EINPROGRESS or (int)_error == EAGAIN) {//
                int err;
                socklen_t err_len = sizeof(err);
                ::getsockopt(_socket.native_handle(), SOL_SOCKET, SO_ERROR, &err, &err_len);
                return err;
            }

            return _error;
        }

        private:
        error_code _error;
        Socket& _socket;
        endpoint& _ep;
    };


}

#endif