#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5555
#define BUFFER_SIZE 1024

void receive_messages(int client_socket) {
    while (true) {
        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cout << "Connection closed." << std::endl;
            break;
        } else {
            buffer[bytes_received] = '\0';
            std::cout << buffer << std::endl;
        }
    }
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Error: Could not create socket." << std::endl;
        return 1;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Error: Could not connect to server." << std::endl;
        close(client_socket);
        return 1;
    }

    std::cout << "Enter your username: ";
    std::string username;
    std::getline(std::cin, username);
    send(client_socket, username.c_str(), username.length(), 0);

    std::thread(receive_messages, client_socket).detach();


    while (true) {
        std::string message;
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        send(client_socket, message.c_str(), message.length(), 0);
    }

    close(client_socket);
    return 0;
}




