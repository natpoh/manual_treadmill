#include "MovementTracker.h"
#include <chrono>
#include <algorithm>

double MovementTracker::getCurrentTimeSeconds() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

MovementTracker::MovementTracker()
    : running(true) {
    monitor_thread = std::thread(&MovementTracker::monitorTimeout, this);
}

MovementTracker::~MovementTracker() {
    stop();
}

void MovementTracker::setCallback(std::function<void(double)> callback) {
    std::lock_guard<std::mutex> lock(speed_mutex);
    on_speed_change_callback = callback;
}

void MovementTracker::registerTick(int delta_ms) {
    double now = getCurrentTimeSeconds();
    
    double delta_sec = delta_ms / 1000.0;
    if (delta_sec > 0.0) {
        // Швидкість = (1 кроків / секунда) * базова константа * множник користувача
        double raw_speed = (1.0 / delta_sec) * 0.5 * speed_multiplier.load();
        
        double alpha = smoothness.load();
        // Згладжування: нове значення = (старе * інерція) + (нове * свіжа реакція)
        
        std::lock_guard<std::mutex> lock(speed_mutex);
        double current = current_speed.load();
        double new_speed = alpha * current + (1.0 - alpha) * raw_speed;
        new_speed = std::min(1.0, std::max(0.0, new_speed));
        current_speed.store(new_speed);
        
        if (on_speed_change_callback) {
            on_speed_change_callback(new_speed);
        }
        
        // Таймаут зупинки беремо з налаштувань
        dynamic_timeout.store(stop_timeout.load());
    }
    
    last_tick_time.store(now);
}

void MovementTracker::monitorTimeout() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        double last = last_tick_time.load();
        if (last == 0.0) continue;
        
        double now = getCurrentTimeSeconds();
        double timeout = dynamic_timeout.load();
        
        if (now - last > timeout) {
            std::lock_guard<std::mutex> lock(speed_mutex);
            double current = current_speed.load();
            if (current > 0.0) {
                // Якщо растояние більше 0.2 сек, це зупинка
                current_speed.store(0.0);
                
                if (on_speed_change_callback) {
                    on_speed_change_callback(0.0);
                }
            }
        }
    }
}

void MovementTracker::stop() {
    running = false;
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
}
