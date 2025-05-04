#include "../include/database.h"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <ctime>
#include <random>
#include <algorithm>

const char* Database::DATABASE_PATH = "chat_server.db";

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::Database() : db(nullptr) {
    if (!initializeDatabase()) {
        throw std::runtime_error("Failed to initialize database");
    }
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::initializeDatabase() {
    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc) {
        return false;
    }

    // Create users table
    const char* createUsersTable =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "salt TEXT NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    // Create sessions table
    const char* createSessionsTable =
        "CREATE TABLE IF NOT EXISTS sessions ("
        "session_id TEXT PRIMARY KEY,"
        "user_id INTEGER NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "expires_at TIMESTAMP NOT NULL,"
        "FOREIGN KEY (user_id) REFERENCES users(id)"
        ");";

    // Create messages table
    const char* createMessagesTable =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender_id INTEGER NOT NULL,"
        "receiver_id INTEGER NOT NULL,"
        "content TEXT NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (sender_id) REFERENCES users(id),"
        "FOREIGN KEY (receiver_id) REFERENCES users(id)"
        ");";

    return executeQuery(createUsersTable) &&
           executeQuery(createSessionsTable) &&
           executeQuery(createMessagesTable);
}

bool Database::executeQuery(const std::string& query) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string Database::generateSalt() {
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));

    std::stringstream ss;
    for (unsigned char byte : salt) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

std::string Database::hashPassword(const std::string& password, const std::string& salt) {
    std::string salted_password = password + salt;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, salted_password.c_str(), salted_password.length()) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP SHA256 hashing failed");
    }
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool Database::createUser(const std::string& username, const std::string& password) {
    std::string salt = generateSalt();
    std::string password_hash = hashPassword(password, salt);

    std::string query = "INSERT INTO users (username, password_hash, salt) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::authenticateUser(const std::string& username, const std::string& password) {
    std::string query = "SELECT password_hash, salt FROM users WHERE username = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }

    std::string stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::string salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

    sqlite3_finalize(stmt);

    std::string computed_hash = hashPassword(password, salt);
    return stored_hash == computed_hash;
}

User Database::getUserByUsername(const std::string& username) {
    User user;
    std::string query = "SELECT id, username, password_hash, salt, created_at FROM users WHERE username = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return user; // Return empty user on error
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        // Convert timestamp to time_point
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::tm tm = {};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        user.created_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    sqlite3_finalize(stmt);
    return user;
}

std::string Database::createSession(int user_id) {
    // Generate a random session ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char* hex_chars = "0123456789abcdef";
    std::string session_id;
    for (int i = 0; i < 32; ++i) {
        session_id += hex_chars[dis(gen)];
    }

    // Calculate expiration time
    auto now = std::chrono::system_clock::now();
    auto expires_at = now + std::chrono::hours(SESSION_DURATION_HOURS);

    // Store session in database
    std::string query = "INSERT INTO sessions (session_id, user_id, expires_at) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return "";
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);

    std::time_t expires_time = std::chrono::system_clock::to_time_t(expires_at);
    std::string expires_str = std::ctime(&expires_time);
    expires_str.pop_back(); // Remove newline
    sqlite3_bind_text(stmt, 3, expires_str.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return "";
    }

    sqlite3_finalize(stmt);
    return session_id;
}

bool Database::validateSession(const std::string& session_id) {
    std::string query = "SELECT expires_at FROM sessions WHERE session_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }

    // Get expiration time
    std::string expires_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::tm tm = {};
    std::istringstream ss(expires_str);
    ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
    auto expires_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    sqlite3_finalize(stmt);

    // Check if session is expired
    auto now = std::chrono::system_clock::now();
    return now < expires_at;
}

void Database::invalidateSession(const std::string& session_id) {
    std::string query = "DELETE FROM sessions WHERE session_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool Database::storeMessage(int sender_id, int receiver_id, const std::string& content) {
    std::string query = "INSERT INTO messages (sender_id, receiver_id, content) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<std::string> Database::getMessageHistory(int user_id, int other_user_id, int limit) {
    std::vector<std::string> messages;
    std::string query =
        "SELECT content, created_at "
        "FROM messages "
        "WHERE (sender_id = ? AND receiver_id = ?) "
        "OR (sender_id = ? AND receiver_id = ?) "
        "ORDER BY created_at DESC "
        "LIMIT ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, other_user_id);
    sqlite3_bind_int(stmt, 3, other_user_id);
    sqlite3_bind_int(stmt, 4, user_id);
    sqlite3_bind_int(stmt, 5, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        messages.push_back("[" + timestamp + "] " + content);
    }

    sqlite3_finalize(stmt);
    return messages;
}

// ... Additional implementation methods will go here ...
