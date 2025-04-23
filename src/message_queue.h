#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

// Message structure for the queue
struct Message {
    int sender_socket;
    std::string content;
};

// Thread-safe message queue
class MessageQueue {
private:
    std::queue<Message> queue;
    std::mutex mtx;
    std::condition_variable cv;
    size_t max_size;
    std::atomic<size_t> current_size{0};

public:
    explicit MessageQueue(size_t size);
    bool push(Message msg);
    Message pop();
    size_t size() const;
};

#endif // MESSAGE_QUEUE_H
