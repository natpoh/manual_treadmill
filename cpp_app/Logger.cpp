#include "Logger.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <windows.h>

void LogMessage(const std::string& message) {
    std::ofstream logFile("debug.log", std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char timeStr[26];
        ctime_s(timeStr, sizeof(timeStr), &now);
        std::string timeStrSanitized(timeStr);
        if (!timeStrSanitized.empty() && timeStrSanitized.back() == '\n') {
            timeStrSanitized.pop_back();
        }
        logFile << "[" << timeStrSanitized << "] " << message << std::endl;
    }
    std::cout << message << std::endl;
    OutputDebugStringA((message + "\n").c_str());
}
