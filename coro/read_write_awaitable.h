#ifndef __READ_WRITE_AWAITABLE_H__
#define __READ_WRITE_AWAITABLE_H__

#include <variant>

#include "exception.h"

#include "unistd.h"

namespace async::net::tcp {

    struct read_write_awaitable {
        std::variant<int, error_code> size_or_error;

        bool await_ready()noexcept { return size_or_error.index() == 0; }
        auto await_suspend()noexcept {}
        auto await_resume()noexcept {

        }
    };


}

#endif