#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "sys/socket.h"
#include "fcntl.h"
#include "unistd.h"

#include "exception.h"
#include "endpoint.h"
#include "sys/epoll.h"
#include "proactor.h"
#include "coro/read_operation.h"
#include "coro/write_operation.h"
#include "coro/accept_operation.h"
#include "coro/connect_operation.h"

#include <optional>
#include <cstring>
#include <limits>
#include <functional>
#include <concepts>
#include <coroutine>

namespace async::net::tcp {

    struct use_coroutine_t {};

    // template<typename CallBack>
    // concept read_write_callback = requires (CallBack c, size_t size, error_code code) {
    //     c(size, code);
    // };

    // template<typename CallBack, typename Executor, typename Socket>
    // concept accept_callback = requires(std::optional<Socket> s, error_code code, CallBack c) {
    //     c(s, code);
    // };

    template<class Executor = proactor, bool Block = false>
    class socket {
        public:
        struct bad_fd {};
        using executor_type = Executor;
        using callback_type = std::function<void(unsigned int)>;
        using socket_type = socket<executor_type, Block>;
        static constexpr bool block = Block;

        socket(executor_type& executor):_executor{ executor }, _callback{ std::make_unique<callback_type>() }, _loop{ _executor.get_event_loop() } {
            _fd = ::socket(PF_INET, SOCK_STREAM, 0);
            if (_fd == -1) {
                throw bad_socket{};
            }
            if constexpr (!Block) {
                set_nonblocking();
            }
        }

        socket(executor_type& executor, int fd):_executor{ executor }, _fd{ fd }, _callback{ std::make_unique<callback_type>() }, _loop{ _executor.get_event_loop() } {
            if (fd == -1)
                throw bad_socket{};
            if constexpr (!Block) {
                set_nonblocking();
            }
            else {
                set_blocking();
            }
        };

        socket(executor_type& executor, bad_fd)noexcept:_executor{ executor }, _fd{ -1 }, _loop{ _executor.get_event_loop() } {}

        socket(const socket&) = delete;
        socket& operator=(const socket&) = delete;

        socket(socket&& other)noexcept: _fd{ other._fd }, _executor{ other._executor }, _loop{ other._loop }, _callback{ std::move(other._callback) }, _in_poll{ other._in_poll } {
            other._fd = -1;
            other._in_poll = false;
        }

        socket& operator=(socket&& other)noexcept {
            _fd = other._fd;
            _executor = other._executor;
            _loop = other._loop;
            _callback = std::move(other._callback);
            _in_poll = other._in_poll;
            other._in_poll = false;
            other._fd = -1;
            return *this;
        }

        ~socket() {
            if (_fd != -1)
                ::close(_fd);
        }


        operator int()noexcept { return _fd; }
        operator bool()noexcept { return _fd != -1; }

        bool valid()const noexcept { return _fd != -1; }

        int native_handle()const noexcept { return _fd; }
        executor_type& get_executor()noexcept { return _executor; }

        error_code bind(endpoint& address)noexcept {
            auto res = ::bind(_fd, (const sockaddr*)&address.address(), sizeof(address.address()));
            if (res == -1) {
                return error_code{};
            }
            return error_code{ 0 };
        }

        error_code bind(const char* ip, int port)noexcept {
            sockaddr_in address;
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_port = ::htons(port);
            inet_aton(ip, &address.sin_addr);

            auto res = ::bind(_fd, (const sockaddr*)&address, sizeof(address));
            if (res == -1) {
                return error_code{};
            }
            return error_code{ 0 };
        }

        error_code bind(const std::string& ip, int port)noexcept {
            return bind(ip.c_str(), port);
        }

        error_code listen(int backlog = std::numeric_limits<int>::max())noexcept {
            auto res = ::listen(_fd, backlog);
            if (res == -1) {
                return error_code{};
            }
            return error_code{ 0 };
        }

        std::optional<socket> accept(error_code& code)noexcept {
            static_assert(Block, "Only support blocking socket.\n");
            auto res = ::accept(_fd, NULL, NULL);
            if (res == -1) {
                return std::nullopt;
            }
            return socket{ _executor, res };
        }

        template<class CallBack>
        auto async_connect(endpoint& ep, CallBack&& callback) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLOUT | EPOLLERR | EPOLLONESHOT;

            int res;
            while (true) {
                res = ::connect(_fd, (const sockaddr*)(&ep.address()), ep.size());
                if (res == -1) {
                    if (errno == EINTR)
                        continue;
                    else if (errno == EINPROGRESS or errno == EAGAIN) {//????????????????????????epoll?????????
                        *_callback = [cb = std::forward<CallBack>(callback), this](unsigned int event) {
                            if (event & EPOLLERR) {//???????????????
                                int err;
                                socklen_t err_len = sizeof(err);
                                ::getsockopt(_fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
                                cb(error_code{ err });
                            }
                            else {//???????????????
                                cb(error_code{ 0 });
                            }
                        };
                        ev.data.ptr = (void*)_callback.get();
                        if (_in_poll) {
                            auto err = _loop.event_mod(_fd, &ev);
                            if (err) {
                                throw err;
                            }
                        }
                        else {
                            auto err = _loop.event_add(_fd, &ev);
                            if (err) {
                                throw err;
                            }
                            _in_poll = true;
                        }
                        break;
                    }
                    else {//?????????
                        _executor.post([cb = std::forward<CallBack>(callback), err = error_code{}]() {
                            cb(err);
                            });
                        break;
                    }
                }
                else {//???????????????
                    _executor.post([cb = std::forward<CallBack>(callback)]() {
                        cb(error_code{ 0 });
                        });
                    break;
                }
            }
        }

        auto async_connect(endpoint& ep, std::coroutine_handle<> h)noexcept {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLOUT | EPOLLERR | EPOLLONESHOT;

            *_callback = [h](unsigned int) {
                h();
            };
            ev.data.ptr = (void*)_callback.get();
            if (_in_poll) {
                return _loop.event_mod(_fd, &ev);
            }
            else {
                auto err = _loop.event_add(_fd, &ev);
                if (!err) {
                    _in_poll = true;
                }
                return err;
            }
        }

        auto async_connect(endpoint& ep)noexcept {
            static_assert(!Block, "Only support nonblocking socket.\n");
            return connect_operation{ *this, ep };
        }

        template<class CallBack>
        auto async_accept(CallBack&& callback) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;

            auto res = ::accept(_fd, NULL, NULL);
            if (-1 == res) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//accept???????????????epoll??????
                    *_callback = [this, cb = std::forward<CallBack>(callback), err = error_code{}](unsigned int event) {
                        auto res = ::accept(_fd, NULL, NULL);
                        if (res == -1) {
                            cb(socket{ _executor, bad_fd{} }, err);
                            return;
                        }

                        cb(socket{ _executor, res }, error_code{ 0 });
                    };
                    ev.data.ptr = (void*)_callback.get();
                    if (_in_poll) {
                        auto err = _loop.event_mod(_fd, &ev);
                        if (err) {
                            // std::cerr << "async_accept event_mod error: " << err.what() << '\n';
                            throw err;
                        }
                    }
                    else {
                        auto err = _loop.event_add(_fd, &ev);
                        if (err) {
                            // std::cerr << "async_accept event_add error: " << err.what() << '\n';
                            throw err;
                        }
                        _in_poll = true;
                    }

                }
                else {//accept??????
                    _executor.post([this, cb = std::forward<CallBack>(callback), err = error_code{}] {
                        cb(socket{ _executor, bad_fd{} }, err);
                        });
                }
            }
            else {//accept??????
                _executor.post([cb = std::forward<CallBack>(callback), this, res]() {
                    cb(socket{ _executor, res }, error_code{ 0 });
                    });
            }

        }

        auto async_accept(std::coroutine_handle<> h) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;

            *_callback = [h](unsigned int event) {
                h();        //????????????
            };

            ev.data.ptr = (void*)_callback.get();
            if (_in_poll) {
                return _loop.event_mod(_fd, &ev);
            }
            else {
                auto err = _loop.event_add(_fd, &ev);
                if (!err)
                    _in_poll = true;
                return err;
            }
        }

        accept_operation<socket_type> async_accept()noexcept { return { *this }; }

        template<class CallBack>
        auto async_read(char* buff, size_t max_size, CallBack&& callback) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

            auto res = ::read(_fd, buff, max_size);
            if (res == -1) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//read???????????????epoll??????
                    *_callback = [this, buff, max_size, cb = std::forward<CallBack>(callback)](unsigned int event) {
                        auto res = ::read(_fd, (void*)buff, max_size);
                        if (res == -1) {
                            cb(-1, error_code{});
                        }
                        else {
                            cb(res, error_code{ 0 });
                        }
                    };

                    ev.data.ptr = (void*)_callback.get();
                    if (_in_poll) {
                        auto err = _loop.event_mod(_fd, &ev);
                        if (err) {
                            // std::cerr << "async_read event_mod error: " << err.what() << '\n';
                            throw err;
                        }
                    }
                    else {
                        auto err = _loop.event_add(_fd, &ev);
                        if (err) {
                            // std::cerr << "async_read event_add error: " << err.what() << '\n';
                            throw err;
                        }
                        _in_poll = true;
                    }
                }
                else {//read??????
                    _executor.post([cb = std::forward<CallBack>(callback), err = error_code{}]() {
                        cb(-1, err);
                        });
                }
            }
            else {//??????????????????????????????res???0???
                _executor.post([cb = std::forward<CallBack>(callback), res]() {
                    cb(res, error_code{ 0 });
                    });
            }

        }

        auto async_read(char* buff, size_t max_size, std::coroutine_handle<> h) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

            *_callback = [h](unsigned int event) {
                h();        //????????????
            };

            ev.data.ptr = (void*)_callback.get();
            if (_in_poll) {
                return _loop.event_mod(_fd, &ev);
            }
            else {
                auto err = _loop.event_add(_fd, &ev);
                if (!err)
                    _in_poll = true;
                return err;
            }
        }

        read_operation<socket_type> async_read(char* buff, size_t max_size)noexcept {
            return { buff, max_size, *this };
        }

        template<class CallBack>
        auto async_write(char* buff, size_t max_size, CallBack&& callback) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;

            auto res = ::write(_fd, buff, max_size);
            if (res == -1) {
                if (errno == EAGAIN or errno == EWOULDBLOCK) {//write???????????????epoll??????
                    *_callback = [this, buff, max_size, cb = std::forward<CallBack>(callback)](unsigned int event) {
                        auto res = ::write(_fd, (void*)buff, max_size);
                        if (res == -1) {
                            cb(-1, error_code{});
                        }
                        else {
                            cb(res, error_code{ 0 });
                        }
                    };
                    ev.data.ptr = (void*)_callback.get();
                    if (_in_poll) {
                        auto err = _loop.event_mod(_fd, &ev);
                        if (err) {
                            // std::cerr << "async_write event_mod error: " << err.what() << '\n';
                            throw err;
                        }
                    }
                    else {
                        auto err = _loop.event_add(_fd, &ev);
                        if (err) {
                            // std::cerr << "async_write event_add error: " << err.what() << '\n';
                            throw err;
                        }
                        _in_poll = true;
                    }
                }
                else {//write??????
                    _executor.post([cb = std::forward<CallBack>(callback), err = error_code{}]() {
                        cb(-1, err);
                        });
                }
            }
            else {//??????????????????????????????res???0???
                _executor.post([cb = std::forward<CallBack>(callback), res]() {
                    cb(res, error_code{ 0 });
                    });
            }



        }


        auto async_write(char* buff, size_t max_size, std::coroutine_handle<> h) {
            static_assert(!Block, "Only support nonblocking socket.\n");

            epoll_event ev;
            ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;

            *_callback = [h](unsigned int event) {
                h();        //????????????
            };

            ev.data.ptr = (void*)_callback.get();
            if (_in_poll) {
                return _loop.event_mod(_fd, &ev);
            }
            else {
                auto err = _loop.event_add(_fd, &ev);
                if (!err)
                    _in_poll = true;
                return err;
            }
        }

        write_operation<socket_type> async_write(char* buff, size_t max_size)noexcept {
            return { buff, max_size, *this };
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
        event_loop& _loop;
        std::unique_ptr<callback_type> _callback;
        bool _in_poll{ false };
    };



}



#endif