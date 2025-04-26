#include "server.h"
#include "socket_utils.h"
#include "constants.h"
#include "connection_pool.h"
#include "message_queue.h"
#include "server_metrics.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <ctime>
#include <mutex>
#include <chrono>
#include <atomic>
#include <map>
#include <queue>
#include <condition_variable>

// Only these globals are defined here:
std::mutex history_mtx;
std::mutex console_mtx;
std::vector<std::string> chat_history;

bool send_with_retry(int socket, const std::string& message, int max_retries = MAX_RETRY_ATTEMPTS) {
    for (int retry_count = 0; retry_count < max_retries; ++retry_count) {
        int result = send(socket, message.c_str(), message.length(), 0);
        if (result > 0) {
            return true;  // Success
        }

        if (errno == EPIPE) {
            return false;  // Client disconnected
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return false;  // Unrecoverable error
        }

        // Wait before retry
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;  // Max retries reached
}

void broadcast(int sender, const std::string& message) {
    auto start = std::chrono::steady_clock::now();

    // Check message size
    if (message.length() > MAX_MESSAGE_SIZE) {
        log_message("Message too large, dropping broadcast");
        metrics.messages_dropped++;
        return;
    }

    // Pre-allocate the timed message
    std::string timed_message;
    {
        std::time_t now = std::time(nullptr);
        char time_buffer[20];
        std::strftime(time_buffer, sizeof(time_buffer), "[%H:%M:%S] ", std::localtime(&now));
        timed_message.reserve(strlen(time_buffer) + message.length());
        timed_message += time_buffer;
        timed_message += message;
    }

    metrics.record_message("broadcast");
    metrics.record_bytes(timed_message.length() * (metrics.current_connections.load() - 1));

    // Store in chat history
    {
        std::lock_guard<std::mutex> lock(history_mtx);
        chat_history.push_back(timed_message);
        if (chat_history.size() > MAX_HISTORY_SIZE) {
            chat_history.erase(chat_history.begin());
        }
    }

    // Track failed connections for batch cleanup
    std::vector<Connection*> failed_connections;
    failed_connections.reserve(8);  // Reserve space for potential failures

    // Send to all active connections
    {
        std::lock_guard<std::mutex> lock(pool_mtx);
        for (auto& conn : connection_pool) {
            if (!conn.in_use || conn.socket == sender) {
                continue;
            }

            if (!send_with_retry(conn.socket, timed_message)) {
                failed_connections.push_back(&conn);
                log_message("Failed to broadcast to client " + conn.username);
            } else {
                conn.last_activity = std::chrono::steady_clock::now();
            }
        }
    }

    // Cleanup failed connections
    for (auto* conn : failed_connections) {
        release_connection(conn);
    }

    auto end = std::chrono::steady_clock::now();
    double latency = std::chrono::duration<double, std::milli>(end - start).count();
    metrics.record_message("broadcast", latency);
}

void message_worker() {
    while (true) {
        try {
            Message msg = message_queue.pop();
            auto start = std::chrono::steady_clock::now();

            // Check if sender is still connected
            bool sender_connected = false;
            {
                std::lock_guard<std::mutex> lock(pool_mtx);
                for (const auto& conn : connection_pool) {
                    if (conn.in_use && conn.socket == msg.sender_socket) {
                        sender_connected = true;
                        break;
                    }
                }
            }

            if (!sender_connected) {
                log_message("Message from disconnected client " + std::to_string(msg.sender_socket));
                continue;
            }

            // Process message in batches for better performance
            if (msg.content[0] == '/') {
                // Handle commands
                process_command(msg);
            } else {
                // Handle regular messages
                std::string username;
                {
                    std::lock_guard<std::mutex> lock(pool_mtx);
                    for (const auto& conn : connection_pool) {
                        if (conn.socket == msg.sender_socket) {
                            username = conn.username;
                            break;
                        }
                    }
                }
                broadcast(msg.sender_socket, username + ": " + msg.content);
            }

            auto end = std::chrono::steady_clock::now();
            double latency = std::chrono::duration<double, std::milli>(end - start).count();
            metrics.record_message("processing", latency);
        } catch (const std::exception& e) {
            log_message("Exception in message worker: " + std::string(e.what()));
        } catch (...) {
            log_message("Unknown exception in message worker");
        }
    }
}
