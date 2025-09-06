#include "PioStateMachineApp.h"

PioStateMachineApp::PioStateMachineApp(const std::string& filepath /*= ""*/)
    : ini_filepath(filepath), pio(PioStateMachine(filepath))
{
}

void PioStateMachineApp::initialize() {

    // Reset GUI state
    tick_steps = 1;
    show_control_window = true;
    show_variable_window = true;
    show_program_window = true;
    show_runtime_window = true;
    show_settings_window = true;
    done = false;
    breakpoints.clear();

    timing_timestamps.clear();
    timing_values.clear();
    timing_values.resize(32);
    current_cycle = 0;  // Reset to 0
    for (int i = 0; i < 32; i++) {
        selected_pins[i] = false;
    }
}

void PioStateMachineApp::reset() {
    initialize();
}

void PioStateMachineApp::setupStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.98f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.45f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.5f, 0.8f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.6f, 0.9f, 0.50f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.7f, 1.0f, 0.50f);
}

void PioStateMachineApp::renderControlWindow() {
    if (!ImGui::Begin("Control Panel", &show_control_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Emulator Controls");
    ImGui::Separator();

    if (ImGui::Button("Tick Once")) {
        pio.tick();
        updateTimingData();
    }

    ImGui::SameLine();
    ImGui::InputInt("Steps", &tick_steps, 1, 100);
    if (tick_steps < 1) tick_steps = 1;
    ImGui::SameLine();
    if (ImGui::Button("Tick Multiple")) {
        int steps_done = 0;
        while (steps_done < tick_steps) {
            if (breakpoints.count(static_cast<int>(pio.regs.pc))) {
                break;
            }
            pio.tick();
            updateTimingData();
            steps_done++;
        }
    }

    if (ImGui::Button("Reset Emulator")) {
        pio.reset(ini_filepath); // reset state machine 
        reset();                 // reset gui state
        // reset timing diagram
        timing_timestamps.clear();
        timing_values.clear();
        current_cycle = 0.0;
    }

    ImGui::Separator();
    ImGui::Text("Run Until Condition");
    static char var_name[32] = "pc";
    static uint32_t target_value = 0;
    static int max_cycles = 10000;
    ImGui::InputText("Variable Name", var_name, IM_ARRAYSIZE(var_name));
    ImGui::InputScalar("Target Value", ImGuiDataType_U32, &target_value, nullptr, nullptr, "%u");
    ImGui::InputInt("Max Cycles", &max_cycles);
    if (max_cycles < 1) max_cycles = 1;
    if (ImGui::Button("Run Until")) {
        pio.run_until_var(var_name, target_value, max_cycles);
    }

    ImGui::Separator();
    ImGui::Text("Breakpoints");
    static int max_cycles_bp = 10000;
    ImGui::InputInt("Max Cycles BP", &max_cycles_bp);
    if (max_cycles_bp < 1) max_cycles_bp = 1;
    if (ImGui::Button("Continue to Breakpoint")) {
        int cycles = 0;
        while (cycles < max_cycles_bp) {
            if (breakpoints.count(static_cast<int>(pio.regs.pc))) {
                break;
            }
            pio.tick();
            updateTimingData();
            cycles++;
        }
    }

    ImGui::End();
}

void PioStateMachineApp::renderVariableWindow() {
    if (!ImGui::Begin("Variables", &show_variable_window)) {
        ImGui::End();
        return;
    }

    ImGui::Text("State Machine Variables");
    ImGui::Separator();

    if (ImGui::BeginTabBar("VariableTabs")) {
        if (ImGui::BeginTabItem("Registers")) {
            if (ImGui::BeginTable("RegistersTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("X");
                ImGui::TableSetColumnIndex(1);
                uint32_t x_val = pio.regs.x;
                if (ImGui::InputScalar("##x", ImGuiDataType_U32, &x_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.regs.x = x_val;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Y");
                ImGui::TableSetColumnIndex(1);
                uint32_t y_val = pio.regs.y;
                if (ImGui::InputScalar("##y", ImGuiDataType_U32, &y_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.regs.y = y_val;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("ISR");
                ImGui::TableSetColumnIndex(1);
                uint32_t isr_val = pio.regs.isr;
                if (ImGui::InputScalar("##isr", ImGuiDataType_U32, &isr_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.regs.isr = isr_val;
                }
                ImGui::SameLine();
                ImGui::Text("(Shift Count:");
                ImGui::SameLine();
                uint32_t isr_count = pio.regs.isr_shift_count;
                if (ImGui::InputScalar("##isrcount", ImGuiDataType_U32, &isr_count)) {
                    if (isr_count <= 32) pio.regs.isr_shift_count = isr_count;
                }
                ImGui::SameLine();
                ImGui::Text(")");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OSR");
                ImGui::TableSetColumnIndex(1);
                uint32_t osr_val = pio.regs.osr;
                if (ImGui::InputScalar("##osr", ImGuiDataType_U32, &osr_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.regs.osr = osr_val;
                }
                ImGui::SameLine();
                ImGui::Text("(Shift Count:");
                ImGui::SameLine();
                uint32_t osr_count = pio.regs.osr_shift_count;
                if (ImGui::InputScalar("##osrcount", ImGuiDataType_U32, &osr_count)) {
                    if (osr_count <= 32) pio.regs.osr_shift_count = osr_count;
                }
                ImGui::SameLine();
                ImGui::Text(")");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("PC");
                ImGui::TableSetColumnIndex(1);
                uint32_t pc_val = pio.regs.pc;
                if (ImGui::InputScalar("##pc", ImGuiDataType_U32, &pc_val)) {
                    if (pc_val < 32)
                        pio.regs.pc = pc_val;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Delay");
                ImGui::TableSetColumnIndex(1);
                uint32_t delay_val = pio.regs.delay;
                if (ImGui::InputScalar("##delay", ImGuiDataType_U32, &delay_val)) {
                    pio.regs.delay = delay_val;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Status");
                ImGui::TableSetColumnIndex(1);
                uint32_t status_val = pio.regs.status;
                if (ImGui::InputScalar("##status", ImGuiDataType_U32, &status_val)) {
                    pio.regs.status = status_val;
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("GPIO")) {
            if (ImGui::BeginTable("GPIOTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Pin", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Raw Data");
                ImGui::TableSetupColumn("Pindir (0=out,1=in)");
                ImGui::TableHeadersRow();

                for (int pin = 0; pin < 32; ++pin) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", pin);
                    ImGui::TableSetColumnIndex(1);
                    int raw_data = pio.gpio.raw_data[pin];
                    ImGui::PushID(pin * 2);
                    if (ImGui::InputInt("##raw_data", &raw_data, 0)) {
                        pio.gpio.raw_data[pin] = static_cast<int8_t>(raw_data & 1);
                    }
                    ImGui::PopID();
                    ImGui::TableSetColumnIndex(2);
                    int pindir = pio.gpio.pindirs[pin];
                    ImGui::PushID(pin * 2 + 1);
                    if (ImGui::InputInt("##pindir", &pindir, 0)) {
                        pio.gpio.pindirs[pin] = static_cast<int8_t>(pindir & 1);
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("FIFOs")) {
            if (ImGui::BeginTable("FIFOsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("TX FIFO Count");
                ImGui::TableSetColumnIndex(1);
                int tx_count = pio.fifo.tx_fifo_count;
                if (ImGui::InputInt("##txcount", &tx_count)) {
                    if (tx_count >= 0 && tx_count <= 8) pio.fifo.tx_fifo_count = static_cast<uint8_t>(tx_count);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("RX FIFO Count");
                ImGui::TableSetColumnIndex(1);
                int rx_count = pio.fifo.rx_fifo_count;
                if (ImGui::InputInt("##rxcount", &rx_count)) {
                    if (rx_count >= 0 && rx_count <= 8) pio.fifo.rx_fifo_count = static_cast<uint8_t>(rx_count);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Push is Stalling");
                ImGui::TableSetColumnIndex(1);
                bool push_stall = pio.fifo.push_is_stalling;
                if (ImGui::Checkbox("##push_stall", &push_stall)) {
                    pio.fifo.push_is_stalling = push_stall;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Pull is Stalling");
                ImGui::TableSetColumnIndex(1);
                bool pull_stall = pio.fifo.pull_is_stalling;
                if (ImGui::Checkbox("##pull_stall", &pull_stall)) {
                    pio.fifo.pull_is_stalling = pull_stall;
                }

                for (int i = 0; i < 4; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("TX FIFO[%d]", i);
                    ImGui::TableSetColumnIndex(1);
                    uint32_t tx_val = pio.fifo.tx_fifo[i];
                    ImGui::PushID(i);
                    if (ImGui::InputScalar("##tx", ImGuiDataType_U32, &tx_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                        pio.fifo.tx_fifo[i] = tx_val;
                    }
                    ImGui::PopID();
                }

                for (int i = 0; i < 4; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("RX FIFO[%d]", i);
                    ImGui::TableSetColumnIndex(1);
                    uint32_t rx_val = pio.fifo.rx_fifo[i];
                    ImGui::PushID(i + 4);
                    if (ImGui::InputScalar("##rx", ImGuiDataType_U32, &rx_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                        pio.fifo.rx_fifo[i] = rx_val;
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("IRQs")) {
            if (ImGui::BeginTable("IRQTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("IRQ Waiting");
                ImGui::TableSetColumnIndex(1);
                bool irq_w = pio.irq_is_waiting;
                if (ImGui::Checkbox("##irqwait", &irq_w)) {
                    pio.irq_is_waiting = irq_w;
                }

                for (int i = 0; i < 8; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("IRQ Flag %d", i);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(i);
                    bool irq_f = pio.irq_flags[i];
                    if (ImGui::Checkbox("##irq", &irq_f)) {
                        pio.irq_flags[i] = irq_f;
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void PioStateMachineApp::renderSettingsWindow() {
    if (!ImGui::Begin("Settings", &show_settings_window)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Configuration Settings");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Settings are editable only when clock is 0");
    ImGui::Separator();

    if (ImGui::BeginTable("SettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, ImVec2(0.0f, ImGui::GetTextLineHeight() * 20))) {
        ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset Count");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int sideset_count = pio.settings.sideset_count;
            if (ImGui::InputInt("##sideset_count", &sideset_count)) {
                pio.settings.sideset_count = sideset_count;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.sideset_count);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset Opt");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool sideset_opt = pio.settings.sideset_opt;
            if (ImGui::Checkbox("##sideset_opt", &sideset_opt)) {
                pio.settings.sideset_opt = sideset_opt;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.sideset_opt ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset to Pindirs");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool sideset_to_pindirs = pio.settings.sideset_to_pindirs;
            if (ImGui::Checkbox("##sideset_to_pindirs", &sideset_to_pindirs)) {
                pio.settings.sideset_to_pindirs = sideset_to_pindirs;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.sideset_to_pindirs ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset Base");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int sideset_base = pio.settings.sideset_base;
            if (ImGui::InputInt("##sideset_base", &sideset_base)) {
                pio.settings.sideset_base = sideset_base;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.sideset_base);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("In Base");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int in_base = pio.settings.in_base;
            if (ImGui::InputInt("##in_base", &in_base)) {
                pio.settings.in_base = in_base;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.in_base);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Base");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int out_base = pio.settings.out_base;
            if (ImGui::InputInt("##out_base", &out_base)) {
                pio.settings.out_base = out_base;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.out_base);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Set Base");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int set_base = pio.settings.set_base;
            if (ImGui::InputInt("##set_base", &set_base)) {
                pio.settings.set_base = set_base;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.set_base);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Jmp Pin");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int jmp_pin = pio.settings.jmp_pin;
            if (ImGui::InputInt("##jmp_pin", &jmp_pin)) {
                pio.settings.jmp_pin = jmp_pin;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.jmp_pin);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Set Count");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int set_count = pio.settings.set_count;
            if (ImGui::InputInt("##set_count", &set_count)) {
                pio.settings.set_count = set_count;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.set_count);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Count");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int out_count = pio.settings.out_count;
            if (ImGui::InputInt("##out_count", &out_count)) {
                pio.settings.out_count = out_count;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.out_count);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Push Threshold");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            uint32_t push_threshold = pio.settings.push_threshold;
            if (ImGui::InputScalar("##push_threshold", ImGuiDataType_U32, &push_threshold)) {
                pio.settings.push_threshold = push_threshold;
            }
        }
        else {
            ImGui::Text("%u", pio.settings.push_threshold);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Pull Threshold");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            uint32_t pull_threshold = pio.settings.pull_threshold;
            if (ImGui::InputScalar("##pull_threshold", ImGuiDataType_U32, &pull_threshold)) {
                pio.settings.pull_threshold = pull_threshold;
            }
        }
        else {
            ImGui::Text("%u", pio.settings.pull_threshold);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("FIFO Level N");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            int fifo_level_N = pio.settings.fifo_level_N;
            if (ImGui::InputInt("##fifo_level_N", &fifo_level_N)) {
                pio.settings.fifo_level_N = fifo_level_N;
            }
        }
        else {
            ImGui::Text("%d", pio.settings.fifo_level_N);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Wrap Start");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            uint32_t wrap_start = pio.settings.wrap_start;
            if (ImGui::InputScalar("##wrap_start", ImGuiDataType_U32, &wrap_start)) {
                pio.settings.wrap_start = wrap_start;
            }
        }
        else {
            ImGui::Text("%u", pio.settings.wrap_start);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Wrap End");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            uint32_t wrap_end = pio.settings.wrap_end;
            if (ImGui::InputScalar("##wrap_end", ImGuiDataType_U32, &wrap_end)) {
                pio.settings.wrap_end = wrap_end;
            }
        }
        else {
            ImGui::Text("%u", pio.settings.wrap_end);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("In Shift Right");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool in_shift_right = pio.settings.in_shift_right;
            if (ImGui::Checkbox("##in_shift_right", &in_shift_right)) {
                pio.settings.in_shift_right = in_shift_right;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.in_shift_right ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Shift Right");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool out_shift_right = pio.settings.out_shift_right;
            if (ImGui::Checkbox("##out_shift_right", &out_shift_right)) {
                pio.settings.out_shift_right = out_shift_right;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.out_shift_right ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("In Shift Autopush");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool in_shift_autopush = pio.settings.in_shift_autopush;
            if (ImGui::Checkbox("##in_shift_autopush", &in_shift_autopush)) {
                pio.settings.in_shift_autopush = in_shift_autopush;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.in_shift_autopush ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Shift Autopull");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool out_shift_autopull = pio.settings.out_shift_autopull;
            if (ImGui::Checkbox("##out_shift_autopull", &out_shift_autopull)) {
                pio.settings.out_shift_autopull = out_shift_autopull;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.out_shift_autopull ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Autopull Enable");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool autopull_enable = pio.settings.autopull_enable;
            if (ImGui::Checkbox("##autopull_enable", &autopull_enable)) {
                pio.settings.autopull_enable = autopull_enable;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.autopull_enable ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Autopush Enable");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool autopush_enable = pio.settings.autopush_enable;
            if (ImGui::InputScalar("##autopush_enable", ImGuiDataType_U32, &autopush_enable)) {
                pio.settings.autopush_enable = autopush_enable;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.autopush_enable ? "true" : "false");
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Status Sel");
        ImGui::TableSetColumnIndex(1);
        if (pio.clock == 0) {
            bool status_sel = pio.settings.status_sel;
            if (ImGui::Checkbox("##status_sel", &status_sel)) {
                pio.settings.status_sel = status_sel;
            }
        }
        else {
            ImGui::Text("%s", pio.settings.status_sel ? "true" : "false");
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void PioStateMachineApp::renderProgramWindow() {
    if (!ImGui::Begin("Program", &show_program_window)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Instruction Memory");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Instructions are editable only when clock is 0");
    ImGui::Separator();

    if (ImGui::BeginTable("InstructionTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Instruction");
        ImGui::TableSetupColumn("Text");
        ImGui::TableSetupColumn("BP", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Current");
        ImGui::TableHeadersRow();

        // Estimate the current instruction's address (typically pc - 1, unless jumped)
        int current_addr = -1;
        if (pio.regs.pc > 0 && pio.instructionMemory[pio.regs.pc - 1] == pio.currentInstruction && !pio.exec_command) {
            current_addr = pio.regs.pc - 1;
        }
        else {
            // Fallback: search for the current instruction in memory
            for (int i = 0; i < 32; ++i) {
                if (pio.instructionMemory[i] == pio.currentInstruction) {
                    current_addr = i;
                    break;
                }
            }
        }

        for (int i = 0; i < 32; ++i) {
            ImGui::TableNextRow();
            if (i == current_addr) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.6f, 0.3f, 0.65f)));
            }
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            uint16_t instr = pio.instructionMemory[i];
            if (pio.clock == 0) {
                if (ImGui::InputScalar("##instr", ImGuiDataType_U16, &instr, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.instructionMemory[i] = instr;
                }
            }
            else {
                ImGui::Text("0x%04X", instr);
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", pio.instruction_text[i].c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::PushID(i + 32);
            bool is_bp = breakpoints.count(i);
            if (ImGui::Checkbox("##bp", &is_bp)) {
                if (is_bp) {
                    breakpoints.insert(i);
                }
                else {
                    breakpoints.erase(i);
                }
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(4);
            if (i == current_addr) {
                ImGui::Text("<-- Current");
            }
            else if (i == pio.regs.pc) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "<-- Next");
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void PioStateMachineApp::renderRuntimeWindow() {
    if (!ImGui::Begin("Runtime Info", &show_runtime_window)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Runtime Flags and State");
    ImGui::Separator();

    if (ImGui::BeginTable("RuntimeTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Clock");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", pio.clock);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Current Instruction");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("0x%04X", pio.currentInstruction);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("State Machine Number");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", pio.stateMachineNumber);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Jmp To");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", pio.jmp_to);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Skip Increase PC");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", pio.skip_increase_pc ? "true" : "false");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Delay Delay");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", pio.delay_delay ? "true" : "false");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Skip Delay");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", pio.skip_delay ? "true" : "false");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Exec Command");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", pio.exec_command ? "true" : "false");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Wait is Stalling");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", pio.wait_is_stalling ? "true" : "false");

        ImGui::EndTable();
    }

    ImGui::End();
}

void PioStateMachineApp::renderUI() {
    renderControlWindow();
    renderVariableWindow();
    renderSettingsWindow();
    renderProgramWindow();
    renderRuntimeWindow();
    renderTimingWindow();

    if (!show_control_window && !show_variable_window && !show_settings_window && !show_program_window && !show_runtime_window) {
        done = true;
    }
}


void PioStateMachineApp::updateTimingData() {
    current_cycle += 1;  // Integer increment

    timing_timestamps.push_back(current_cycle);

    // Record data for all pins as 0 or 1 integers
    for (int pin = 0; pin < 32; pin++) {
        timing_values[pin].push_back(pio.gpio.raw_data[pin] ? 1 : 0);  // Pure integers
    }

    // Keep buffer size manageable
    if (timing_timestamps.size() > MAX_TIMING_SAMPLES) {
        timing_timestamps.erase(timing_timestamps.begin());
        for (int pin = 0; pin < 32; pin++) {
            timing_values[pin].erase(timing_values[pin].begin());
        }
    }
}

void PioStateMachineApp::renderTimingWindow() {
    if (!ImGui::Begin("Timing Diagram", &show_timing_window)) {
        ImGui::End();
        return;
    }

    ImGui::Text("GPIO Pin Timing Diagram");
    ImGui::Separator();

    // Compact pin selection in table format
    ImGui::Text("Select up to 5 pins to display:");
    static int selected_pin_list[5] = { -1, -1, -1, -1, -1 };

    if (ImGui::BeginTable("PinSelection", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Pin 1");
        ImGui::TableSetupColumn("Pin 2");
        ImGui::TableSetupColumn("Pin 3");
        ImGui::TableSetupColumn("Pin 4");
        ImGui::TableSetupColumn("Pin 5");
        ImGui::TableHeadersRow();

        // Pin input row
        ImGui::TableNextRow();
        for (int slot = 0; slot < 5; slot++) {
            ImGui::TableSetColumnIndex(slot);
            ImGui::PushID(slot);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputInt("##pin", &selected_pin_list[slot]);

            // Validate range
            if (selected_pin_list[slot] < -1) selected_pin_list[slot] = -1;
            if (selected_pin_list[slot] > 31) selected_pin_list[slot] = 31;
            ImGui::PopID();
        }

        // Current value row
        ImGui::TableNextRow();
        for (int slot = 0; slot < 5; slot++) {
            ImGui::TableSetColumnIndex(slot);
            if (selected_pin_list[slot] >= 0 && selected_pin_list[slot] < 32) {
                ImGui::Text("Current: %d", pio.gpio.raw_data[selected_pin_list[slot]]);
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(-1 = disabled)");
            }
        }

        ImGui::EndTable();
    }

    // Update selected_pins array based on the list
    for (int i = 0; i < 32; i++) {
        selected_pins[i] = false;
    }
    int selected_count = 0;
    for (int slot = 0; slot < 5; slot++) {
        if (selected_pin_list[slot] >= 0 && selected_pin_list[slot] < 32) {
            selected_pins[selected_pin_list[slot]] = true;
            selected_count++;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Clear All")) {
        for (int i = 0; i < 5; i++) {
            selected_pin_list[i] = -1;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear History")) {
        timing_timestamps.clear();
        for (auto& pin_data : timing_values) {
            pin_data.clear();
        }
        current_cycle = 0;
    }

    // Plot with proper vertical separation
    if (timing_timestamps.size() > 1 && selected_count > 0 && ImPlot::BeginPlot("##Timing", ImVec2(-1, 400))) {
        ImPlot::SetupAxes("Clock Cycles", "Channels");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, current_cycle + 1);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1, selected_count * 2, ImPlotCond_Always);

        ImPlot::SetupAxisFormat(ImAxis_X1, "%.0f");

        // Custom Y-axis labels showing pin numbers
        std::vector<double> y_ticks;
        std::vector<const char*> y_labels;
        std::vector<std::string> label_strings;

        int plot_index = 0;
        for (int slot = 0; slot < 5; slot++) {
            int pin = selected_pin_list[slot];
            if (pin >= 0 && pin < 32) {
                y_ticks.push_back(plot_index * 2 + 0.5);
                label_strings.push_back("GPIO" + std::to_string(pin));
                plot_index++;
            }
        }

        for (const auto& str : label_strings) {
            y_labels.push_back(str.c_str());
        }

        if (!y_ticks.empty()) {
            ImPlot::SetupAxisTicks(ImAxis_Y1, y_ticks.data(), static_cast<int>(y_ticks.size()), y_labels.data());
        }

        ImPlot::SetupAxis(ImAxis_Y1, "Channels", ImPlotAxisFlags_Lock);

        // Colors for different pins
        ImVec4 colors[] = {
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red  
            ImVec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow
            ImVec4(1.0f, 0.0f, 1.0f, 1.0f)   // Magenta
        };

        plot_index = 0;
        for (int slot = 0; slot < 5; slot++) {
            int pin = selected_pin_list[slot];
            if (pin >= 0 && pin < 32 && pin < timing_values.size() && !timing_values[pin].empty()) {
                // Convert to double with proper channel separation
                std::vector<double> double_timestamps(timing_timestamps.begin(), timing_timestamps.end());
                std::vector<double> offset_values;
                for (int val : timing_values[pin]) {
                    offset_values.push_back(static_cast<double>(val + plot_index * 2));
                }

                std::string label = "GPIO " + std::to_string(pin);
                ImPlot::SetNextLineStyle(colors[plot_index % 5], 2.0f);
                ImPlot::PlotStairs(label.c_str(),
                    double_timestamps.data(),
                    offset_values.data(),
                    static_cast<int>(timing_timestamps.size()));
                plot_index++;
            }
        }

        ImPlot::EndPlot();  // This was missing - critical!
    }

    ImGui::End();  // This must always be called
}