#pragma once

#include <sqlite3.h>
#include <string>
#include <memory>
#include <vector>
#include <chrono>

struct User {
    int id;
    std::string username;
    std::string password_hash;
    std::string salt;
    std::chrono::system_clock::time_point created_at;
};

struct Session {
    std::string session_id;
    int user_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
};

class Database {
public:
    static Database& getInstance();

    // User management
    bool createUser(const std::string& username, const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password);
    User getUserByUsername(const std::string& username);

    // Session management
    std::string createSession(int user_id);
    bool validateSession(const std::string& session_id);
    void invalidateSession(const std::string& session_id);

    // Message history (optional)
    bool storeMessage(int sender_id, int receiver_id, const std::string& content);
    std::vector<std::string> getMessageHistory(int user_id, int other_user_id, int limit = 100);

private:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool initializeDatabase();
    bool executeQuery(const std::string& query);
    std::string hashPassword(const std::string& password, const std::string& salt);
    std::string generateSalt();

    sqlite3* db;
    static const char* DATABASE_PATH;
    static const int SESSION_DURATION_HOURS = 24;
};
