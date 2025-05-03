# C++ Chat Server

A high-performance, multi-threaded chat server implementation in C++.

## Features

- Multi-threaded message processing
- Connection pooling
- Message queuing
- Chat history
- Private messaging
- Server statistics
- User listing
- Unit testing with Google Test

## Dependencies

- C++17 or higher
- CMake 3.14 or higher
- Google Test (automatically fetched during build)
- pthread library

## Build & Run

### Prerequisites

```bash
# Install required packages (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential cmake
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

## Testing

The project includes comprehensive unit tests using Google Test framework.

### Running Tests

```bash
# Method 1: Using CMake
cd build
make test

# Method 2: Using the test script
./run_tests.sh
```

## Chat Commands

- `/stats` - Show server statistics
- `/list` - List active users
- `/msg <username> <message>` - Send private message to <username>

## Technical Details

- Max connections: 200
- Message queue size: 2000
- Worker threads: 4
- Chat history size: 1000 messages
- Default port: 5555
- C++ Standard: C++17
- Build System: CMake
- Testing Framework: Google Test

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
./server    # Terminal 1
./client    # Terminal 2
```



