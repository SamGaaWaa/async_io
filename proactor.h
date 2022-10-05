#ifndef __PROACTOR_H__
#define __PROACTOR_H__


#include "event_loop.h"
#include "exception.h"
#include "block_queue.h"

#include <concepts>
#include <type_traits>
#include <coroutine>
#include <functional>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <ranges>
#include <algorithm>
#include <iostream>

namespace async::net::tcp {

    class proactor {
        public:

        proactor():_loop{ event_loop::get_event_loop() } {}

        void run() {
            std::call_once(_flag, [this]() {init_thread(); });
            while (true) {
                auto func_opt = _queue.pop();
                if (func_opt) {
                    (*func_opt)();
                }
            }
        }

        event_loop& get_event_loop()noexcept { return _loop; }

        auto post(std::function<void()> func) {
            _queue.push(std::move(func));
        }


        private:
        void init_thread() {
            _dispatch_thread = std::jthread([this]() {
                while (true) {
                    auto events = _loop.select(1024);

                    auto rng = events | std::views::filter([ ](const auto& ev) {
                        return *((std::function<void()>*)ev.data.ptr) != nullptr;
                        }) | std::views::transform([ ](const auto& ev) {
                            auto ptr = (std::function<void()>*)ev.data.ptr;
                            auto callback = std::move(*ptr);
                            *ptr = nullptr;
                            return callback;
                            });
                        auto lock = _queue.lock();
                        _queue.unsafe_append(rng.begin(), rng.end());
                }
                });
        }


        event_loop _loop;
        std::jthread _dispatch_thread;
        std::once_flag _flag;
        block_queue<std::function<void()>> _queue;
    };


}




#endif