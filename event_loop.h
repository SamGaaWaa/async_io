#ifndef __EVENT_LOOP__
#define __EVENT_LOOP__

#include "sys/epoll.h"
#include "unistd.h"

#include "exception.h"

#include <optional>
#include <vector>


namespace async::net {


    class event_loop {
        public:

        event_loop(int fd)noexcept : _epoll_fd{ fd } {}
        event_loop(const event_loop&) = delete;
        event_loop& operator=(const event_loop&) = delete;
        event_loop(event_loop&&) = default;
        event_loop& operator=(event_loop&&) = default;
        ~event_loop() { ::close(_epoll_fd); }

        int handle()noexcept { return _epoll_fd; }

        std::optional<error_code> event_add(int fd, epoll_event* event)noexcept {
            auto res = ::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, event);
            if (res == -1) {
                return error_code::get_error_code(errno);
            }
            return std::nullopt;
        }

        std::optional<error_code> event_del(int fd)noexcept {
            auto res = ::epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
            if (res == -1) {
                return error_code::get_error_code(errno);
            }
            return std::nullopt;
        }

        std::optional<error_code> event_mod(int fd, epoll_event* event)noexcept {
            auto res = ::epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, event);
            if (res == -1) {
                return error_code::get_error_code(errno);
            }
            return std::nullopt;
        }

        std::vector<epoll_event> select(size_t max_size, int timeout = -1) {
            std::vector<epoll_event> events;
            events.resize(max_size);
            auto res = ::epoll_wait(_epoll_fd, events.data(), max_size, timeout);
            if (res == -1) {
                auto error = *error_code::get_error_code(errno);
                throw error;
            }
            events.resize(res);
            return events;
        }

        static event_loop get_event_loop() {
            int res = ::epoll_create(1);
            if (res == -1) {
                auto error = *error_code::get_error_code(errno);
                throw error;
            };
            return { res };
        }

        private:
        int _epoll_fd;
    };


}

#endif