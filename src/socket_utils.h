#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include "constants.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>

// Configure socket with appropriate options
// Parameters:
//   socket: The socket file descriptor to configure
//   is_server: true if this is a server socket, false for client socket
// Returns:
//   true if all options were set successfully, false otherwise
bool configure_socket(int socket, bool is_server);

// Set socket to non-blocking mode
// Parameters:
//   socket: The socket file descriptor to make non-blocking
// Returns:
//   true if successful, false otherwise
bool set_socket_nonblocking(int socket);

// Log socket-related errors with consistent formatting
// Parameters:
//   operation: The operation that failed (e.g., "setsockopt", "bind", etc.)
//   error: Additional error context
void log_socket_error(const std::string& operation, const std::string& error);

#endif // SOCKET_UTILS_H
