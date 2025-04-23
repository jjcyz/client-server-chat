#include "network_handler.h"
#include "connection_pool.h"
#include "message_queue.h"
#include "server_metrics.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <iostream>
#include <chrono>
#include <thread>

extern MessageQueue message_queue;
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

void broadcast(int sender, const std::string& message) {
    auto start = std::chrono::steady_clock::now();

    // Check message size
    if (message.length() > MAX_MESSAGE_SIZE) {
        log_message("Message too large, dropping broadcast");
        metrics.messages_dropped++;
        return;
    }

    std::time_t now = std::time(nullptr);
    char time_buffer[20];
    std::strftime(time_buffer, sizeof(time_buffer), "[%H:%M:%S] ", std::localtime(&now));

    std::string timed_message = time_buffer + message;

    metrics.record_message("broadcast");
    metrics.record_bytes(timed_message.length() * (metrics.current_connections.load() - 1));

    {
        std::lock_guard<std::mutex> lock(history_mtx);
        chat_history.push_back(timed_message);
        // Limit history size to prevent memory growth
        if (chat_history.size() > MAX_HISTORY_SIZE) {
            chat_history.erase(chat_history.begin());
        }
    }

    // Send to all active connections
    {
        std::lock_guard<std::mutex> lock(pool_mtx);
        for (auto& conn : connection_pool) {
            if (conn.in_use && conn.socket != sender) {
                int retry_count = 0;
                while (retry_count < MAX_RETRY_ATTEMPTS) {
                    int result = send(conn.socket, timed_message.c_str(), timed_message.length(), 0);
                    if (result > 0) {
                        conn.last_activity = std::chrono::steady_clock::now();
                        break;
                    }

                    if (errno == EPIPE) {
                        log_message("Client " + conn.username + " disconnected unexpectedly");
                        release_connection(&conn);
                        break;
                    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        retry_count++;
                        if (retry_count < MAX_RETRY_ATTEMPTS) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            continue;
                        }
                        log_message("Socket buffer full for client " + conn.username);
                        break;
                    } else {
                        log_message("Failed to broadcast to client " + conn.username + ": " + std::string(strerror(errno)));
                        release_connection(&conn);
                        break;
                    }
                }
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    metrics.record_message("broadcast", latency);
}

void handle_client(int client_socket) {
    try {
        Connection* conn = get_available_connection();
        if (!conn) {
            log_message("No available connections in pool");
            close(client_socket);
            return;
        }

        // Set socket options for better performance
        int opt = 1;
        int socket_buffer_size = 64 * 1024;  // 64KB buffer

        struct {
            int level;
            int optname;
            const void* optval;
            socklen_t optlen;
            const char* error_msg;
        } socket_options[] = {
            {SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt), "keepalive"},
            {IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt), "TCP_NODELAY"},
            {SOL_SOCKET, SO_RCVBUF, &socket_buffer_size, sizeof(socket_buffer_size), "receive buffer size"},
            {SOL_SOCKET, SO_SNDBUF, &socket_buffer_size, sizeof(socket_buffer_size), "send buffer size"}
        };

        for (const auto& option : socket_options) {
            if (setsockopt(client_socket, option.level, option.optname, option.optval, option.optlen) < 0) {
                log_message("Error: Could not set " + std::string(option.error_msg) + " for client: " + std::string(strerror(errno)));
            }
        }

        conn->socket = client_socket;
        metrics.update_connections(metrics.current_connections.load() + 1);
        log_message("New connection accepted. Current connections: " + std::to_string(metrics.current_connections.load()));

        char username_buffer[BUFFER_SIZE];
        int bytes_received_username = recv(client_socket, username_buffer, BUFFER_SIZE, 0);
        if (bytes_received_username <= 0) {
            log_message("Failed to receive username: " + std::string(strerror(errno)));
            release_connection(conn);
            return;
        }
        username_buffer[bytes_received_username] = '\0';
        conn->username = std::string(username_buffer);

        broadcast(-1, conn->username + " has joined the chat");

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

            // Check message size
            if (message.length() > MAX_MESSAGE_SIZE) {
                log_message("Message too large from client " + conn->username);
                continue;
            }

            Message msg;
            msg.sender_socket = client_socket;
            msg.content = message;

            if (!message_queue.push(msg)) {
                metrics.messages_dropped++;
                log_message("Message queue full, dropping message from " + conn->username);
            }
        }

        broadcast(-1, conn->username + " has left the chat");
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
