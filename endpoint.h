#ifndef __ENDPOINT_H__
#define __ENDPOINT_H__

#include <string_view>
#include <string>
#include <cstring>


#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

namespace async::net {

    class endpoint {

        public:
        endpoint(const char* ip, int port)noexcept {
            memset(&_address, 0, sizeof(_address));
            _address.sin_family = AF_INET;
            _address.sin_port = port;
            inet_aton(ip, &_address.sin_addr);
        }

        endpoint(const std::string& ip, int port)noexcept {
            endpoint(ip.c_str(), port);
        }

        sockaddr_in& address()noexcept { return _address; }

        private:
        sockaddr_in _address;
    };



}


#endif