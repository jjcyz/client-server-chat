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

## Testing

```bash
# Build and run tests (from build directory)
make check
```

## Technical Details

- Max connections: 200
- Message queue size: 2000
- Worker threads: 4
- Chat history size: 1000 messages
- Default port: 5555



