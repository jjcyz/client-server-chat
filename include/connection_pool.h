#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "constants.h"
#include <string>
#include <chrono>
#include <vector>
#include <mutex>

// Connection pool structure
struct Connection {
    int socket;
    std::string username;
    bool in_use;
    bool authenticated = false;
    std::chrono::steady_clock::time_point last_activity;
};

// Forward declarations
void initialize_connection_pool();
Connection* get_available_connection();
void release_connection(Connection* conn);

// Global variables
extern std::vector<Connection> connection_pool;
extern std::mutex pool_mtx;

#endif // CONNECTION_POOL_H
