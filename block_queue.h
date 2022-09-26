//
// Created by 86137 on 2022/9/13.
//

#ifndef ASYNC_IO_BLOCK_QUEUE_H
#define ASYNC_IO_BLOCK_QUEUE_H

#include <queue>
#include <mutex>
#include <concepts>
#include <optional>
#include <condition_variable>
#include <atomic>

namespace async {

    template<class T>
    class block_queue {

        public:
        template<std::convertible_to<T> T1>
        void push(T1&& x) {
            {
                std::lock_guard l{ _m };
                _queue.push(std::forward<T1>(x));
                _size.fetch_add(1);
            }
            _condition_variable.notify_all();
        }

        std::optional<T> pop() noexcept {
            std::optional<T> res;
            try {
                std::unique_lock l{ _m };
                _condition_variable.wait(l, [ this ] { return !_queue.empty(); });
                res = std::move(_queue.front());
                _queue.pop();
                _size.fetch_sub(1);
            }
            catch (...) {
                if (res)
                    _queue.pop();
            }
            return res;
        }

        bool empty() const noexcept { return _size == 0; };

        private:
        std::queue<T> _queue;
        std::mutex _m;
        std::condition_variable _condition_variable;
        std::atomic_int64_t _size = 0;
    };


}
#endif //ASYNC_IO_BLOCK_QUEUE_H
