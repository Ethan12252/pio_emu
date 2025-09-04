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
    pio.settings.warp_start = 0;
    pio.settings.warp_end = 3;

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
                uint32_t x_val = pio.get_var("regs.x");
                if (ImGui::InputScalar("##x", ImGuiDataType_U32, &x_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.set_var("regs.x", x_val);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Y");
                ImGui::TableSetColumnIndex(1);
                uint32_t y_val = pio.get_var("regs.y");
                if (ImGui::InputScalar("##y", ImGuiDataType_U32, &y_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.set_var("regs.y", y_val);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("ISR");
                ImGui::TableSetColumnIndex(1);
                uint32_t isr_val = pio.get_var("regs.isr");
                if (ImGui::InputScalar("##isr", ImGuiDataType_U32, &isr_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.set_var("regs.isr", isr_val);
                }
                ImGui::SameLine();
                ImGui::Text("(Shift Count:");
                ImGui::SameLine();
                uint32_t isr_count = pio.get_var("regs.isr_shift_count");
                if (ImGui::InputScalar("##isrcount", ImGuiDataType_U32, &isr_count)) {
                    if (isr_count <= 32) pio.set_var("regs.isr_shift_count", isr_count);
                }
                ImGui::SameLine();
                ImGui::Text(")");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("OSR");
                ImGui::TableSetColumnIndex(1);
                uint32_t osr_val = pio.get_var("regs.osr");
                if (ImGui::InputScalar("##osr", ImGuiDataType_U32, &osr_val, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.set_var("regs.osr", osr_val);
                }
                ImGui::SameLine();
                ImGui::Text("(Shift Count:");
                ImGui::SameLine();
                uint32_t osr_count = pio.get_var("regs.osr_shift_count");
                if (ImGui::InputScalar("##osrcount", ImGuiDataType_U32, &osr_count)) {
                    if (osr_count <= 32) pio.set_var("regs.osr_shift_count", osr_count);
                }
                ImGui::SameLine();
                ImGui::Text(")");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("PC");
                ImGui::TableSetColumnIndex(1);
                uint32_t pc_val = pio.get_var("regs.pc");
                if (ImGui::InputScalar("##pc", ImGuiDataType_U32, &pc_val)) {
                    if (pc_val < 32) pio.set_var("regs.pc", pc_val);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Delay");
                ImGui::TableSetColumnIndex(1);
                uint32_t delay_val = pio.get_var("regs.delay");
                if (ImGui::InputScalar("##delay", ImGuiDataType_U32, &delay_val)) {
                    pio.set_var("regs.delay", delay_val);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Status");
                ImGui::TableSetColumnIndex(1);
                uint32_t status_val = pio.get_var("regs.status");
                if (ImGui::InputScalar("##status", ImGuiDataType_U32, &status_val)) {
                    pio.set_var("regs.status", status_val);
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("GPIO")) {
            if (ImGui::BeginTable("GPIOTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Pin 22 Raw Data");
                ImGui::TableSetColumnIndex(1);
                int raw22 = pio.gpio.raw_data[22];
                if (ImGui::InputInt("##raw22", &raw22, 0)) {
                    pio.gpio.raw_data[22] = static_cast<int8_t>(raw22 & 1);
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Pin 22 Pindir");
                ImGui::TableSetColumnIndex(1);
                int pindir22 = pio.gpio.pindirs[22];
                if (ImGui::InputInt("##pindir22", &pindir22, 0)) {
                    pio.gpio.pindirs[22] = static_cast<int8_t>(pindir22 & 1);
                }
                ImGui::SameLine();
                ImGui::Text("(0=output,1=input)");

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
                ImGui::Text("TX FIFO[0]");
                ImGui::TableSetColumnIndex(1);
                uint32_t tx0 = pio.fifo.tx_fifo[0];
                if (ImGui::InputScalar("##tx0", ImGuiDataType_U32, &tx0, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.fifo.tx_fifo[0] = tx0;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("RX FIFO[0]");
                ImGui::TableSetColumnIndex(1);
                uint32_t rx0 = pio.fifo.rx_fifo[0];
                if (ImGui::InputScalar("##rx0", ImGuiDataType_U32, &rx0, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    pio.fifo.rx_fifo[0] = rx0;
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

void PioStateMachineApp::renderProgramWindow() {
    if (!ImGui::Begin("Program", &show_program_window)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Instruction Memory");
    ImGui::Separator();

    if (ImGui::BeginTable("InstructionTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Instruction");
        ImGui::TableHeadersRow();

        for (int i = 0; i < program_size; ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("0x%02X", i);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            uint16_t instr = pio.instructionMemory[i];
            if (ImGui::InputScalar("##instr", ImGuiDataType_U16, &instr, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
                pio.instructionMemory[i] = instr;
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void PioStateMachineApp::renderRuntimeWindow() {
    // Placeholder for runtime info window
    if (!ImGui::Begin("Runtime Info", &show_runtime_window)) {
        ImGui::End();
        return;
    }
    ImGui::Text("Runtime info window not yet implemented.");
    ImGui::End();
}

void PioStateMachineApp::renderUI() {
    renderControlWindow();
    renderVariableWindow();
    renderProgramWindow();
    renderRuntimeWindow();

    if (!show_control_window && !show_variable_window && !show_program_window && !show_runtime_window) {
        done = true;
    }
}