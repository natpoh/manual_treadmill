#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>

class MovementTracker;
class SensorReader;
class GamepadOutput;

class Gui {
public:
    Gui();
    ~Gui();

    bool init();
    void render();
    bool shouldClose() const;
    void cleanup();

private:
    void renderImGui();
    void updateLogic();

    bool show_window = true;
    bool is_running = false;

    // HWND and D3D11 variables
    void* hwnd;
    // We will keep D3D device setup in main.cpp to avoid exposing d3d11.h here,
    // but we'll provide a render function that main calls.

    // Config variables
    float smoothness = 0.0f;
    float speed_multiplier = 1.0f;
    float stop_timeout = 0.2f;
    int threshold_trigger = 40;
    int threshold_release = 20;
    int sensor_center = 1023;
    int sensor_mode = 0; // 0 = Raw, 1 = Pulse Counter
    int sma_period_ms = 3000;
    int sma2_period_ms = 500;
    
    // State
    float current_output_speed = 0.0f;
    float current_smoothed_speed = 0.0f;
    double start_time;
    double last_update_time = 0.0;
    double last_tick_time_sec = 0.0;
    std::vector<double> sma_history;
    std::atomic<int> last_delta_ms{0};
    std::atomic<int> last_analog_val{0};

    // Analog processing state
    bool is_magnet_near = false;
    std::chrono::steady_clock::time_point last_tick_time;
    std::vector<double> trigger_timestamps;
    std::mutex trigger_mutex;

    // Graph data
    std::vector<float> times;
    std::vector<float> speeds;
    std::vector<float> speeds_smoothed;
    std::vector<float> analog_vals;
    std::vector<int> pending_analog_vals;
    std::mutex plot_mutex;

    // Components
    MovementTracker* tracker;
    SensorReader* reader;
    GamepadOutput* gamepad;

    std::vector<std::string> available_ports;
    int selected_port_idx = 0;
};
