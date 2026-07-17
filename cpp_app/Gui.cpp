#include "Gui.h"
#include "imgui.h"
#include "implot.h"
#include "SensorReader.h"
#include "MovementTracker.h"
#include "GamepadOutput.h"
#include <chrono>
#include <algorithm>

Gui::Gui() : tracker(nullptr), reader(nullptr), gamepad(nullptr) {
    tracker = new MovementTracker();
    gamepad = new GamepadOutput();
    gamepad->init();
    
    available_ports = SensorReader::getAvailablePorts();
    if (available_ports.empty()) {
        available_ports.push_back("No ports");
    }

    auto now = std::chrono::steady_clock::now();
    start_time = std::chrono::duration<double>(now.time_since_epoch()).count();
}

Gui::~Gui() {
    if (reader) {
        reader->stop();
        delete reader;
    }
    if (tracker) {
        tracker->stop();
        delete tracker;
    }
    if (gamepad) {
        gamepad->stopMoving();
        delete gamepad;
    }
}

void Gui::updateLogic() {
    if (!is_running) return;

    tracker->smoothness.store(smoothness);
    tracker->speed_multiplier.store(speed_multiplier);
    tracker->stop_timeout.store(stop_timeout);

    // Pop all pending values
    std::vector<int> current_pending;
    {
        std::lock_guard<std::mutex> lock(plot_mutex);
        current_pending = std::move(pending_analog_vals);
        pending_analog_vals.clear();
    }

    auto now = std::chrono::steady_clock::now();
    double current_time = std::chrono::duration<double>(now.time_since_epoch()).count() - start_time;

    // Run 100Hz simulation updates
    double time_step = 0.010; // 10ms (100Hz)
    while (last_update_time + time_step <= current_time) {
        last_update_time += time_step;
        
        // Calculate SMA: sum of ticks in the sliding window of size sma_period_ms
        double window_start = last_update_time - (sma_period_ms / 1000.0);
        int active_ticks = 0;
        
        {
            std::lock_guard<std::mutex> lock(trigger_mutex);
            // Clean up extremely old timestamps
            trigger_timestamps.erase(
                std::remove_if(trigger_timestamps.begin(), trigger_timestamps.end(),
                    [window_start, this](double t) { return (t - this->start_time) < window_start - 1.0; }),
                trigger_timestamps.end()
            );
            
            // Count ticks in the current sliding window
            for (double t : trigger_timestamps) {
                double t_rel = t - start_time;
                if (t_rel >= window_start && t_rel <= last_update_time) {
                    active_ticks++;
                }
            }
        }
        
        double raw_speed = static_cast<double>(active_ticks);
        
        // Store raw speed to the SMA 2 history buffer to compute the second SMA
        sma_history.push_back(raw_speed);
        
        int required_size = sma2_period_ms / 10;
        if (required_size < 1) required_size = 1;
        
        while (sma_history.size() > required_size) {
            sma_history.erase(sma_history.begin());
        }
        while (sma_history.size() < required_size) {
            sma_history.push_back(raw_speed); // fill if too small
        }
        
        // Calculate SMA 2 average
        double sum = 0.0;
        for (double v : sma_history) {
            sum += v;
        }
        double smoothed_speed = sum / sma_history.size();
        
        // Gamepad speed is calculated from the smoothed speed scaled by max_speed_limit
        double final_speed = smoothed_speed / (max_speed_limit > 0 ? max_speed_limit : 1.0);
        if (final_speed > 1.0) final_speed = 1.0;
        if (final_speed < 0.0) final_speed = 0.0;
        
        tracker->current_speed.store(final_speed);
        current_output_speed = static_cast<float>(raw_speed); // Store raw count for plotting
        current_smoothed_speed = static_cast<float>(smoothed_speed); // Store smoothed count for plotting
        gamepad->setSpeed(static_cast<float>(final_speed));
    }

    // Push pending values to the graph
    if (!current_pending.empty()) {
        int count = current_pending.size();
        double frame_start_time = current_time - 0.016; // approximate 60Hz frame time
        
        for (int i = 0; i < count; i++) {
            double t = frame_start_time + (current_time - frame_start_time) * (i + 1) / count;
            times.push_back(static_cast<float>(t));
            speeds.push_back(current_output_speed);
            speeds_smoothed.push_back(current_smoothed_speed);
            
            float val = static_cast<float>(current_pending[i]);
            analog_vals.push_back(val);
        }
    } else {
        times.push_back(static_cast<float>(current_time));
        speeds.push_back(current_output_speed);
        speeds_smoothed.push_back(current_smoothed_speed);
        
        float val = 0.0f;
        if (sensor_mode == 0) {
            val = static_cast<float>(last_analog_val.load());
        } else {
            val = static_cast<float>(last_analog_val.exchange(0));
        }
        analog_vals.push_back(val);
    }

    int max_points = 300;
    while (times.size() > max_points) {
        times.erase(times.begin());
        speeds.erase(speeds.begin());
        speeds_smoothed.erase(speeds_smoothed.begin());
        analog_vals.erase(analog_vals.begin());
    }
}

void Gui::render() {
    updateLogic();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    
    ImGui::Begin("VRTreadmill", nullptr, window_flags);

    // Left Panel
    ImGui::BeginChild("LeftPanel", ImVec2(400, 0), true);
    
    ImGui::TextDisabled("VR Treadmill Controller");
    ImGui::Separator();
    ImGui::Spacing();

    // Port selection
    ImGui::Text("COM Port:");
    if (ImGui::BeginCombo("##PortCombo", available_ports[selected_port_idx].c_str())) {
        for (int i = 0; i < available_ports.size(); i++) {
            bool is_selected = (selected_port_idx == i);
            if (ImGui::Selectable(available_ports[i].c_str(), is_selected)) {
                selected_port_idx = i;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    if (ImGui::Button("Refresh Ports")) {
        available_ports = SensorReader::getAvailablePorts();
        if (available_ports.empty()) available_ports.push_back("No ports");
        selected_port_idx = 0;
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    // Show input data
    ImGui::Text("Sensor Signal:");
    int current_last_delta_ms = last_delta_ms.load();
    if (current_last_delta_ms > 0) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Last Step: %d ms", current_last_delta_ms);
    } else {
        ImGui::TextDisabled("Waiting for step...");
    }

    ImGui::Text("Analog Value (pin A0):");
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%d / 1023", last_analog_val.load());

    // Sliders
    ImGui::SliderInt("Trigger Threshold", &threshold_trigger, 5, 1000);
    ImGui::SliderInt("SMA 1 Period (ms)", &sma_period_ms, 10, 3000);
    ImGui::SliderInt("SMA 2 Period (ms)", &sma2_period_ms, 10, 3000);
    ImGui::SliderInt("Maximum Speed", &max_speed_limit, 1, 100);

    // Compute release threshold dynamically with hysteresis
    threshold_release = threshold_trigger - 10;
    if (threshold_release < 2) {
        threshold_release = 2;
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (!is_running) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("START", ImVec2(-1, 50))) {
            if (available_ports[selected_port_idx] != "No ports") {
                reader = new SensorReader(available_ports[selected_port_idx]);
                reader->setCallback([this](int delta_ms) {
                    double now_sec = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
                    this->last_tick_time_sec = now_sec;
                    {
                        std::lock_guard<std::mutex> lock(this->trigger_mutex);
                        this->trigger_timestamps.push_back(now_sec);
                    }
                    this->last_delta_ms.store(delta_ms);
                });
                reader->setAnalogCallback([this](int val) {
                    if (this->sensor_mode == 0) {
                        this->last_analog_val.store(val);

                        int center = this->sensor_center;
                        int diff = std::abs(val - center);
                        auto now = std::chrono::steady_clock::now();

                        if (!this->is_magnet_near && diff > this->threshold_trigger) {
                            this->is_magnet_near = true;
                            
                            double now_sec = std::chrono::duration<double>(now.time_since_epoch()).count();
                            this->last_tick_time_sec = now_sec;
                            {
                                std::lock_guard<std::mutex> lock(this->trigger_mutex);
                                this->trigger_timestamps.push_back(now_sec);
                            }
                            
                            int delta_ms = 2000; // default for first tick
                            if (this->last_tick_time.time_since_epoch().count() > 0) {
                                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_tick_time).count();
                                delta_ms = static_cast<int>(elapsed);
                            }
                            
                            this->last_delta_ms.store(delta_ms);
                            this->last_tick_time = now;
                        }
                        else if (this->is_magnet_near && diff < this->threshold_release) {
                            this->is_magnet_near = false;
                        }
                    } else {
                        // Pulse Counter mode
                        this->last_analog_val.fetch_add(val);

                        if (val > 0) {
                            auto now = std::chrono::steady_clock::now();
                            double now_sec = std::chrono::duration<double>(now.time_since_epoch()).count();
                            this->last_tick_time_sec = now_sec;
                            {
                                std::lock_guard<std::mutex> lock(this->trigger_mutex);
                                for (int i = 0; i < val; i++) {
                                    this->trigger_timestamps.push_back(now_sec);
                                }
                            }
                            
                            int delta_ms = 2000;
                            if (this->last_tick_time.time_since_epoch().count() > 0) {
                                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_tick_time).count();
                                delta_ms = static_cast<int>(elapsed);
                            }
                            
                            this->last_delta_ms.store(delta_ms);
                            this->last_tick_time = now;
                        }
                    }

                    // Push to the thread-safe queue for plotting
                    {
                        std::lock_guard<std::mutex> lock(this->plot_mutex);
                        this->pending_analog_vals.push_back(val);
                    }
                });
                if (reader->start()) {
                    is_running = true;
                    // Reset graph
                    times.clear();
                    speeds.clear();
                    speeds_smoothed.clear();
                    analog_vals.clear();
                    {
                        std::lock_guard<std::mutex> lock(plot_mutex);
                        pending_analog_vals.clear();
                    }
                    {
                        std::lock_guard<std::mutex> lock(trigger_mutex);
                        trigger_timestamps.clear();
                    }
                    sma_history.clear();
                    last_delta_ms.store(0);
                    is_magnet_near = false;
                    last_tick_time = std::chrono::steady_clock::time_point();
                    double now_sec = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
                    start_time = now_sec;
                    last_update_time = 0.0;
                    last_tick_time_sec = now_sec - 10.0; // initialize far in past
                }
            }
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP", ImVec2(-1, 50))) {
            is_running = false;
            reader->stop();
            delete reader;
            reader = nullptr;
            gamepad->stopMoving();
            current_output_speed = 0.0f;
            current_smoothed_speed = 0.0f;
        }
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // Right Panel - Graphs
    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
    
    float available_height = ImGui::GetContentRegionAvail().y;
    float space_y = ImGui::GetStyle().ItemSpacing.y;
    float height_analog = (available_height - space_y) * 0.33f;
    float height_speed = (available_height - space_y) * 0.67f;

    // Top Plot: Sensor Analog Signal
    if (ImPlot::BeginPlot("Sensor Analog Signal", ImVec2(-1, height_analog))) {
        ImPlot::SetupAxes("Time (s)", "Raw Analog Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -5, 1050, ImPlotCond_Always);
        
        if (!times.empty()) {
            ImPlot::PlotLine("Analog Signal", times.data(), analog_vals.data(), times.size(), {ImPlotProp_LineColor, ImVec4(1.0f, 0.8f, 0.2f, 1.0f), ImPlotProp_LineWeight, 1.5f});
            
            // Draw Trigger Threshold
            double low_x[2] = {times.front(), times.back()};
            double low_y[2] = {static_cast<double>(sensor_center - threshold_trigger), static_cast<double>(sensor_center - threshold_trigger)};
            std::string low_label = "Trigger Threshold (" + std::to_string(sensor_center - threshold_trigger) + ")";
            ImPlot::PlotLine(low_label.c_str(), low_x, low_y, 2, {ImPlotProp_LineColor, ImVec4(1.0f, 0.2f, 0.2f, 1.0f), ImPlotProp_LineWeight, 2.0f});
        }
        ImPlot::EndPlot();
    }

    // Bottom Plot: Calculated Speed (SMA)
    if (ImPlot::BeginPlot("Calculated Speed (SMA)", ImVec2(-1, height_speed))) {
        ImPlot::SetupAxes("Time (s)", "Pulses", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LockMax);
        
        double max_y_limit = static_cast<double>(max_speed_limit) * 1.30;
        ImPlot::SetupAxisLimits(ImAxis_Y1, -max_y_limit * 0.05, max_y_limit, ImPlotCond_Always);
        
        if (!times.empty()) {
            ImPlot::PlotLine("Pulses (SMA 1)", times.data(), speeds.data(), times.size(), {ImPlotProp_LineColor, ImVec4(0.2f, 0.6f, 1.0f, 1.0f), ImPlotProp_LineWeight, 2.0f});
            ImPlot::PlotLine("Smoothed (SMA 2)", times.data(), speeds_smoothed.data(), times.size(), {ImPlotProp_LineColor, ImVec4(0.2f, 1.0f, 0.6f, 1.0f), ImPlotProp_LineWeight, 2.0f});
            
            // Draw Horizontal Max Speed Target Line
            double max_x[2] = {times.front(), times.back()};
            double max_y[2] = {static_cast<double>(max_speed_limit), static_cast<double>(max_speed_limit)};
            std::string max_label = "Max Speed Line (" + std::to_string(max_speed_limit) + ")";
            ImPlot::PlotLine(max_label.c_str(), max_x, max_y, 2, {ImPlotProp_LineColor, ImVec4(1.0f, 0.2f, 0.2f, 1.0f), ImPlotProp_LineWeight, 2.0f});
        }
        ImPlot::EndPlot();
    }
    
    ImGui::EndChild();

    ImGui::End();
}
