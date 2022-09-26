#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <optional>
#include <exception>

namespace async {

    struct bad_socket :std::exception {};

    struct error_code :std::exception {

        static std::optional<error_code> get_error_code(int x);



        private:
        int _error;
    };
}

#endif