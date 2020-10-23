#ifndef MOCK_WEB_SOCKET_H
#define MOCK_WEB_SOCKET_H

#include <string>

namespace mock_web_socket {
    struct web_socket {
        void open(std::string_view url);
        std::string receive_msg();
    };
}  // namespace mock_web_socket

#endif
