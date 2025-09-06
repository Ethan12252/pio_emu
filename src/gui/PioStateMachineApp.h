#pragma once
#include "../PioStateMachine.h"
#include "imgui.h"
#include "implot.h"
#include <string>
#include <set>
#include <vector>
#include <deque>

class PioStateMachineApp {
private:
    PioStateMachine pio;
    std::string ini_filepath;
    bool show_control_window = true;
    bool show_variable_window = true;
    bool show_program_window = true;
    bool show_runtime_window = true;
    bool show_settings_window = true;
    bool done = false;
    int tick_steps = 1;
    std::set<int> breakpoints;

    // timing diagram
    bool show_timing_window = true;
    int selected_gpio_pin = 0;
    std::vector<double> timing_timestamps;
    std::vector<double> timing_values;
    double current_time = 0.0;
    static const size_t MAX_TIMING_SAMPLES = 1000;

    // UI rendering methods for each window
    void renderControlWindow();
    void renderVariableWindow();
    void renderProgramWindow();
    void renderRuntimeWindow();
    void renderSettingsWindow();
    void renderTimingWindow();

    void updateTimingData();
public:
    PioStateMachineApp(const std::string& filepath = "");
    void initialize();
    void reset();
    void renderUI();
    bool shouldExit() const { return done; }
    void setupStyle();

};