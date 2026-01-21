#include "database.h"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>

const char* Database::DATABASE_PATH = "chat_server.db";
const int Database::BROADCAST_RECEIVER_ID = 0;

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
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "is_admin INTEGER DEFAULT 0"
        ");";

    if (!executeQuery(createUsersTable)) {
        return false;
    }

    // Create messages table
    const char* createMessagesTable =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender_id INTEGER NOT NULL,"
        "receiver_id INTEGER NOT NULL,"
        "content TEXT NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (sender_id) REFERENCES users(id)"
        ");";

    if (!executeQuery(createMessagesTable)) {
        return false;
    }

    // Create index for efficient message queries
    const char* createMessageIndex =
        "CREATE INDEX IF NOT EXISTS idx_messages_receiver_time "
        "ON messages(receiver_id, created_at DESC);";
    executeQuery(createMessageIndex);  // Non-critical, don't fail if index exists

    // Add is_admin column if not present (safe to run multiple times)
    const char* checkAdminColumn =
        "SELECT COUNT(*) FROM pragma_table_info('users') WHERE name='is_admin'";
    sqlite3_stmt* stmt;
    bool column_exists = false;
    if (sqlite3_prepare_v2(db, checkAdminColumn, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            column_exists = sqlite3_column_int(stmt, 0) > 0;
        }
        sqlite3_finalize(stmt);
    }

    if (!column_exists) {
        const char* addAdminColumn = "ALTER TABLE users ADD COLUMN is_admin INTEGER DEFAULT 0";
        sqlite3_exec(db, addAdminColumn, nullptr, nullptr, nullptr);
    }

    return true;
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

    // Use constant-time comparison to prevent timing attacks
    if (stored_hash.length() != computed_hash.length()) {
        return false;
    }
    return CRYPTO_memcmp(stored_hash.c_str(), computed_hash.c_str(), stored_hash.length()) == 0;
}

bool Database::removeUser(const std::string& username) {
    std::string query = "DELETE FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::isAdmin(const std::string& username) {
    std::string query = "SELECT is_admin FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool is_admin = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        is_admin = sqlite3_column_int(stmt, 0) == 1;
    }
    sqlite3_finalize(stmt);
    return is_admin;
}

int Database::getUserID(const std::string& username) {
    std::string query = "SELECT id FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    int user_id = 0;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return user_id;
}

bool Database::storeMessage(int sender_id, int receiver_id, const std::string& content) {
    if (sender_id == 0) {
        return false;  // Invalid sender
    }

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

std::vector<std::string> Database::loadRecentMessages(int limit) {
    std::vector<std::string> messages;

    // Load broadcast messages (receiver_id = 0) ordered by most recent
    std::string query =
        "SELECT m.content, m.created_at, u.username "
        "FROM messages m "
        "JOIN users u ON m.sender_id = u.id "
        "WHERE m.receiver_id = ? "
        "ORDER BY m.created_at DESC "
        "LIMIT ?";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, BROADCAST_RECEIVER_ID);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* content_ptr = sqlite3_column_text(stmt, 0);
        const unsigned char* timestamp_ptr = sqlite3_column_text(stmt, 1);
        const unsigned char* username_ptr = sqlite3_column_text(stmt, 2);

        // Skip rows with null values
        if (!content_ptr || !timestamp_ptr || !username_ptr) {
            continue;
        }

        std::string content = reinterpret_cast<const char*>(content_ptr);
        std::string timestamp = reinterpret_cast<const char*>(timestamp_ptr);
        std::string username = reinterpret_cast<const char*>(username_ptr);

        // Format: [HH:MM:SS] username: message
        std::string formatted;
        if (timestamp.length() >= 19) {
            // Extract time portion (assuming format like "2024-01-01 12:34:56")
            std::string time_str = timestamp.substr(11, 8);  // Extract HH:MM:SS
            formatted = "[" + time_str + "] " + username + ": " + content;
        } else {
            formatted = "[" + timestamp + "] " + username + ": " + content;
        }

        messages.push_back(formatted);
    }

    sqlite3_finalize(stmt);

    // Reverse to get chronological order (oldest first)
    std::reverse(messages.begin(), messages.end());

    return messages;
}
