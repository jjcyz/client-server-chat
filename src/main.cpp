#include "network_handler.h"
#include "connection_pool.h"
#include "message_queue.h"
#include "server_metrics.h"
#include "command_processor.h"
#include "socket_utils.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include "constants.h"


int main() {
    try {
        initialize_connection_pool();
        log_message("Initialized connection pool with " + std::to_string(MAX_CONNECTIONS) + " slots");

        // Start worker threads
        std::vector<std::thread> workers;
        for (int i = 0; i < WORKER_THREADS; i++) {
            workers.emplace_back(message_worker);
        }
        log_message("Started " + std::to_string(WORKER_THREADS) + " worker threads");

        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            log_message("Error: Could not create socket: " + std::string(strerror(errno)));
            return 1;
        }
        log_message("Created server socket");

        // Use socket_utils to configure the socket
        if (!configure_socket(server_socket, true)) {
            log_message("Error: Could not configure server socket");
            close(server_socket);
            return 1;
        }
        log_message("Set socket options");

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(PORT);

        if (bind(server_socket, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
            log_message("Error: Could not bind to port " + std::to_string(PORT) + ": " + std::string(strerror(errno)));
            close(server_socket);
            return 1;
        }
        log_message("Bound to port " + std::to_string(PORT));

        if (listen(server_socket, SOMAXCONN) == -1) {
            log_message("Error: Could not listen on port " + std::to_string(PORT) + ": " + std::string(strerror(errno)));
            close(server_socket);
            return 1;
        }

        log_message("Server is listening on port " + std::to_string(PORT) + "...");
        log_message("Maximum concurrent connections: " + std::to_string(MAX_CONNECTIONS));
        log_message("Worker threads: " + std::to_string(WORKER_THREADS));

        while (true) {
            try {
                sockaddr_in client_address;
                socklen_t client_address_size = sizeof(client_address);
                int client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);
                if (client_socket == -1) {
                    log_message("Error: Could not accept incoming connection: " + std::string(strerror(errno)));
                    continue;
                }

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
                log_message("New connection from " + std::string(client_ip) + ":" + std::to_string(ntohs(client_address.sin_port)));

                std::thread(handle_client, client_socket).detach();
            } catch (const std::exception& e) {
                log_message("Exception in accept loop: " + std::string(e.what()));
            } catch (...) {
                log_message("Unknown exception in accept loop");
            }
        }

        close(server_socket);
        return 0;
    } catch (const std::exception& e) {
        log_message("Fatal exception: " + std::string(e.what()));
        return 1;
    } catch (...) {
        log_message("Unknown fatal exception");
        return 1;
    }
}
