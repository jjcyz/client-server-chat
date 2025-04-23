#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "server.h"  // for Message struct

class MessageQueue {
private:
    std::queue<Message> queue;
    std::mutex mtx;
    std::condition_variable cv;
    const size_t max_size;
    std::atomic<size_t> current_size;

public:
    explicit MessageQueue(size_t size);
    bool push(Message msg);
    Message pop();
    size_t size() const;

    // Delete copy constructor and assignment operator
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
};

#endif // MESSAGE_QUEUE_H
