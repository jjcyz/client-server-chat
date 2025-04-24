#ifndef CONSTANTS_H
#define CONSTANTS_H

// Network settings
#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_CONNECTIONS 200
#define CONNECTION_TIMEOUT 30

// Message settings
#define MAX_MESSAGE_SIZE 4096
#define MESSAGE_QUEUE_SIZE 2000
#define MAX_HISTORY_SIZE 1000

// Performance settings
#define WORKER_THREADS 4
#define MAX_RETRY_ATTEMPTS 5
#define MAX_LATENCY_SAMPLES 1000

// Socket buffer size (in bytes)
#define SOCKET_BUFFER_SIZE (256 * 1024)  // 256KB

#endif // CONSTANTS_H
