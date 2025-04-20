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
# Build the server
make

# Run the server
./server
```

The server will start listening on port 5555.

## Chat Commands

- `/stats` - Show server statistics
- `/list` - List active users
- `/msg <username> <message>` - Send private message

## Technical Details

- Max connections: 200
- Message queue size: 2000
- Worker threads: 4
- Chat history size: 1000 messages
- Default port: 5555

## Testing

A stress test client is included:
```bash
# Build and run stress test
g++ -std=c++17 -pthread stress_test.cpp -o stress_test
./stress_test
```



