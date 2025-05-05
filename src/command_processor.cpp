#include "command_processor.h"
#include "server_metrics.h"
#include "connection_pool.h"
#include "network_handler.h"
#include "database.h"
#include <sys/socket.h>
#include <cstring>
#include <unordered_map>
#include <functional>

using CommandHandler = std::function<void(const Message&)>;

static std::unordered_map<std::string, CommandHandler> command_map = {
    {"/stats", handle_stats},
    {"/list", handle_list},
    {"/msg", handle_msg},
    {"/register", handle_register},
    {"/login", handle_login},
    {"/removeuser", handle_removeuser}
};

extern MessageQueue message_queue;

void process_command(const Message& msg) {
    // Find the connection for this socket
    Connection* conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(pool_mtx);
        for (auto& c : connection_pool) {
            if (c.in_use && c.socket == msg.sender_socket) {
                conn = &c;
                break;
            }
        }
    }
    if (!conn) return;

    const auto space_pos = msg.content.find(' ');
    const std::string_view command(msg.content.data(),
        space_pos == std::string::npos ? msg.content.length() : space_pos);

    // Only allow /login and /register if not authenticated
    if (!conn->authenticated && command != "/login" && command != "/register") {
        std::string reply = "You must log in or register before using chat commands.\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        return;
    }

    if (auto it = command_map.find(std::string(command)); it != command_map.end()) {
        it->second(msg);
    } else {
        handle_unknown(msg);
    }
}

// Handler for /stats command
void handle_stats(const Message& msg) {
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
    for (const auto& type : metrics.get_message_types()) {
        stats += "  " + type.first + ": " + std::to_string(type.second) + "\n";
    }

    if (send(msg.sender_socket, stats.c_str(), stats.length(), 0) <= 0) {
        log_message("Failed to send stats to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
    }
    metrics.record_message("stats");
}

// Handler for /list command
void handle_list(const Message& msg) {
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
}

// Handler for /msg command
void handle_msg(const Message& msg) {
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
        std::string invalid = "Invalid command format or user does not exist.\n";
        if (send(msg.sender_socket, invalid.c_str(), invalid.length(), 0) <= 0) {
            log_message("Failed to send invalid command message to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
        }
    }
}

// Handler for unknown commands
void handle_unknown(const Message& msg) {
    std::string unknown = "Unknown command.\n";
    if (send(msg.sender_socket, unknown.c_str(), unknown.length(), 0) <= 0) {
        log_message("Failed to send unknown command message to client " + std::to_string(msg.sender_socket) + ": " + std::string(strerror(errno)));
    }
    metrics.record_message("unknown_command");
}

void handle_register(const Message& msg) {
    // Expected format: /register username password
    size_t first_space = msg.content.find(' ');
    size_t second_space = msg.content.find(' ', first_space + 1);
    if (first_space == std::string::npos || second_space == std::string::npos) {
        std::string reply = "Usage: /register <username> <password>\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        return;
    }
    std::string username = msg.content.substr(first_space + 1, second_space - first_space - 1);
    std::string password = msg.content.substr(second_space + 1);
    Database& db = Database::getInstance();
    if (db.createUser(username, password)) {
        std::string reply = "Registration successful!\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        // Set authenticated flag
        std::lock_guard<std::mutex> lock(pool_mtx);
        for (auto& c : connection_pool) {
            if (c.in_use && c.socket == msg.sender_socket) {
                c.authenticated = true;
                c.username = username;
                break;
            }
        }
    } else {
        std::string reply = "Registration failed (user may already exist).\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
    }
}

void handle_login(const Message& msg) {
    // Expected format: /login username password
    size_t first_space = msg.content.find(' ');
    size_t second_space = msg.content.find(' ', first_space + 1);
    if (first_space == std::string::npos || second_space == std::string::npos) {
        std::string reply = "Usage: /login <username> <password>\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        return;
    }
    std::string username = msg.content.substr(first_space + 1, second_space - first_space - 1);
    std::string password = msg.content.substr(second_space + 1);
    Database& db = Database::getInstance();
    if (db.authenticateUser(username, password)) {
        std::string reply = "Login successful!\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        // Set authenticated flag
        std::lock_guard<std::mutex> lock(pool_mtx);
        for (auto& c : connection_pool) {
            if (c.in_use && c.socket == msg.sender_socket) {
                c.authenticated = true;
                c.username = username;
                break;
            }
        }
    } else {
        std::string reply = "Login failed.\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
    }
}

void handle_removeuser(const Message& msg) {
    // Expected format: /removeuser username
    size_t first_space = msg.content.find(' ');
    if (first_space == std::string::npos) {
        std::string reply = "Usage: /removeuser <username>\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        return;
    }
    std::string target_username = msg.content.substr(first_space + 1);
    // Find the sender's username
    std::string sender_username;
    {
        std::lock_guard<std::mutex> lock(pool_mtx);
        for (const auto& c : connection_pool) {
            if (c.in_use && c.socket == msg.sender_socket) {
                sender_username = c.username;
                break;
            }
        }
    }
    Database& db = Database::getInstance();
    if (!db.isAdmin(sender_username)) {
        std::string reply = "Permission denied. Only admins can remove users.\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
        return;
    }
    if (db.removeUser(target_username)) {
        std::string reply = "User '" + target_username + "' removed successfully.\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
    } else {
        std::string reply = "Failed to remove user '" + target_username + "'.\n";
        send(msg.sender_socket, reply.c_str(), reply.length(), 0);
    }
}
