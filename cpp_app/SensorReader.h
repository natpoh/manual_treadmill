#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <windows.h>

class SensorReader {
public:
    using Callback = std::function<void(int)>;

    SensorReader(const std::string& port, int baudrate = 115200);
    ~SensorReader();

    void setCallback(Callback cb) { tick_callback = cb; }
    void setAnalogCallback(Callback cb) { analog_callback = cb; }
    bool start();
    void stop();
    bool isRunning() const { return running; }

    static std::vector<std::string> getAvailablePorts();

private:
    void readLoop();
    void trigger(int delta_ms);

    std::string port;
    int baudrate;
    std::atomic<bool> running;
    std::function<void(int)> on_trigger_callback;
    std::thread read_thread;
    HANDLE hSerial;

    Callback tick_callback;
    Callback analog_callback;
};
