#ifndef __PROACTOR_H__
#define __PROACTOR_H__


#include "thread_pool.h"
#include "event_loop.h"
#include "exception.h"
#include "socket.h"

#include <concepts>
#include <type_traits>
#include <coroutine>
#include <functional>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>

namespace async::net {

    class proactor;

    template<typename CallBack>
    concept read_write_callback = requires (CallBack c, size_t size, error_code code) {
        c(size, code);
    };

    template<typename CallBack>
    concept accept_callback = requires(std::optional<async::net::tcp::socket<proactor, false>> s, error_code code, CallBack c) {
        c(s, code);
    };

    class proactor {
        public:
        struct use_coroutine {};

        proactor() :_loop{ event_loop::get_event_loop() } {}

        void run() {
            std::call_once(_flag, [ this ]() {init_thread(); });
        }

        template<read_write_callback CallBack>
        auto async_read(int fd, auto* buffer, size_t size, CallBack&& callback) {
            epoll_event ev;
            ev.events = EPOLLIN;

            if constexpr (std::is_same_v<CallBack, use_coroutine>) {

            }
            else {
                auto func_ptr = std::make_unique<std::function<void()>>([ fd, buffer, size,
                    callback = std::forward<CallBack>(callback) ]() {
                        auto res = ::read(fd, (void*)buffer, size);
                        if (res == -1) {
                            callback(0, *error_code::get_error_code(errno));
                        }
                        else {
                            callback(res, error_code{ 0 });
                        }
                    });

                ev.data.ptr = (void*)func_ptr.release();
                _loop.event_add(fd, &ev);
            }
        }


        template<read_write_callback CallBack>
        auto async_write(int fd, auto* buffer, size_t size, CallBack&& callback) {
            epoll_event ev;
            ev.events = EPOLLOUT;

            if constexpr (std::is_same_v<CallBack, use_coroutine>) {

            }
            else {
                auto func_ptr = std::make_unique<std::function<void()>>([ fd, buffer, size,
                    callback = std::forward<CallBack>(callback) ]() {
                        auto res = ::write(fd, (void*)buffer, size);
                        if (res == -1) {
                            callback(0, *error_code::get_error_code(errno));
                        }
                        else {
                            callback(res, error_code{ 0 });
                        }
                    });

                ev.data.ptr = (void*)func_ptr.release();
                _loop.event_add(fd, &ev);
            }
        }


        template<accept_callback CallBack>
        auto async_accept(int fd, CallBack&& callback) {
            epoll_event ev;
            ev.events = EPOLLIN;

            if constexpr (std::is_same_v<CallBack, use_coroutine>) {

            }
            else {
                auto func_ptr = std::make_unique<std::function<void()>>([ this, fd, callback = std::forward<CallBack>(callback) ]() {
                    auto res = ::accept(fd, NULL, NULL);
                    if (res == -1) {
                        callback(std::nullopt, *error_code::get_error_code(errno));
                    }
                    else {
                        std::optional client = async::net::tcp::socket<proactor, false>{ *this, res };
                        callback(std::move(client), error_code{ 0 });
                    }

                    });

                ev.data.ptr = (void*)func_ptr.release();
                _loop.event_add(fd, &ev);
            }

        }

        private:
        void init_thread() {
            _thread = std::jthread([ this ]() {
                while (true) {
                    auto events = _loop.select(1024);
                    for (auto& ev : events) {
                        auto callback_ptr = (std::function<void()>*)ev.data.ptr;
                        auto callback = std::move(*callback_ptr);
                        std::destroy_at(callback_ptr);
                        callback();
                    }
                }
                });
        }

        event_loop _loop;
        std::jthread _thread;
        std::once_flag _flag;
    };


}


#endif