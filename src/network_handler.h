#ifndef NETWORK_HANDLER_H
#define NETWORK_HANDLER_H

#include <string>
#include <cstring>

void handle_client(int client_socket);

void log_message(const std::string& message);
#endif // NETWORK_HANDLER_H
