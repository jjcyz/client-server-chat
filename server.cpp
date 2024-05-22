#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>
#include <ctime>

#define PORT 5555
#define BUFFER_SIZE 1024

std::vector<std::pair<int, std::string> > clients;

void broadcast(int sender, const std::string& message) {
    std::time_t now = std::time(nullptr);
    char time_buffer[20];
    std::strftime(time_buffer, sizeof(time_buffer), "[%H:%M:%S] ", std::localtime(&now));

    std::string timed_message = time_buffer + message;
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
        std::string user_list = "Active users:\n";
        for (const auto& client : clients) {
            user_list += client.second + "\n";
        }
        send(client_socket, user_list.c_str(), user_list.length(), 0);
    } else {
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

    clients.push_back(std::make_pair(client_socket, username));
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
