# C++ Chat Server

A high-performance, multi-threaded chat server implementation in C++ with user authentication and admin controls.

## Features

- Multi-threaded message processing
- Connection pooling
- Message queuing
- Chat history
- Private messaging
- User authentication (register/login)
- Admin controls (user removal)
- Server statistics
- User listing
- Unit testing with Google Test

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

# The executables will be created in the build directory
```

### Running the Application

```bash
# Run server (in one terminal)
./server

# Run client (in another terminal)
./client
```

The server will start listening on port 5555 by default.

## Authentication & Admin

- **Registration/Login required:** Users must register or log in before chatting.
- **Passwords:** Passwords are salted and hashed using SHA-256 via OpenSSL before being stored in the SQLite database (`chat_server.db`).
- **Admin users:** Admins can remove users from the system using a special command. Admin status is stored in the database (`is_admin` column).

## Chat Commands

- `/register <username> <password>` — Register a new user
- `/login <username> <password>` — Log in as an existing user
- `/stats` — Show server statistics
- `/list` — List active users
- `/msg <username> <message>` — Send private message to <username>
- `/removeuser <username>` — (Admin only) Remove a user from the system

## Technical Details

- Max connections: 200
- Message queue size: 2000
- Worker threads: 4
- Chat history size: 1000 messages
- Default port: 5555
- C++ Standard: C++17
- Build System: CMake or Makefile
- Testing Framework: Google Test
- Database: SQLite3 (`chat_server.db`)
- Passwords: Salted and hashed with OpenSSL SHA-256

### Common Development Commands

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

**Note:** User and admin data is persistent in `chat_server.db`. You can inspect or modify it using standard SQLite tools.



