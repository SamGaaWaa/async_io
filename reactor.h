#ifndef __REACTOR_H__
#define __REACTOR_H__

#include "event_loop.h"

#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <ranges>

namespace async::net::tcp {
    class reactor {
        public:
        event_loop& get_event_loop()noexcept { return _loop; }
        auto post(std::function<void()> func) { func(); }

        void run(size_t select_size = 1024, int time_out = -1) {
            std::call_once(_flag, [this] {
                init_id();
                });
            auto this_id = std::this_thread::get_id();
            if (this_id == _id) {
                _buffer.resize(select_size);
                while (true) {
                    auto res = _loop.select(_buffer, time_out);
                    if (res == -1)
                        throw error_code{};
                    for (auto i{ 0 }; i < res; ++i) {
                        auto cb_ptr = (std::function<void(unsigned int)>*)_buffer[i].data.ptr;
                        if (*cb_ptr == nullptr)
                            continue;
                        auto cb = std::move(*cb_ptr);
                        *cb_ptr = nullptr;
                        cb(_buffer[i].events);
                    }

                }
            }
        }


        private:

        void init_id() { _id = std::this_thread::get_id(); }

        event_loop _loop;
        std::once_flag _flag;
        std::thread::id _id;
        std::vector<epoll_event> _buffer;
    };


}

#endif