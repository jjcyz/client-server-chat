#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <functional>

#define PORT 5555
#define BUFFER_SIZE 1024

void receive_messages(int client_socket, bool& authenticated) {
    while (true) {
        char buffer[BUFFER_SIZE + 1];
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cout << "Connection closed." << std::endl;
            break;
        } else {
            buffer[bytes_received] = '\0';
            std::string message(buffer);
            std::cout << message << std::endl;
            if (message.find("Login successful!") != std::string::npos ||
                message.find("Registration successful!") != std::string::npos) {
                authenticated = true;
            }
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

    bool authenticated = false;
    std::thread receiver(receive_messages, client_socket, std::ref(authenticated));

    // Authentication loop
    while (!authenticated) {
        std::cout << "Do you want to (l)ogin or (r)egister? ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice != "l" && choice != "r") {
            std::cout << "Invalid choice. Enter 'l' to login or 'r' to register." << std::endl;
            continue;
        }
        std::string username, password;
        std::cout << "Username: ";
        std::getline(std::cin, username);
        std::cout << "Password: ";
        std::getline(std::cin, password);
        std::string command = (choice == "l" ? "/login " : "/register ") + username + " " + password;
        send(client_socket, command.c_str(), command.length(), 0);
        // Wait for server response
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "You are now authenticated. You can start chatting!" << std::endl;
    while (true) {
        std::string message;
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        send(client_socket, message.c_str(), message.length(), 0);
    }

    close(client_socket);
    receiver.join();
    return 0;
}

