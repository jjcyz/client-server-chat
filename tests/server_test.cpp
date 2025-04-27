// tests/server_test.cpp
#include <gtest/gtest.h>
#include "server.h"
#include <thread>
#include <chrono>

class ServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize the connection pool before each test
        initialize_connection_pool();

        // Clear chat history
        std::lock_guard<std::mutex> lock(history_mtx);
        chat_history.clear();
    }

    void TearDown() override {
        // Clean up after each test
        std::lock_guard<std::mutex> lock(history_mtx);
        chat_history.clear();
    }
};

// Test message logging
TEST_F(ServerTest, LogMessageTest) {
    std::string test_message = "Test log message";
    EXPECT_NO_THROW(log_message(test_message));
}

// Test chat history
TEST_F(ServerTest, ChatHistoryTest) {
    std::string test_message = "Test chat message";
    {
        std::lock_guard<std::mutex> lock(history_mtx);
        chat_history.push_back(test_message);
        EXPECT_EQ(chat_history.back(), test_message);
        EXPECT_EQ(chat_history.size(), 1);
    }
}

// Test command processing
TEST_F(ServerTest, ProcessCommandTest) {
    // Test /stats command
    Message stats_msg{1, "/stats"};
    EXPECT_NO_THROW(process_command(stats_msg));

    // Test /list command
    Message list_msg{1, "/list"};
    EXPECT_NO_THROW(process_command(list_msg));

    // Test /msg command
    Message msg_command{1, "/msg 2 Hello!"};
    EXPECT_NO_THROW(process_command(msg_command));
}

// Test invalid commands
TEST_F(ServerTest, InvalidCommandTest) {
    Message invalid_msg{1, "/invalid_command"};
    EXPECT_NO_THROW(process_command(invalid_msg));
}

// Test message worker
TEST_F(ServerTest, MessageWorkerTest) {
    std::thread worker_thread(message_worker);
    worker_thread.detach();  // Let it run in background

    // Give some time for worker to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Test client handling
TEST_F(ServerTest, HandleClientTest) {
    // Create a mock client socket
    int mock_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(mock_socket, 0);

    // Test in separate thread since handle_client blocks
    std::thread client_thread(handle_client, mock_socket);
    client_thread.detach();

    // Give some time for client handler to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cleanup
    close(mock_socket);
}

// Test concurrent access to chat history
TEST_F(ServerTest, ConcurrentHistoryAccessTest) {
    const int NUM_THREADS = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i]() {
            std::string msg = "Message from thread " + std::to_string(i);
            std::lock_guard<std::mutex> lock(history_mtx);
            chat_history.push_back(msg);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::lock_guard<std::mutex> lock(history_mtx);
    EXPECT_EQ(chat_history.size(), NUM_THREADS);
}

// Test console mutex
TEST_F(ServerTest, ConsoleMutexTest) {
    const int NUM_THREADS = 5;
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i]() {
            std::string msg = "Log from thread " + std::to_string(i);
            log_message(msg);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

// Test message size limits
TEST_F(ServerTest, MessageSizeLimitTest) {
    std::string long_message(MAX_MESSAGE_SIZE + 1, 'a');
    Message large_msg{1, long_message};
    EXPECT_NO_THROW(process_command(large_msg));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
