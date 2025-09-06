#pragma once
#include "../PioStateMachine.h"
#include "imgui.h"
#include <string>
#include <set>

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

    // UI rendering methods for each window
    void renderControlWindow();
    void renderVariableWindow();
    void renderProgramWindow();
    void renderRuntimeWindow();
    void renderSettingsWindow();

public:
    PioStateMachineApp(const std::string& filepath = "");
    void initialize();
    void reset();
    void renderUI();
    bool shouldExit() const { return done; }
    void setupStyle();
};