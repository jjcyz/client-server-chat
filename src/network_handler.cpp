#include "network_handler.h"
#include "connection_pool.h"
#include "message_queue.h"
#include "server_metrics.h"
#include "socket_utils.h"
#include "server.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <iostream>
#include <chrono>
#include <thread>

extern std::vector<std::string> chat_history;
extern std::mutex history_mtx;
extern std::mutex console_mtx;

void log_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(console_mtx);
    std::time_t now = std::time(nullptr);
    char time_buffer[20];
    std::strftime(time_buffer, sizeof(time_buffer), "[%H:%M:%S] ", std::localtime(&now));
    std::cout << time_buffer << message << std::endl;
}

void handle_client(int client_socket) {
    try {
        Connection* conn = get_available_connection();
        if (!conn) {
            log_message("No available connections in pool");
            close(client_socket);
            return;
        }

        if (!configure_socket(client_socket, false)) {
            log_message("Error: Could not configure client socket");
            release_connection(conn);
            return;
        }

        conn->socket = client_socket;
        metrics.update_connections(metrics.current_connections.load() + 1);
        log_message("New connection accepted. Current connections: " + std::to_string(metrics.current_connections.load()));

        while (true) {
            char buffer[BUFFER_SIZE];
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    log_message("Client " + conn->username + " disconnected normally");
                } else {
                    log_message("Error receiving from client " + conn->username + ": " + std::string(strerror(errno)));
                }
                break;
            }
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            Message msg;
            msg.sender_socket = client_socket;
            msg.content = message;

            if (!message_queue.push(msg)) {
                metrics.messages_dropped++;
                log_message("Message queue full, dropping message from " + conn->username);
            }
        }

        metrics.update_connections(metrics.current_connections.load() - 1);
        log_message("Connection closed. Current connections: " + std::to_string(metrics.current_connections.load()));
        release_connection(conn);
    } catch (const std::exception& e) {
        log_message("Exception in handle_client: " + std::string(e.what()));
        if (client_socket != -1) {
            close(client_socket);
        }
    } catch (...) {
        log_message("Unknown exception in handle_client");
        if (client_socket != -1) {
            close(client_socket);
        }
    }
}
