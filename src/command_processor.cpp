#include "command_processor.h"
#include "server_metrics.h"
#include "connection_pool.h"
#include "network_handler.h"
#include <sys/socket.h>
#include <cstring>

extern MessageQueue message_queue;

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
        for (const auto& type : metrics.get_message_types()) {
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

