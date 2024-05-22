#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>

#define PORT 5555
#define BUFFER_SIZE 1024

std::vector<std::pair<int, std::string> > clients;

void broadcast(int sender, const std::string& username, const char* message) {
    for (auto& client : clients) {
        if (client.first != sender) {
            send(client.first, (username + ": " + message).c_str(), strlen(username.c_str()) + strlen(message) + 2, 0);
        }
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

    while (true) {
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Remove disconnected client
            clients.erase(std::remove_if(clients.begin(), clients.end(),
                                          [&](const std::pair<int, std::string>& client){ return client.first == client_socket; }), clients.end());
            close(client_socket);
            break;
        } else {
            buffer[bytes_received] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            broadcast(client_socket, username, buffer);
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













