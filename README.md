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

## Build & Run

```bash
# Clone the repository with submodules
git clone https://github.com/jjcyz/client-server-chat.git
git submodule init
git submodule update

# Build
mkdir build
cd build
cmake ..
make

# Run server
./server

# Run client
./client
```

The server will start listening on port 5555.

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

## Alternative Testing

```bash
cd build
make test      # Builds and runs all tests with colored output
```



