#include "connection_pool.h"
#include "server_metrics.h"
#include <unistd.h>
#include <chrono>
#include <string>

// Global variables
std::vector<Connection> connection_pool(MAX_CONNECTIONS);
std::mutex pool_mtx;

// Forward declaration of log_message
void log_message(const std::string& message);

void initialize_connection_pool() {
    std::lock_guard<std::mutex> lock(pool_mtx);
    // No need to clear and reinitialize since it's already initialized with MAX_CONNECTIONS
    log_message("Connection pool initialized with " + std::to_string(MAX_CONNECTIONS) + " slots");
}

Connection* get_available_connection() {
    std::lock_guard<std::mutex> lock(pool_mtx);
    auto now = std::chrono::steady_clock::now();

    // First try to find an unused connection
    for (auto& conn : connection_pool) {
        if (!conn.in_use) {
            conn.in_use = true;
            conn.last_activity = now;
            return &conn;
        }
    }

    // If no unused connections, try to find a stale connection
    for (auto& conn : connection_pool) {
        if (conn.in_use) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - conn.last_activity).count();
            if (duration > CONNECTION_TIMEOUT) {
                // Connection is stale, clean it up
                if (conn.socket != -1) {
                    close(conn.socket);
                    log_message("Cleaned up stale connection from " + conn.username);
                }
                conn.socket = -1;
                conn.in_use = true;
                conn.last_activity = now;
                return &conn;
            }
        }
    }

    log_message("No available connections in pool");
    return nullptr;
}

void release_connection(Connection* conn) {
    if (!conn) return;

    std::lock_guard<std::mutex> lock(pool_mtx);
    if (conn->socket != -1) {
        close(conn->socket);
        log_message("Closed socket for connection " + conn->username);
    }
    conn->socket = -1;
    conn->in_use = false;
    conn->username.clear();
    log_message("Released connection from pool");
}
