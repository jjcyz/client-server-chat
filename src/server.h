#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <chrono>
#include "constants.h"

// Message structure for the queue
struct Message {
    int sender_socket;
    std::string content;
};

// Forward declarations
void log_message(const std::string& message);
void process_command(const Message& msg);
void initialize_connection_pool();
void handle_client(int client_socket);
void message_worker();

#endif // SERVER_H
