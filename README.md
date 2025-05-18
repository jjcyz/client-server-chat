# C++ Chat Server

A high-performance, multi-threaded chat server implementation in C++ with user authentication, admin controls, and testing.

## Features

- Multi-threaded message processing with worker threads
- Connection pooling with automatic cleanup of stale connections
- Message queuing with size limits
- Persistent chat history with SQLite storage
- Private messaging between users
- Secure user authentication with salted password hashing
- Admin controls for user management
- Real-time server statistics and metrics
- Active user listing and session management
- Comprehensive unit testing with Google Test
- Stress testing capabilities for performance validation
- Non-blocking socket operations
- Thread-safe operations with mutex protection
- Automatic connection timeout handling
- Message size validation and limits

## Dependencies

- C++17 or higher
- CMake 3.14 or higher
- SQLite3
- OpenSSL (for secure password hashing)
- Google Test (automatically fetched during build)
- pthread library

## Build & Run

### Prerequisites

```bash
# Install required packages (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev libssl-dev
```

### Building the Project

```bash
# Clone the repository with submodules
git clone https://github.com/jjcyz/client-server-chat.git
cd client-server-chat
git submodule init
git submodule update

# Build
mkdir -p build
cd build
cmake ..
make

# The executables will be created in build 
```

### Running the Application

```bash
# Run server (in one terminal)
./server

# Run client (in another terminal)
./client
```

The server will start listening on port 5555 by default.

## Authentication & Security

- **Registration/Login required:** Users must register or log in before chatting
- **Password Security:**
  - Passwords are salted and hashed using SHA-256 via OpenSSL
  - Salt is randomly generated for each user
  - Hashed passwords are stored in SQLite database
- **Session Management:**
  - Unique session IDs for each login
  - Session timeout handling
  - Automatic cleanup of expired sessions
- **Admin users:** Admins can remove users from the system using a special command

## Chat Commands

- `/register <username> <password>` — Register a new user
- `/login <username> <password>` — Log in as an existing user
- `/stats` — Show server statistics (messages, connections, uptime)
- `/list` — List active users and their status
- `/msg <username> <message>` — Send private message to <username>
- `/removeuser <username>` — (Admin only) Remove a user from the system

## Technical Details

### Server Configuration
- Max connections: 200
- Message queue size: 2000
- Worker threads: 4
- Chat history size: 1000 messages
- Default port: 5555
- Buffer size: 1024 bytes
- Connection timeout: 60 seconds

### Performance Features
- Connection pooling for efficient resource management
- Non-blocking socket operations
- Multi-threaded message processing
- Thread-safe operations with mutex protection
- Automatic cleanup of stale connections
- Message size validation and limits

### Database
- SQLite3 (`chat_server.db`)
- Tables:
  - Users (username, password_hash, salt, is_admin)
  - Sessions (session_id, user_id, created_at)
  - Messages (sender_id, receiver_id, content, timestamp)

### Testing
- Unit tests for server and client
- Mock server implementation for client testing
- Stress testing capabilities
- Concurrent operation testing
- Connection failure testing
- Message size limit testing

### Development Environment
- C++ Standard: C++17
- Build System: CMake
- Testing Framework: Google Test
- IDE Support: VSCode configuration included
- CI/CD: GitHub Actions workflow

## Common Development Commands

```bash
# Clean build
cd build
rm -rf *
cmake ..
make

# Build only
make

# Run tests
make test

# Run server and client
./build/server    # Terminal 1
./build/client    # Terminal 2
```

## Alternative Testing Commands

```bash
# Method 1: Using CMake
cd build
make test

# Method 2: Using the test script
./run_tests.sh
```
---

**Note:** User and admin data is persistent in `chat_server.db`. You can inspect or modify it using standard SQLite tools. The database is automatically created on first run if it doesn't exist.



