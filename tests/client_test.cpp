// tests/client_test.cpp
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <fcntl.h>

// Mock server to test client connections
class MockServer {
private:
    int server_socket;
    int port;
    std::atomic<bool> running;
    std::thread server_thread;
    std::mutex mtx;
    std::vector<std::string> received_messages;

public:
    explicit MockServer(int port = 5555) : port(port), running(false) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_GE(server_socket, 0) << "Failed to create mock server socket";

        // Set socket to non-blocking mode
        int flags = fcntl(server_socket, F_GETFL, 0);
        fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        int reuse = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        EXPECT_NE(-1, bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
            << "Failed to bind mock server socket";

        EXPECT_NE(-1, listen(server_socket, 1)) << "Failed to listen on mock server socket";
    }

    void start() {
        running = true;
        server_thread = std::thread([this]() { run(); });
    }

    void stop() {
        running = false;
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    std::vector<std::string> getReceivedMessages() {
        std::lock_guard<std::mutex> lock(mtx);
        return received_messages;
    }

    ~MockServer() {
        stop();
    }

private:
    void handleClient(int client_socket) {
        char buffer[1024];
        int total_received = 0;

        while (running && total_received < sizeof(buffer) - 1) {
            int bytes_received = recv(client_socket, buffer + total_received,
                                   sizeof(buffer) - total_received - 1, 0);
            if (bytes_received <= 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                break;
            }
            total_received += bytes_received;

            // Look for message boundary
            for (int i = 0; i < total_received; i++) {
                if (buffer[i] == '\0' || buffer[i] == '\n') {
                    buffer[i] = '\0';
                    if (i > 0) {
                        std::lock_guard<std::mutex> lock(mtx);
                        received_messages.emplace_back(buffer);
                    }
                    // Move remaining data to start of buffer
                    total_received -= (i + 1);
                    if (total_received > 0) {
                        memmove(buffer, buffer + i + 1, total_received);
                    }
                    break;
                }
            }
        }
    }

    void run() {
        while (running) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                break;
            }

            // Set socket options
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

            // Handle client in a separate thread
            std::thread([this, client_socket]() {
                handleClient(client_socket);
                close(client_socket);
            }).detach();
        }
    }
};

class ClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_server = std::make_unique<MockServer>();
        mock_server->start();
        // Give the server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        mock_server->stop();
    }

    std::unique_ptr<MockServer> mock_server;
};

// Test basic connection
TEST_F(ClientTest, TestConnection) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_socket, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    ASSERT_NE(-1, inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr));

    ASSERT_NE(-1, connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)));

    // Send username
    std::string username = "test_user";
    ASSERT_NE(-1, send(client_socket, username.c_str(), username.length(), 0));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto messages = mock_server->getReceivedMessages();
    ASSERT_FALSE(messages.empty());
    EXPECT_EQ(messages[0], username);

    close(client_socket);
}

// Test message sending
TEST_F(ClientTest, TestMessageSending) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_socket, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    ASSERT_NE(-1, inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr));

    ASSERT_NE(-1, connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)));

    // Send username
    std::string username = "test_user";
    ASSERT_NE(-1, send(client_socket, username.c_str(), username.length(), 0));

    // Wait for username to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send test message
    std::string test_message = "Hello, server!";
    ASSERT_NE(-1, send(client_socket, test_message.c_str(), test_message.length(), 0));

    // Wait longer to ensure message is processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check messages with retry
    std::vector<std::string> messages;
    for (int retry = 0; retry < 5; retry++) {
        messages = mock_server->getReceivedMessages();
        if (messages.size() >= 2) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ASSERT_GE(messages.size(), 2) << "Expected at least 2 messages (username and test message)";
    EXPECT_EQ(messages[0], username) << "First message should be username";
    EXPECT_EQ(messages[1], test_message) << "Second message should be test message";

    close(client_socket);
}

// Test message receiving
TEST_F(ClientTest, TestMessageReceiving) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_socket, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    ASSERT_NE(-1, inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr));

    ASSERT_NE(-1, connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)));

    // Send username
    std::string username = "test_user";
    ASSERT_NE(-1, send(client_socket, username.c_str(), username.length(), 0));

    // Send message and expect echo
    std::string test_message = "Echo test";
    ASSERT_NE(-1, send(client_socket, test_message.c_str(), test_message.length(), 0));

    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    ASSERT_GT(bytes_received, 0);
    buffer[bytes_received] = '\0';
    EXPECT_EQ(std::string(buffer), test_message);

    close(client_socket);
}

// Test connection failure
TEST_F(ClientTest, TestConnectionFailure) {
    mock_server->stop();  // Stop the server to test connection failure

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_socket, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    ASSERT_NE(-1, inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr));

    EXPECT_EQ(-1, connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)));
    close(client_socket);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
