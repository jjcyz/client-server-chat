#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>
#include <ctime>
#include <mutex>
#include <chrono> // for time
#include <atomic> // for atomic variables (performace gains)
#include <map> // for message types

#define PORT 5555
#define BUFFER_SIZE 1024

// Performance metrics
struct ServerMetrics {
    std::atomic<size_t> total_messages_processed{0};
    std::atomic<size_t> current_connections{0};
    std::atomic<size_t> peak_connections{0};
    std::atomic<size_t> total_bytes_transferred{0};
    std::chrono::steady_clock::time_point start_time;
    std::map<std::string, size_t> message_types;

    ServerMetrics() : start_time(std::chrono::steady_clock::now()) {}

    void record_message(const std::string& type) {
        total_messages_processed++;
        message_types[type]++;
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
};

ServerMetrics metrics;
std::vector<std::pair<int, std::string>> clients;
std::vector<std::string> chat_history;
std::mutex mtx;

void broadcast(int sender, const std::string& message) {
    std::time_t now = std::time(nullptr);
    char time_buffer[20];
    std::strftime(time_buffer, sizeof(time_buffer), "[%H:%M:%S] ", std::localtime(&now));

    std::string timed_message = time_buffer + message;

    // Record metrics
    metrics.record_message("broadcast");
    metrics.record_bytes(timed_message.length() * (clients.size() - 1));

    {
        std::lock_guard<std::mutex> lock(mtx);
        chat_history.push_back(timed_message);
    }

    for (auto& client : clients) {
        if (client.first != sender) {
            send(client.first, timed_message.c_str(), timed_message.length(), 0);
        }
    }
}

void send_join_message(const std::string& username) {
    std::string join_message = username + " has joined the chat";
    broadcast(-1, join_message);
}

void send_leave_message(const std::string& username) {
    std::string leave_message = username + " has left the chat";
    broadcast(-1, leave_message);
}

void handle_command(int client_socket, const std::string& command) {
    if (command.substr(0, 5) == "/msg ") {
        metrics.record_message("private");
        size_t pos = command.find(' ', 5);
        if (pos != std::string::npos) {
            std::string recipient = command.substr(5, pos - 5);
            std::string private_message = command.substr(pos + 1);

            auto it = std::find_if(clients.begin(), clients.end(),
                                   [&](const std::pair<int, std::string>& client) { return client.second == recipient; });
            if (it != clients.end()) {
                send(it->first, ("(private) " + private_message).c_str(), ("(private) " + private_message).length(), 0);
            } else {
                send(client_socket, "User not found.\n", 15, 0);
            }
        } else {
            send(client_socket, "Invalid command format.\n", 24, 0);
        }
    } else if (command == "/list") {
        metrics.record_message("list_users");
        std::string user_list = "Active users:\n";
        for (const auto& client : clients) {
            user_list += client.second + "\n";
        }
        send(client_socket, user_list.c_str(), user_list.length(), 0);
    } else if (command == "/history") {
        metrics.record_message("history");
        std::lock_guard<std::mutex> lock(mtx); // Ensure thread-safe access to chat_history
        std::string history = "[Chat History]";
        send(client_socket, history.c_str(), history.length(), 0);
        for (const auto &msg : chat_history) {
            std::string msg_with_newline = msg + "\n";
            send(client_socket, msg_with_newline.c_str(), msg_with_newline.length(), 0);
        }
    } else if (command == "/stats") {
        std::string stats = "Server Statistics:\n";
        stats += "Uptime: " + std::to_string(metrics.get_uptime_seconds()) + " seconds\n";
        stats += "Total Messages: " + std::to_string(metrics.total_messages_processed.load()) + "\n";
        stats += "Messages/Second: " + std::to_string(metrics.get_messages_per_second()) + "\n";
        stats += "Current Connections: " + std::to_string(metrics.current_connections.load()) + "\n";
        stats += "Peak Connections: " + std::to_string(metrics.peak_connections.load()) + "\n";
        stats += "Total Data Transferred: " + std::to_string(metrics.total_bytes_transferred.load()) + " bytes\n";
        stats += "Message Types:\n";
        for (const auto& type : metrics.message_types) {
            stats += "  " + type.first + ": " + std::to_string(type.second) + "\n";
        }
        send(client_socket, stats.c_str(), stats.length(), 0);
    } else {
        metrics.record_message("unknown_command");
        std::string unknown_command = "Unknown command.\n";
        send(client_socket, unknown_command.c_str(), unknown_command.length(), 0);
    }
}

void handle_client(int client_socket) {
    char username_buffer[BUFFER_SIZE];
    int bytes_received_username = recv(client_socket, username_buffer, BUFFER_SIZE, 0);
    if (bytes_received_username <= 0) {
        close(client_socket);
        return;
    }
    username_buffer[bytes_received_username] = '\0';
    std::string username(username_buffer);

    {
        std::lock_guard<std::mutex> lock(mtx);
        clients.push_back(std::make_pair(client_socket, username));
        metrics.update_connections(clients.size());
    }
    send_join_message(username);

    while (true) {
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Remove disconnected client
            auto it = std::find_if(clients.begin(), clients.end(),
                                   [&](const std::pair<int, std::string>& client){ return client.first == client_socket; });
            if (it != clients.end()) {
                send_leave_message(it->second);
                clients.erase(it);
            }
            close(client_socket);
            break;
        } else {
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            if (message[0] == '/') {
                handle_command(client_socket, message);
            } else {
                broadcast(client_socket, username + ": " + message);
            }
        }
    }

    // Update metrics when client disconnects
    {
        std::lock_guard<std::mutex> lock(mtx);
        metrics.update_connections(clients.size());
    }
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Error: Could not create socket." << std::endl;
        return 1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Error: Could not bind to port " << PORT << "." << std::endl;
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Error: Could not listen on port " << PORT << "." << std::endl;
        close(server_socket);
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);
        if (client_socket == -1) {
            std::cerr << "Error: Could not accept incoming connection." << std::endl;
        } else {
            std::thread(handle_client, client_socket).detach();
        }
    }

    close(server_socket);
    return 0;
}
