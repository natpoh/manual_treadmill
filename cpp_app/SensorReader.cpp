#include "SensorReader.h"
#include <iostream>
#include <sstream>

SensorReader::SensorReader(const std::string& port, int baudrate)
    : port(port), baudrate(baudrate), running(false), hSerial(INVALID_HANDLE_VALUE) {
}

SensorReader::~SensorReader() {
    stop();
}

std::vector<std::string> SensorReader::getAvailablePorts() {
    std::vector<std::string> ports;
    char path[MAX_PATH];
    for (int i = 1; i <= 256; ++i) {
        std::string portName = "COM" + std::to_string(i);
        DWORD res = QueryDosDeviceA(portName.c_str(), path, MAX_PATH);
        if (res != 0) {
            ports.push_back(portName);
        }
    }
    return ports;
}

bool SensorReader::start() {
    if (running) return true;

    std::string fullPortName = "\\\\.\\" + port;
    hSerial = CreateFileA(fullPortName.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "[SensorReader] Error opening port " << port << std::endl;
        return false;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    // Enable DTR and RTS so Arduino boards restart and start sending data
    EscapeCommFunction(hSerial, SETDTR);
    EscapeCommFunction(hSerial, SETRTS);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    running = true;
    read_thread = std::thread(&SensorReader::readLoop, this);
    return true;
}

void SensorReader::stop() {
    running = false;
    if (read_thread.joinable()) {
        read_thread.join();
    }
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
}

void SensorReader::trigger(int delta_ms) {
    if (on_trigger_callback) {
        on_trigger_callback(delta_ms);
    }
}

void SensorReader::readLoop() {
    char buffer[256];
    DWORD bytesRead;
    std::string line_buffer = "";

    while (running && hSerial != INVALID_HANDLE_VALUE) {
        if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                line_buffer += buffer;

                size_t pos;
                while ((pos = line_buffer.find('\n')) != std::string::npos) {
                    std::string line = line_buffer.substr(0, pos);
                    line_buffer.erase(0, pos + 1);

                    // Remove \r if present
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    if (line.rfind("TICK:", 0) == 0) {
                        try {
                            int delta_ms = std::stoi(line.substr(5));
                            if (tick_callback) {
                                tick_callback(delta_ms);
                            }
                        } catch (...) {}
                    } else if (line.rfind("ANALOG:", 0) == 0) {
                        try {
                            int val = std::stoi(line.substr(7));
                            if (analog_callback) {
                                analog_callback(val);
                            }
                        } catch (...) {}
                    } else {
                        try {
                            int val = std::stoi(line);
                            if (analog_callback) {
                                analog_callback(val);
                            }
                        } catch (...) {}
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
