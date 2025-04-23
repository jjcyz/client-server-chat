#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <chrono>

// Constants
#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 200
#define MESSAGE_QUEUE_SIZE 2000
#define WORKER_THREADS 4
#define MAX_MESSAGE_SIZE 4096
#define CONNECTION_TIMEOUT 30
#define MAX_RETRY_ATTEMPTS 5
#define MAX_HISTORY_SIZE 100
#define MAX_LATENCY_SAMPLES 1000

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
