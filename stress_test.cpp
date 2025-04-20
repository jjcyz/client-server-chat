#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <chrono>
#include <atomic>
#include <random>
#include <mutex>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5555
#define NUM_CLIENTS 100
#define MESSAGES_PER_CLIENT 10
#define MESSAGE_INTERVAL_MS 100
#define MAX_CONCURRENT_THREADS 50  // Increased from 20 to 50
#define MAX_RETRIES 3
#define RETRY_DELAY_MS 1000
#define CONNECTION_BATCH_SIZE 20   // Increased from 10 to 20
#define BATCH_DELAY_MS 100        // Reduced from 200 to 100

std::atomic<int> successful_connections{0};
std::atomic<int> failed_connections{0};
std::atomic<int> total_messages_sent{0};
std::vector<double> latencies;
std::mutex latencies_mutex;

void client_thread(int client_id) {
    int retries = 0;
    while (retries < MAX_RETRIES) {
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            std::cerr << "Client " << client_id << " failed to create socket: " << strerror(errno) << std::endl;
            failed_connections++;
            return;
        }

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(PORT);
        if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
            std::cerr << "Client " << client_id << " invalid address: " << strerror(errno) << std::endl;
            failed_connections++;
            close(client_socket);
            return;
        }

        std::cout << "Client " << client_id << " attempting to connect (attempt " << retries + 1 << "/" << MAX_RETRIES << ")..." << std::endl;
        if (connect(client_socket, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
            std::cerr << "Client " << client_id << " failed to connect: " << strerror(errno) << std::endl;
            close(client_socket);
            retries++;
            if (retries < MAX_RETRIES) {
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue;
            }
            failed_connections++;
            return;
        }

        std::cout << "Client " << client_id << " connected successfully" << std::endl;
        successful_connections++;

        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = 5;  // 5 second timeout
        tv.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        // Send username
        std::string username = "stress_test_" + std::to_string(client_id);
        if (send(client_socket, username.c_str(), username.length(), 0) <= 0) {
            std::cerr << "Client " << client_id << " failed to send username: " << strerror(errno) << std::endl;
            close(client_socket);
            continue;
        }

        bool connection_error = false;
        // Send messages
        for (int i = 0; i < MESSAGES_PER_CLIENT; i++) {
            auto start = std::chrono::high_resolution_clock::now();

            std::string message = "Message " + std::to_string(i) + " from client " + std::to_string(client_id);
            if (send(client_socket, message.c_str(), message.length(), 0) <= 0) {
                std::cerr << "Client " << client_id << " failed to send message " << i << ": " << strerror(errno) << std::endl;
                connection_error = true;
                break;
            }

            // Wait for response
            char buffer[1024];
            if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
                std::cerr << "Client " << client_id << " failed to receive response for message " << i << ": " << strerror(errno) << std::endl;
                connection_error = true;
                break;
            }

            auto end = std::chrono::high_resolution_clock::now();
            double latency = std::chrono::duration<double, std::milli>(end - start).count();

            {
                std::lock_guard<std::mutex> lock(latencies_mutex);
                latencies.push_back(latency);
            }

            total_messages_sent++;
            std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_INTERVAL_MS));
        }

        close(client_socket);
        if (!connection_error) {
            // If we completed successfully, break out of retry loop
            break;
        }
        retries++;
        if (retries < MAX_RETRIES) {
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        }
    }
}

int main() {
    std::cout << "Starting stress test with " << NUM_CLIENTS << " clients..." << std::endl;
    std::cout << "Maximum concurrent threads: " << MAX_CONCURRENT_THREADS << std::endl;
    std::cout << "Messages per client: " << MESSAGES_PER_CLIENT << std::endl;
    std::cout << "Message interval: " << MESSAGE_INTERVAL_MS << "ms" << std::endl;
    std::cout << "Connection batch size: " << CONNECTION_BATCH_SIZE << std::endl;
    std::cout << "Batch delay: " << BATCH_DELAY_MS << "ms" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(NUM_CLIENTS);

    // Process clients in smaller batches
    for (int i = 0; i < NUM_CLIENTS; i += CONNECTION_BATCH_SIZE) {
        int batch_size = std::min(CONNECTION_BATCH_SIZE, NUM_CLIENTS - i);

        // Wait if we have too many active threads
        while (threads.size() >= MAX_CONCURRENT_THREADS) {
            for (auto it = threads.begin(); it != threads.end();) {
                if (it->joinable()) {
                    it->join();
                    it = threads.erase(it);
                } else {
                    ++it;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Start a batch of connections
        for (int j = 0; j < batch_size; j++) {
            threads.emplace_back(client_thread, i + j);
        }

        // Wait before starting the next batch
        std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_DELAY_MS));
    }

    // Wait for remaining threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double>(end_time - start_time).count();

    // Calculate statistics
    double avg_latency = 0;
    double max_latency = 0;
    double min_latency = std::numeric_limits<double>::max();
    {
        std::lock_guard<std::mutex> lock(latencies_mutex);
        for (double latency : latencies) {
            avg_latency += latency;
            max_latency = std::max(max_latency, latency);
            min_latency = std::min(min_latency, latency);
        }
        if (!latencies.empty()) {
            avg_latency /= latencies.size();
        }
    }

    std::cout << "\nStress Test Results:" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "Total Time: " << total_time << " seconds" << std::endl;
    std::cout << "Successful Connections: " << successful_connections << " ("
              << (static_cast<double>(successful_connections) / NUM_CLIENTS * 100) << "%)" << std::endl;
    std::cout << "Failed Connections: " << failed_connections << " ("
              << (static_cast<double>(failed_connections) / NUM_CLIENTS * 100) << "%)" << std::endl;
    std::cout << "Total Messages Sent: " << total_messages_sent << std::endl;
    std::cout << "Messages/Second: " << total_messages_sent / total_time << std::endl;
    std::cout << "Average Latency: " << avg_latency << " ms" << std::endl;
    std::cout << "Min Latency: " << min_latency << " ms" << std::endl;
    std::cout << "Max Latency: " << max_latency << " ms" << std::endl;
    std::cout << "Total Latency Samples: " << latencies.size() << std::endl;

    return 0;
}
