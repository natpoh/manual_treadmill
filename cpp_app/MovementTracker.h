#pragma once
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

class MovementTracker {
public:
    MovementTracker();
    ~MovementTracker();

    void setCallback(std::function<void(double)> callback);
    void registerTick(int delta_ms);
    void stop();

    std::atomic<float> smoothness{0.3f};
    std::atomic<float> speed_multiplier{1.0f};
    std::atomic<float> stop_timeout{0.2f};
    std::atomic<double> current_speed{0.0}; // raw speed 0.0 to 1.0

private:
    void monitorTimeout();

    std::function<void(double)> on_speed_change_callback;
    std::atomic<bool> running;
    
    std::atomic<double> last_tick_time{0.0};
    std::atomic<double> dynamic_timeout{1.0};
    
    std::thread monitor_thread;
    std::mutex speed_mutex;

    double getCurrentTimeSeconds();
};
