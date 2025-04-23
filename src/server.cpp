#include "server.h"
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

// Constants
#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 200
#define MESSAGE_QUEUE_SIZE 10000
#define WORKER_THREADS 4
#define MAX_MESSAGE_SIZE 4096
#define CONNECTION_TIMEOUT 30
#define MAX_RETRY_ATTEMPTS 5
#define MAX_HISTORY_SIZE 100
#define MAX_LATENCY_SAMPLES 1000

// Connection pool structure
struct Connection {
    int socket;
    std::string username;
    bool in_use;
    std::chrono::steady_clock::time_point last_activity;
};

// Thread-safe message queue
class MessageQueue {
private:
    std::queue<Message> queue;
    std::mutex mtx;
    std::condition_variable cv;
    size_t max_size;
    std::atomic<size_t> current_size{0};

public:
    MessageQueue(size_t size) : max_size(size) {}

    bool push(Message msg) {
        std::unique_lock<std::mutex> lock(mtx);
        if (current_size >= max_size) {
            return false;
        }
        queue.push(msg);
        current_size++;
        cv.notify_one();
        return true;
    }

    Message pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty(); });
        Message msg = queue.front();
        queue.pop();
        current_size--;
        return msg;
    }

    size_t size() const {
        return current_size;
    }
};

// Performance metrics
struct ServerMetrics {
    std::atomic<size_t> total_messages_processed{0};
    std::atomic<size_t> current_connections{0};
    std::atomic<size_t> peak_connections{0};
    std::atomic<size_t> total_bytes_transferred{0};
    std::atomic<size_t> messages_dropped{0};
    std::chrono::steady_clock::time_point start_time;
    std::map<std::string, size_t> message_types;
    std::vector<double> message_latencies;

    ServerMetrics() : start_time(std::chrono::steady_clock::now()) {}

    void record_message(const std::string& type, double latency = 0) {
        total_messages_processed++;
        message_types[type]++;
        if (latency > 0) {
            if (message_latencies.size() >= MAX_LATENCY_SAMPLES) {
                message_latencies.erase(message_latencies.begin());
            }
            message_latencies.push_back(latency);
        }
    }

    void record_bytes(size_t bytes) {
        total_bytes_transferred += bytes;
    }

    void update_connections(size_t count) {
        current_connections = count;
        peak_connections = std::max(peak_connections.load(), count);
    }

    double get_uptime_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }

    double get_messages_per_second() const {
        return total_messages_processed.load() / get_uptime_seconds();
    }

    double get_average_latency() const {
        if (message_latencies.empty()) return 0;
        double sum = 0;
        for (double latency : message_latencies) {
            sum += latency;
        }
        return sum / message_latencies.size();
    }
};

ServerMetrics metrics;
std::vector<Connection> connection_pool(MAX_CONNECTIONS);
std::mutex pool_mtx;
MessageQueue message_queue(MESSAGE_QUEUE_SIZE);
std::vector<std::string> chat_history;
std::mutex history_mtx;
std::mutex console_mtx;  // Add this near other mutex declarations

void initialize_connection_pool() {
    std::lock_guard<std::mutex> lock(pool_mtx);
    // No need to clear and reinitialize since it's already initialized with MAX_CONNECTIONS
    log_message("Connection pool initialized with " + std::to_string(MAX_CONNECTIONS) + " slots");
}

Connection* get_available_connection() {
    std::lock_guard<std::mutex> lock(pool_mtx);
    auto now = std::chrono::steady_clock::now();

    // First try to find an unused connection
    for (auto& conn : connection_pool) {
        if (!conn.in_use) {
            conn.in_use = true;
            conn.last_activity = now;
            return &conn;
        }
    }

    // If no unused connections, try to find a stale connection
    for (auto& conn : connection_pool) {
        if (conn.in_use) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - conn.last_activity).count();
            if (duration > CONNECTION_TIMEOUT) {
                // Connection is stale, clean it up
                if (conn.socket != -1) {
                    close(conn.socket);
                    log_message("Cleaned up stale connection from " + conn.username);
                }
                conn.socket = -1;
                conn.in_use = true;
                conn.last_activity = now;
                return &conn;
            }
        }
    }

    log_message("No available connections in pool");
    return nullptr;
}

void release_connection(Connection* conn) {
    if (!conn) return;

    std::lock_guard<std::mutex> lock(pool_mtx);
    if (conn->socket != -1) {
        close(conn->socket);
        log_message("Closed socket for connection " + conn->username);
    }
    conn->socket = -1;
    conn->in_use = false;
    conn->username.clear();
    log_message("Released connection from pool");
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

        // Set socket options for better performance
        int opt = 1;
        int socket_buffer_size = 256 * 1024;  // Increased from 64KB to 256KB

        // Consolidate socket options
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

void process_command(const Message& msg) {
    if (msg.content == "/stats") {
        std::string stats = "Server Statistics:\n";
        stats += "Uptime: " + std::to_string(metrics.get_uptime_seconds()) + " seconds\n";
        stats += "Total Messages: " + std::to_string(metrics.total_messages_processed.load()) + "\n";
        stats += "Messages/Second: " + std::to_string(metrics.get_messages_per_second()) + "\n";
        stats += "Current Connections: " + std::to_string(metrics.current_connections.load()) + "\n";
        stats += "Peak Connections: " + std::to_string(metrics.peak_connections.load()) + "\n";
        stats += "Total Data Transferred: " + std::to_string(metrics.total_bytes_transferred.load()) + " bytes\n";
        stats += "Average Message Latency: " + std::to_string(metrics.get_average_latency()) + " ms\n";
        stats += "Messages Dropped: " + std::to_string(metrics.messages_dropped.load()) + "\n";
        stats += "Message Types:\n";
        for (const auto& type : metrics.message_types) {
            stats += "  " + type.first + ": " + std::to_string(type.second) + "\n";
        }

        if (send(msg.sender_socket, stats.c_str(), stats.length(), 0) <= 0) {
            log_message("Failed to send stats to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
        }
        metrics.record_message("stats");
    } else if (msg.content == "/list") {
        std::string user_list = "Active users:\n";
        {
            std::lock_guard<std::mutex> lock(pool_mtx);
            for (const auto& conn : connection_pool) {
                if (conn.in_use) {
                    user_list += conn.username + "\n";
                }
            }
        }
        if (send(msg.sender_socket, user_list.c_str(), user_list.length(), 0) <= 0) {
            log_message("Failed to send user list to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
        }
        metrics.record_message("list_users");
    } else if (msg.content.substr(0, 5) == "/msg ") {
        size_t pos = msg.content.find(' ', 5);
        if (pos != std::string::npos) {
            std::string recipient = msg.content.substr(5, pos - 5);
            std::string private_message = msg.content.substr(pos + 1);
            bool found = false;

            {
                std::lock_guard<std::mutex> lock(pool_mtx);
                for (const auto& conn : connection_pool) {
                    if (conn.in_use && conn.username == recipient) {
                        std::string sender_username;
                        for (const auto& sender_conn : connection_pool) {
                            if (sender_conn.socket == msg.sender_socket) {
                                sender_username = sender_conn.username;
                                break;
                            }
                        }
                        std::string full_message = "(private from " + sender_username + ") " + private_message;
                        if (send(conn.socket, full_message.c_str(), full_message.length(), 0) <= 0) {
                            log_message("Failed to send private message to " + recipient + ": " + std::string(strerror(errno)));
                        }
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                std::string not_found = "User not found.\n";
                if (send(msg.sender_socket, not_found.c_str(), not_found.length(), 0) <= 0) {
                    log_message("Failed to send not found message to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
                }
            }
            metrics.record_message("private");
            } else {
            std::string invalid = "Invalid command format.\n";
            if (send(msg.sender_socket, invalid.c_str(), invalid.length(), 0) <= 0) {
                log_message("Failed to send invalid command message to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
            }
        }
    } else {
        std::string unknown = "Unknown command.\n";
        if (send(msg.sender_socket, unknown.c_str(), unknown.length(), 0) <= 0) {
            log_message("Failed to send unknown command message to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
        }
        metrics.record_message("unknown_command");
    }
}
