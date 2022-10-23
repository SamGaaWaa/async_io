//
// Created by 86137 on 2022/9/13.
//

#ifndef ASYNC_IO_THREAD_POOL_H
#define ASYNC_IO_THREAD_POOL_H

#include <thread>
#include <memory>
#include <vector>
#include <functional>
#include <exception>
#include "block_queue.h"

namespace async {

    template<class CallBack>
    concept void_result_callback = requires(CallBack c) { c(std::exception_ptr{}); };

    template<class R, class CallBack>
    concept result_callback = requires(std::optional<R> res, CallBack c) { c(std::exception_ptr{}, res); };

    template<class Func, class CallBack>
    struct thread_pool_callback {
        static constexpr bool value = (std::invocable<Func> && (
            (std::is_same_v<std::invoke_result<Func>, void> && void_result_callback<CallBack>) ||
            (!std::is_same_v<std::invoke_result<Func>, void> && result_callback<std::invoke_result<Func>, CallBack>)
            ));
    };



    class thread_pool {
        using Task = std::function<void()>;
        public:

        struct default_call_back {
            void operator()(std::exception_ptr ptr, auto&& res)const {
                if (ptr)
                    std::rethrow_exception(ptr);
            }
            void operator()(std::exception_ptr ptr)const {
                if (ptr)
                    std::rethrow_exception(ptr);
            }
        };

        explicit thread_pool(int x) {
            for (auto i{ 0 }; i < x; ++i) {
                _threads.emplace_back([this] {
                    while (!_stop) {
                        auto task = _queue.pop();
                        if (task)
                            (*task)();
                    }
                    });
            }
        }

        thread_pool(const thread_pool&) = delete;

        thread_pool& operator=(const thread_pool&) = delete;

        ~thread_pool() {
            if (!_stop)
                stop();
            for (auto& t : _threads)
                if (t.joinable())
                    t.join();
        }

        template<class Func, class CallBack>
        //            requires thread_pool_callback<Func, CallBack>::value
        auto post(Func&& func, CallBack&& call_back) {
            _queue.push([task = std::forward<Func>(func),
                call_back = std::forward<CallBack>(call_back)] {
                    using result_type = std::invoke_result_t<Func>;
            if constexpr (std::is_same<result_type, void>::value) {
                std::exception_ptr ptr;
                try {
                    task();
                }
                catch (...) {
                    ptr = std::current_exception();
                }
                call_back(ptr);
            }
            else {
                std::exception_ptr ptr;
                std::optional<result_type> res;
                try {
                    res = task();
                }
                catch (...) {
                    ptr = std::current_exception();
                    res = std::nullopt;
                }
                call_back(ptr, std::move(res));
            }
                });
        }

        template<class Func>
            requires std::invocable<Func>
        auto post(Func&& func) {
            return post(std::forward<Func>(func), default_call_back{});
        }


        void stop() {
            auto counter = std::make_shared<std::atomic_int64_t>(_threads.size() + 1);
            for (auto i{ 0 }; i < _threads.size(); ++i) {
                _queue.push([counter] {
                    counter->fetch_sub(1);
                while (*counter != 0);
                    });
            }
            while (*counter != 1);
            _stop.store(true);
            counter->fetch_sub(1);
        }


        private:
        std::vector<std::thread> _threads;
        async::block_queue<Task> _queue;
        std::atomic_bool _stop{ false };
    };


}

#endif //ASYNC_IO_THREAD_POOL_H
