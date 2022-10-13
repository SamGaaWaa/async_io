// #include "sys/epoll.h"
// #include "sys/socket.h"
// #include "sys/types.h"
// #include "netinet/in.h"
// #include "arpa/inet.h"
// #include "unistd.h"
// #include "fcntl.h"


// #include <iostream>
// #include <cstdlib>
// #include <cstring>
// #include <thread>
// #include <string>
// #include <chrono>
// #include <vector>
// #include <array>

// int main(int argc, char** argv) {
//     if (argc != 3) {
//         std::cerr << "args error\n";
//         return -1;
//     }
//     const char* ip = argv[1];
//     int port = std::atoi((const char*)argv[2]);


//     int fd = socket(PF_INET, SOCK_STREAM, 0);
//     if (fd == -1) {
//         std::cerr << "create socket failed\n";
//         return -1;
//     }

//     sockaddr_in addr;
//     socklen_t len;

//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     inet_aton(ip, &addr.sin_addr);
//     addr.sin_port = port;

//     int res = ::connect(fd, (const sockaddr*)&addr, sizeof(addr));
//     if (res == -1) {
//         std::cerr << "connect failed\n";
//         return -1;
//     }

//     char c;
//     std::string tmp;

//     while (true) {
//         while ((c = getchar()) != '\n') {
//             tmp.push_back(c);
//         }
//         tmp.push_back('\0');
//         res = ::write(fd, (const void*)tmp.data(), tmp.size() - 1);
//         if (res == -1) {
//             std::cerr << "write error\n";
//             ::close(fd);
//             return -1;
//         }
//         tmp[res] = '\0';
//         std::cout << "has writed: " << tmp.data() << std::endl;
//         tmp.clear();
//     }

// }



#include "sys/epoll.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "fcntl.h"


#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <string>
#include <chrono>
#include <vector>
#include <array>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "args error\n";
        return -1;
    }
    const char* ip = argv[1];
    int port = std::atoi((const char*)argv[2]);


    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        std::cerr << "create socket failed\n";
        return -1;
    }

    sockaddr_in addr;
    socklen_t len;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_aton(ip, &addr.sin_addr);
    addr.sin_port = ::htons(port);

    std::cout << addr.sin_port << '\n';

    int res = ::connect(fd, (const sockaddr*)&addr, sizeof(addr));
    if (res == -1) {
        std::cerr << "connect failed: " << strerror(errno) << '\n';
        return -1;
    }

    std::cout << "Has connected.\n";

    char c;
    std::array<char, 1024> tmp;

    while (true) {
        auto i = 0;
        while ((c = getchar()) != '\n' and i < tmp.size() - 1) {
            tmp[i++] = c;
        }

        res = ::write(fd, (const void*)tmp.data(), i);
        if (res == -1) {
            std::cerr << "write error\n";
            ::close(fd);
            return -1;
        }


        res = ::read(fd, (void*)tmp.data(), res);
        if (res == -1) {
            std::cerr << "read error\n";
            ::close(fd);
            return -1;
        }
        tmp[res] = '\0';
        std::cout << ">>>" << tmp.data() << std::endl;
    }

}