#ifndef SERVER_METRICS_H
#define SERVER_METRICS_H

#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <vector>

#define MAX_LATENCY_SAMPLES 1000

// Performance metrics
class ServerMetrics {
private:
    std::chrono::steady_clock::time_point start_time;
    std::map<std::string, size_t> message_types;
    std::vector<double> message_latencies;

public:
    std::atomic<size_t> total_messages_processed{0};
    std::atomic<size_t> current_connections{0};
    std::atomic<size_t> peak_connections{0};
    std::atomic<size_t> total_bytes_transferred{0};
    std::atomic<size_t> messages_dropped{0};

    ServerMetrics();
    void record_message(const std::string& type, double latency = 0);
    void record_bytes(size_t bytes);
    void update_connections(size_t count);
    double get_uptime_seconds() const;
    double get_messages_per_second() const;
    double get_average_latency() const;
    const std::map<std::string, size_t>& get_message_types() const;
};

// Global metrics instance
extern ServerMetrics metrics;

#endif // SERVER_METRICS_H
