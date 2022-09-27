//
// Created by 86137 on 2022/9/13.
//

#ifndef ASYNC_IO_BLOCK_QUEUE_H
#define ASYNC_IO_BLOCK_QUEUE_H

#include <deque>
#include <mutex>
#include <concepts>
#include <optional>
#include <condition_variable>
#include <atomic>

namespace async {

    template<class T>
    class block_queue {

        public:
        using value_type = T;

        template<std::convertible_to<T> T1>
        void push(T1&& x) {
            {
                std::lock_guard l{ _m };
                _queue.push_back(std::forward<T1>(x));
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
                _queue.pop_front();
                _size.fetch_sub(1);
            }
            catch (...) {
                if (res)
                    _queue.pop_front();
            }
            return res;
        }

        bool empty() const noexcept { return _size == 0; };

        template<std::convertible_to<T> T1>
        void unsafe_push(T1&& x) {
            _queue.push(std::forward<T1>(x));
            _size.fetch_add(1);
            _condition_variable.notify_all();
        }

        template<class Iter1, class Iter2>
        void unsafe_append(Iter1&& first, Iter2&& last) {
            _queue.insert(_queue.end(), first, last);
            _size = _queue.size();
            _condition_variable.notify_all();
        }

        auto lock() {
            return std::unique_lock{ _m };
        }

        private:
        std::deque<T> _queue;
        std::mutex _m;
        std::condition_variable _condition_variable;
        std::atomic_int64_t _size = 0;
    };


}
#endif //ASYNC_IO_BLOCK_QUEUE_H
