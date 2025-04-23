#include "message_queue.h"

MessageQueue::MessageQueue(size_t size) : max_size(size), current_size(0) {}

bool MessageQueue::push(Message msg) {
    std::unique_lock<std::mutex> lock(mtx);
    if (current_size.load() >= max_size) {
        return false;
    }
    queue.push(std::move(msg));
    current_size++;
    cv.notify_one();
    return true;
}

Message MessageQueue::pop() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() -> bool { return !queue.empty(); });
    Message msg = std::move(queue.front());
    queue.pop();
    current_size--;
    return msg;
}

size_t MessageQueue::size() const {
    return current_size.load();
}
