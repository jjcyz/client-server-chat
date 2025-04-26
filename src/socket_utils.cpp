#include "socket_utils.h"
#include "constants.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <netinet/in.h>  // Add this for IPPROTO_TCP
#include <netinet/tcp.h> // Add this for TCP_NODELAY

struct SocketOption {
        int level;
        int optname;
        const void* optval;
        socklen_t optlen;
        const char* error_msg;

        // Constructor to allow brace-enclosed initialization
        SocketOption(int level, int optname, const void* optval, socklen_t optlen, const char* error_msg)
            : level(level), optname(optname), optval(optval), optlen(optlen), error_msg(error_msg) {}

};

bool configure_socket(int socket, bool is_server) {
    int opt = 1;
    int buffer_size = SOCKET_BUFFER_SIZE;  // Create variable for buffer size

    // Create and initialize vector of socket options
    std::vector<SocketOption> options;

    // Add options using push_back
    options.push_back({SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt), "keepalive"});
    options.push_back({IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt), "TCP_NODELAY"});
    options.push_back({SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size), "receive buffer"});
    options.push_back({SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size), "send buffer"});

    // Add server-specific options
    if (is_server) {
        options.push_back({SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt), "address reuse"});
    }

    // Apply all socket options
    for (const auto& option : options) {
        if (setsockopt(socket, option.level, option.optname, option.optval, option.optlen) < 0) {
            log_socket_error("setsockopt", option.error_msg);
            return false;
        }
    }
    return true;
}

bool set_socket_nonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        log_socket_error("fcntl", "get flags");
        return false;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_socket_error("fcntl", "set non-blocking");
        return false;
    }
    return true;
}

void log_socket_error(const std::string& operation, const std::string& error) {
    std::string error_msg = "Socket error during " + operation + ": " + error;
    if (errno != 0) {
        error_msg += " (" + std::string(strerror(errno)) + ")";
    }
    std::cerr << error_msg << std::endl;
}
