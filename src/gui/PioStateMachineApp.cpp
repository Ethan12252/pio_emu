#include "PioStateMachineApp.h"

void PioStateMachineApp::initialize() {
    // Reset the emulator to a clean state
    pio = PioStateMachine();

    // Load WS2812 program
    static const uint16_t ws2812_program_instructions[] = {
        0x6321, // out x, 1 side 0 [3]
        0x1223, // jmp !x, 3 side 1 [2]
        0x1200, // jmp 0 side 1 [2]
        0xa242, // nop side 0 [2]
    };

    program_size = sizeof(ws2812_program_instructions) / sizeof(uint16_t);
    for (int i = 0; i < program_size; ++i) {
        pio.instructionMemory[i] = ws2812_program_instructions[i];
    }

    // Configure settings for WS2812
    pio.settings.sideset_opt = false;
    pio.settings.sideset_count = 1;
    pio.settings.sideset_base = 22;
    pio.settings.pull_threshold = 24;
    pio.settings.out_shift_right = false;
    pio.settings.autopull_enable = true;
    pio.settings.wrap_start = 0;
    pio.settings.wrap_end = 3;

    pio.gpio.pindirs[22] = 0; // Output

    // Initialize FIFO data
    pio.fifo.tx_fifo[0] = 0xbaabff00;
    pio.fifo.tx_fifo_count = 1;

    // Reset GUI state
    tick_steps = 1;
    show_control_window = true;
    show_variable_window = true;
    show_program_window = true;
    show_runtime_window = true;
    show_settings_window = true;
    done = false;
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
    }

    ImGui::SameLine();
    ImGui::InputInt("Steps", &tick_steps, 1, 100);
    if (tick_steps < 1) tick_steps = 1;
    ImGui::SameLine();
    if (ImGui::Button("Tick Multiple")) {
        for (int i = 0; i < tick_steps; ++i) {
            pio.tick();
        }
    }

    if (ImGui::Button("Reset Emulator")) {
        reset();
    }

    ImGui::Separator();
    ImGui::Text("Run Until Condition");
    static char var_name[32] = "regs.pc";
    static uint32_t target_value = 0;
    static int max_cycles = 10000;
    ImGui::InputText("Variable Name", var_name, IM_ARRAYSIZE(var_name));
    ImGui::InputScalar("Target Value", ImGuiDataType_U32, &target_value, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputInt("Max Cycles", &max_cycles);
    if (max_cycles < 1) max_cycles = 1;
    if (ImGui::Button("Run Until")) {
        pio.run_until_var(var_name, target_value, max_cycles);
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

                for (int i = 0; i < 8; ++i) {
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

                for (int i = 0; i < 8; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("RX FIFO[%d]", i);
                    ImGui::TableSetColumnIndex(1);
                    uint32_t rx_val = pio.fifo.rx_fifo[i];
                    ImGui::PushID(i + 8);
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
    ImGui::Separator();

    if (ImGui::BeginTable("SettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, ImVec2(0.0f, ImGui::GetTextLineHeight() * 20))) {
        ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset Count");
        ImGui::TableSetColumnIndex(1);
        int sideset_count = pio.settings.sideset_count;
        if (ImGui::InputInt("##sideset_count", &sideset_count)) {
            pio.settings.sideset_count = sideset_count;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset Opt");
        ImGui::TableSetColumnIndex(1);
        bool sideset_opt = pio.settings.sideset_opt;
        if (ImGui::Checkbox("##sideset_opt", &sideset_opt)) {
            pio.settings.sideset_opt = sideset_opt;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset to Pindirs");
        ImGui::TableSetColumnIndex(1);
        bool sideset_to_pindirs = pio.settings.sideset_to_pindirs;
        if (ImGui::Checkbox("##sideset_to_pindirs", &sideset_to_pindirs)) {
            pio.settings.sideset_to_pindirs = sideset_to_pindirs;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Sideset Base");
        ImGui::TableSetColumnIndex(1);
        int sideset_base = pio.settings.sideset_base;
        if (ImGui::InputInt("##sideset_base", &sideset_base)) {
            pio.settings.sideset_base = sideset_base;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("In Base");
        ImGui::TableSetColumnIndex(1);
        int in_base = pio.settings.in_base;
        if (ImGui::InputInt("##in_base", &in_base)) {
            pio.settings.in_base = in_base;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Base");
        ImGui::TableSetColumnIndex(1);
        int out_base = pio.settings.out_base;
        if (ImGui::InputInt("##out_base", &out_base)) {
            pio.settings.out_base = out_base;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Set Base");
        ImGui::TableSetColumnIndex(1);
        int set_base = pio.settings.set_base;
        if (ImGui::InputInt("##set_base", &set_base)) {
            pio.settings.set_base = set_base;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Jmp Pin");
        ImGui::TableSetColumnIndex(1);
        int jmp_pin = pio.settings.jmp_pin;
        if (ImGui::InputInt("##jmp_pin", &jmp_pin)) {
            pio.settings.jmp_pin = jmp_pin;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Set Count");
        ImGui::TableSetColumnIndex(1);
        int set_count = pio.settings.set_count;
        if (ImGui::InputInt("##set_count", &set_count)) {
            pio.settings.set_count = set_count;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Count");
        ImGui::TableSetColumnIndex(1);
        int out_count = pio.settings.out_count;
        if (ImGui::InputInt("##out_count", &out_count)) {
            pio.settings.out_count = out_count;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Push Threshold");
        ImGui::TableSetColumnIndex(1);
        uint32_t push_threshold = pio.settings.push_threshold;
        if (ImGui::InputScalar("##push_threshold", ImGuiDataType_U32, &push_threshold)) {
            pio.settings.push_threshold = push_threshold;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Pull Threshold");
        ImGui::TableSetColumnIndex(1);
        uint32_t pull_threshold = pio.settings.pull_threshold;
        if (ImGui::InputScalar("##pull_threshold", ImGuiDataType_U32, &pull_threshold)) {
            pio.settings.pull_threshold = pull_threshold;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("FIFO Level N");
        ImGui::TableSetColumnIndex(1);
        int fifo_level_N = pio.settings.fifo_level_N;
        if (ImGui::InputInt("##fifo_level_N", &fifo_level_N)) {
            pio.settings.fifo_level_N = fifo_level_N;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Warp Start");
        ImGui::TableSetColumnIndex(1);
        uint32_t wrap_start = pio.settings.wrap_start;
        if (ImGui::InputScalar("##wrap_start", ImGuiDataType_U32, &wrap_start)) {
            pio.settings.wrap_start = wrap_start;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Warp End");
        ImGui::TableSetColumnIndex(1);
        uint32_t wrap_end = pio.settings.wrap_end;
        if (ImGui::InputScalar("##wrap_end", ImGuiDataType_U32, &wrap_end)) {
            pio.settings.wrap_end = wrap_end;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("In Shift Right");
        ImGui::TableSetColumnIndex(1);
        bool in_shift_right = pio.settings.in_shift_right;
        if (ImGui::Checkbox("##in_shift_right", &in_shift_right)) {
            pio.settings.in_shift_right = in_shift_right;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Shift Right");
        ImGui::TableSetColumnIndex(1);
        bool out_shift_right = pio.settings.out_shift_right;
        if (ImGui::Checkbox("##out_shift_right", &out_shift_right)) {
            pio.settings.out_shift_right = out_shift_right;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("In Shift Autopush");
        ImGui::TableSetColumnIndex(1);
        bool in_shift_autopush = pio.settings.in_shift_autopush;
        if (ImGui::Checkbox("##in_shift_autopush", &in_shift_autopush)) {
            pio.settings.in_shift_autopush = in_shift_autopush;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Out Shift Autopull");
        ImGui::TableSetColumnIndex(1);
        bool out_shift_autopull = pio.settings.out_shift_autopull;
        if (ImGui::Checkbox("##out_shift_autopull", &out_shift_autopull)) {
            pio.settings.out_shift_autopull = out_shift_autopull;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Autopull Enable");
        ImGui::TableSetColumnIndex(1);
        bool autopull_enable = pio.settings.autopull_enable;
        if (ImGui::Checkbox("##autopull_enable", &autopull_enable)) {
            pio.settings.autopull_enable = autopull_enable;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Autopush Enable");
        ImGui::TableSetColumnIndex(1);
        bool autopush_enable = pio.settings.autopush_enable;
        if (ImGui::Checkbox("##autopush_enable", &autopush_enable)) {
            pio.settings.autopush_enable = autopush_enable;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Status Sel");
        ImGui::TableSetColumnIndex(1);
        bool status_sel = pio.settings.status_sel;
        if (ImGui::Checkbox("##status_sel", &status_sel)) {
            pio.settings.status_sel = status_sel;
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
    ImGui::Separator();

    if (ImGui::BeginTable("InstructionTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Instruction");
        ImGui::TableSetupColumn("Current PC");
        ImGui::TableHeadersRow();

        uint32_t current_pc = pio.regs.pc;

        for (int i = 0; i < 32; ++i) {  // Show all 32 possible instructions
            ImGui::TableNextRow();
            if (i == current_pc) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.6f, 0.3f, 0.65f)));
            }
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("0x%02X", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            uint16_t instr = pio.instructionMemory[i];
            if (ImGui::InputScalar("##instr", ImGuiDataType_U16, &instr, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
                pio.instructionMemory[i] = instr;
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(2);
            if (i == current_pc) {
                ImGui::Text("<-- Current");
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
        int clock_val = pio.clock;
        ImGui::Text("%d", clock_val);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Current Instruction");
        ImGui::TableSetColumnIndex(1);
        uint16_t curr_instr = pio.currentInstruction;
        ImGui::Text("0x%04X", curr_instr);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("State Machine Number");
        ImGui::TableSetColumnIndex(1);
        uint16_t sm_num = pio.stateMachineNumber;
        ImGui::Text("%u", sm_num);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Jmp To");
        ImGui::TableSetColumnIndex(1);
        int jmp_to = pio.jmp_to;
        if (ImGui::InputInt("##jmp_to", &jmp_to)) {
            pio.jmp_to = jmp_to;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Skip Increase PC");
        ImGui::TableSetColumnIndex(1);
        bool skip_inc_pc = pio.skip_increase_pc;
        if (ImGui::Checkbox("##skip_inc_pc", &skip_inc_pc)) {
            pio.skip_increase_pc = skip_inc_pc;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Delay Delay");
        ImGui::TableSetColumnIndex(1);
        bool delay_delay = pio.delay_delay;
        if (ImGui::Checkbox("##delay_delay", &delay_delay)) {
            pio.delay_delay = delay_delay;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Skip Delay");
        ImGui::TableSetColumnIndex(1);
        bool skip_delay = pio.skip_delay;
        if (ImGui::Checkbox("##skip_delay", &skip_delay)) {
            pio.skip_delay = skip_delay;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Exec Command");
        ImGui::TableSetColumnIndex(1);
        bool exec_command = pio.exec_command;
        if (ImGui::Checkbox("##exec_command", &exec_command)) {
            pio.exec_command = exec_command;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Wait is Stalling");
        ImGui::TableSetColumnIndex(1);
        bool wait_stall = pio.wait_is_stalling;
        if (ImGui::Checkbox("##wait_stall", &wait_stall)) {
            pio.wait_is_stalling = wait_stall;
        }

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

    if (!show_control_window && !show_variable_window && !show_settings_window && !show_program_window && !show_runtime_window) {
        done = true;
    }
}