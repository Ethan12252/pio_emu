#pragma once
#include "../PioStateMachine.h"
#include "imgui.h"
#include <string>

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

    // UI rendering methods for each window
    void renderControlWindow();
    void renderVariableWindow();
    void renderProgramWindow();
    void renderRuntimeWindow();
    void renderSettingsWindow();

public:
    PioStateMachineApp(const std::string& filepath = "") : ini_filepath(filepath) {}
    void initialize();
    void reset();
    void renderUI();
    bool shouldExit() const { return done; }
    void setupStyle();
};