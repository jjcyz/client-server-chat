#include "server_metrics.h"
#include <algorithm>

// Global metrics instance
ServerMetrics metrics;

ServerMetrics::ServerMetrics() : start_time(std::chrono::steady_clock::now()) {}

void ServerMetrics::record_message(const std::string& type, double latency) {
    total_messages_processed++;
    message_types[type]++;
    if (latency > 0) {
        if (message_latencies.size() >= MAX_LATENCY_SAMPLES) {
            message_latencies.erase(message_latencies.begin());
        }
        message_latencies.push_back(latency);
    }
}

void ServerMetrics::record_bytes(size_t bytes) {
    total_bytes_transferred += bytes;
}

void ServerMetrics::update_connections(size_t count) {
    current_connections = count;
    peak_connections = std::max(peak_connections.load(), count);
}

double ServerMetrics::get_uptime_seconds() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start_time).count();
}

double ServerMetrics::get_messages_per_second() const {
    return total_messages_processed.load() / get_uptime_seconds();
}

double ServerMetrics::get_average_latency() const {
    if (message_latencies.empty()) return 0;
    double sum = 0;
    for (double latency : message_latencies) {
        sum += latency;
    }
    return sum / message_latencies.size();
}

const std::map<std::string, size_t>& ServerMetrics::get_message_types() const {
    return message_types;
}
