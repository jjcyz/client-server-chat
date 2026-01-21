#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
    static Database& getInstance();

    // User management
    bool createUser(const std::string& username, const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password);
    bool removeUser(const std::string& username);
    bool isAdmin(const std::string& username);
    int getUserID(const std::string& username);  // Returns 0 if user not found

    // Message storage
    bool storeMessage(int sender_id, int receiver_id, const std::string& content);
    std::vector<std::string> loadRecentMessages(int limit = 1000);  // Load recent broadcast messages

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
    static const int BROADCAST_RECEIVER_ID;  // Use 0 for broadcast messages
};

#endif // DATABASE_H
