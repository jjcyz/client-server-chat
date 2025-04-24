#ifndef NETWORK_HANDLER_H
#define NETWORK_HANDLER_H

#include <string>
#include <cstring>

#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_MESSAGE_SIZE 4096
#define MAX_RETRY_ATTEMPTS 5
#define MAX_HISTORY_SIZE 1000  // Maximum number of messages to keep in chat history

void handle_client(int client_socket);

void log_message(const std::string& message);
#endif // NETWORK_HANDLER_H

