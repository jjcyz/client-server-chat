#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <chrono>
#include <mutex>
#include <vector>
#include "constants.h"

// Message structure for the queue
struct Message {
    int sender_socket;
    std::string content;
};

// Server-specific globals
extern std::mutex history_mtx;
extern std::mutex console_mtx;
extern std::vector<std::string> chat_history;

// Function declarations
void log_message(const std::string& message);
void process_command(const Message& msg);
void initialize_connection_pool();
void handle_client(int client_socket);
void message_worker();

#endif // SERVER_H
