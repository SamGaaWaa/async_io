#ifndef __ASYNC_TASK_H__
#define __ASYNC_TASK_H__

#include <coroutine>

namespace async {
    struct task {
        struct promise_type {
            task get_return_object()noexcept {
                return task{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }
            std::suspend_never initial_suspend() noexcept { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() {}
        };

        explicit task(std::coroutine_handle<promise_type> h)noexcept:_handle{ h } {}
        task(const task&) = delete;
        task& operator=(const task&) = delete;
        task(task&& other)noexcept:_handle{ other._handle } { other._handle = nullptr; }
        task& operator=(task&& other)noexcept {
            _handle = other._handle;
            other._handle = nullptr;
            return *this;
        }

        auto operator()()noexcept { _handle.resume(); }
        bool done()noexcept { return _handle.done(); }


        private:
        std::coroutine_handle<promise_type> _handle;
    };

}

#endif