#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <optional>
#include <exception>

#include "errno.h"

namespace async {

    struct bad_socket :std::exception {
        const char* what() { return "bad socket\n"; }
    };

    struct error_code :std::exception {

        static std::optional<error_code> get_error_code(int x) {
            switch (x) {
                case	EPERM:	    return error_code{ x };
                case	ENOENT:	    return error_code{ x };
                case	ESRCH:	    return error_code{ x };
                case	EINTR:	    return error_code{ x };
                case	EIO:		return error_code{ x };
                case	ENXIO:	    return error_code{ x };
                case	E2BIG:	    return error_code{ x };
                case	ENOEXEC:	return error_code{ x };
                case	EBADF:	    return error_code{ x };
                case	ECHILD:	    return error_code{ x };
                case	EAGAIN:	    return error_code{ x };
                case	ENOMEM:	    return error_code{ x };
                case	EACCES:	    return error_code{ x };
                case	EFAULT:	    return error_code{ x };
                case	ENOTBLK:	return error_code{ x };
                case	EBUSY:	    return error_code{ x };
                case	EEXIST:	    return error_code{ x };
                case	EXDEV:	    return error_code{ x };
                case	ENODEV:	    return error_code{ x };
                case	ENOTDIR:	return error_code{ x };
                case	EISDIR:	    return error_code{ x };
                case	EINVAL:	    return error_code{ x };
                case	ENFILE:	    return error_code{ x };
                case	EMFILE:	    return error_code{ x };
                case	ENOTTY:	    return error_code{ x };
                case	ETXTBSY:	return error_code{ x };
                case	EFBIG:	    return error_code{ x };
                case	ENOSPC:	    return error_code{ x };
                case	ESPIPE:	    return error_code{ x };
                case	EROFS:	    return error_code{ x };
                case	EMLINK:	    return error_code{ x };
                case	EPIPE:	    return error_code{ x };
                case	EDOM:	    return error_code{ x };
                case	ERANGE:	    return error_code{ x };
                default:            return std::nullopt;
            }
        }

        const char* what()noexcept {
            switch (_error) {
                case	EPERM:	    return "Operation not permitted ";
                case	ENOENT:	    return "No such file or director";
                case	ESRCH:	    return "No such process ";
                case	EINTR:	    return "Interrupted system call ";
                case	EIO:		    return "I/O error ";
                case	ENXIO:	    return "No such device or address";
                case	E2BIG:	    return "Argument list too long ";
                case	ENOEXEC:	    return "Exec format error ";
                case	EBADF:	    return "Bad file number ";
                case	ECHILD:	    return "No child processes ";
                case	EAGAIN:	    return "Try again ";
                case	ENOMEM:	    return "Out of memory ";
                case	EACCES:	    return "Permission denied ";
                case	EFAULT:	    return "Bad address ";
                case	ENOTBLK:	    return "Block device required ";
                case	EBUSY:	    return "Device or resource busy ";
                case	EEXIST:	    return "File exists ";
                case	EXDEV:	    return "Cross-device link ";
                case	ENODEV:	    return "No such device ";
                case	ENOTDIR:	    return "Not a directory ";
                case	EISDIR:	    return "Is a directory ";
                case	EINVAL:	    return "Invalid argument ";
                case	ENFILE:	    return "File table overflow ";
                case	EMFILE:	    return "Too many open files ";
                case	ENOTTY:	    return "Not a typewriter ";
                case	ETXTBSY:	    return "Text file busy ";
                case	EFBIG:	    return "File too large ";
                case	ENOSPC:	    return "No space left on device ";
                case	ESPIPE:	    return "Illegal seek ";
                case	EROFS:	    return "Read-only file system ";
                case	EMLINK:	    return "Too many links ";
                case	EPIPE:	    return "Broken pipe ";
                case	EDOM:	    return "Math argument out of domain of func ";
                case	ERANGE:	    return "Math result not representable ";
                default: return "No error ";
            }
        }

        error_code()noexcept :_error{ errno } {}
        error_code(int x)noexcept :_error{ x } {}

        operator bool()noexcept { return _error != 0; }
        operator int()noexcept { return _error; }


        private:
        int _error;
    };
}

#endif