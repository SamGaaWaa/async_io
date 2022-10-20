#ifndef __READ_OPERATION_H__
#define __READ_OPERATION_H__

#include <coroutine>
#include <stddef.h>
#include <unistd.h>
#include <tuple>
#include <functional>

#include "exception.h"

namespace async::net::tcp {

    template<class Executor>
    struct read_operation {
        int fd;
        char* buffer;
        int size;
        error_code error;


        bool await_ready()noexcept {
            auto res = ::read(fd, (void*)buffer, size);
            if (res == -1) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//read阻塞，放进epoll监听

                }
                else {//read出错
                    size = -1;
                    error = error_code{};
                    return true;
                }
            }
            else {//读取成功
                size = res;
                error = { 0 };
                return true;
            }
        }

        auto await_suspend(std::coroutine_handle<> h)noexcept {

        }

        auto await_resume()noexcept {
            return std::tuple{ size, error };
        }
    };

}
#endif