# High-Performance Client-Server Chat Application

A robust, multi-threaded chat server implementation in C++ that supports concurrent client connections, message broadcasting, and stress testing capabilities. This application is designed for high performance and reliability in handling multiple simultaneous connections.

## Features

### Core Functionality
- **Multi-threaded Server**: Handles multiple client connections concurrently using worker threads
- **Broadcast Messaging**: Efficient message distribution to all connected clients
- **Connection Management**: Robust handling of client connections and disconnections
- **Non-blocking I/O**: Asynchronous message handling for improved performance
- **Socket Buffer Optimization**: Configurable socket buffer sizes for better throughput

### Testing Capabilities
- **Stress Testing**: Built-in stress testing tool for performance evaluation
- **Concurrent Client Simulation**: Test with multiple simultaneous client connections
- **Configurable Test Parameters**: Adjustable client count, message frequency, and test duration
- **Performance Metrics**: Track successful connections, failed attempts, and message throughput

## Building the Application

### Prerequisites
- C++17 compiler (g++ recommended)
- POSIX-compliant operating system (Linux, macOS)
- pthread library
- Standard C++ libraries

### Compilation
```bash
# Build all targets (server and stress test)
make all

# Build specific targets
make server
make stress_test

# Clean build files
make clean
```

## Running the Application

### Server
```bash
	 ./server
```
The server will start and display:
- Number of worker threads
- Maximum connections supported
- Server IP and port
- Connection pool status

### Stress Test
```bash
./stress_test
```
The stress test will:
- Create multiple client connections
- Send messages at configured intervals
- Report connection success/failure rates
- Log any errors or timeouts

### Configuration Parameters

#### Server (`server.cpp`)
- `WORKER_THREADS`: Number of worker threads (default: 4)
- `MAX_CONNECTIONS`: Maximum concurrent connections (default: 200)
- `MESSAGE_QUEUE_SIZE`: Size of message queue (default: 2000)
- `MAX_MESSAGE_SIZE`: Maximum message size in bytes (default: 1024)

#### Stress Test (`stress_test.cpp`)
- `NUM_CLIENTS`: Number of test clients (default: 50)
- `CONCURRENT_THREADS`: Number of concurrent test threads (default: 25)
- `BATCH_SIZE`: Clients per batch (default: 10)
- `DELAY_MS`: Delay between batches in milliseconds (default: 200)

## Architecture

### Server Design
- **Connection Pool**: Pre-allocated pool of connection objects for memory efficiency
- **Worker Thread Pool**: Dedicated threads for processing client messages
- **Message Queue**: Thread-safe queue for managing broadcast messages
- **Non-blocking I/O**: Asynchronous socket operations for better performance

### Performance Optimizations
- Socket buffer size optimization
- Efficient memory management with connection pooling
- Non-blocking socket operations
- Configurable timeouts and retry mechanisms

## Utilities

### restart.sh
A utility script for server management:
```bash
./restart.sh
```
- Stops any running server instances
- Rebuilds the server
- Starts a new server instance



