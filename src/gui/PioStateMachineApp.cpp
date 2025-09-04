#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "PioStateMachineApp.h"

void PioStateMachineApp::initialize() {
    // Initialize PIO Emulator
    PioStateMachine pio;

    // Load WS2812 program
    static const uint16_t ws2812_program_instructions[] = {
        0x6321, //  0: out    x, 1    side 0 [3] 
        0x1223, //  1: jmp    !x, 3   side 1 [2] 
        0x1200, //  2: jmp    0       side 1 [2] 
        0xa242, //  3: nop            side 0 [2] 
    };
    
    for (size_t i = 0; i < sizeof(ws2812_program_instructions) / sizeof(uint16_t); i++)
        pio.instructionMemory[i] = ws2812_program_instructions[i];

    // Settings as in ws2812 example
    pio.settings.sideset_opt = false;
    pio.settings.sideset_count = 1;
    pio.settings.sideset_base = 22;
    pio.settings.pull_threshold = 24;
    pio.settings.out_shift_right = false;
    pio.settings.autopull_enable = true;
    pio.settings.warp_start = 0;
    pio.settings.warp_end = 3;

    pio.gpio.pindirs[22] = 0;  // Output

    // Data
    pio.fifo.tx_fifo[0] = 0xba'ab'ff'00;
    pio.fifo.tx_fifo_count = 1;
}

void PioStateMachineApp::reset() {
    pio = PioStateMachine();
    initialize();
}

void PioStateMachineApp::renderUI() {
        // Controls Window
        if (show_controls && ImGui::Begin("Controls", &show_controls))
        {
            if (ImGui::Button("Tick")) {
                pio.tick();
            }

            ImGui::InputInt("Steps", &tick_steps, 1, 100);
            ImGui::SameLine();
            if (ImGui::Button("Tick Multiple")) {
                for (int i = 0; i < tick_steps; ++i) {
                    pio.tick();
                }
            }
            if (ImGui::Button("Reset")) {
                // TODO: will implement a reset function in PioStateMachine
            }
            ImGui::End();
        }

        // Main Window with Tabs
        if (show_main_window && ImGui::Begin("PIO Emulator", &show_main_window))
        {
            if (ImGui::BeginTabBar("PioTabs"))
            {
                if (ImGui::BeginTabItem("Runtime"))
                {
                    if (ImGui::BeginTable("RuntimeTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Property");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Clock");
                        ImGui::TableSetColumnIndex(1);
                        int clk = pio.clock;
                        if (ImGui::InputInt("##clock", &clk))
                            pio.clock = clk;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Current Instruction");
                        ImGui::TableSetColumnIndex(1);
                        uint16_t curr_instr = pio.currentInstruction;
                        if (ImGui::InputScalar("##currinstr", ImGuiDataType_U16, &curr_instr, NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.currentInstruction = curr_instr;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("JMP To");
                        ImGui::TableSetColumnIndex(1);
                        int jmp = pio.jmp_to;
                        if (ImGui::InputInt("##jmpto", &jmp))
                            pio.jmp_to = jmp;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Skip Increase PC");
                        ImGui::TableSetColumnIndex(1);
                        bool skip_pc = pio.skip_increase_pc;
                        if (ImGui::Checkbox("##skippc", &skip_pc))
                            pio.skip_increase_pc = skip_pc;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Delay Delay");
                        ImGui::TableSetColumnIndex(1);
                        bool delay_d = pio.delay_delay;
                        if (ImGui::Checkbox("##delaydelay", &delay_d))
                            pio.delay_delay = delay_d;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Skip Delay");
                        ImGui::TableSetColumnIndex(1);
                        bool skip_d = pio.skip_delay;
                        if (ImGui::Checkbox("##skipdelay", &skip_d))
                            pio.skip_delay = skip_d;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Exec Command");
                        ImGui::TableSetColumnIndex(1);
                        bool exec_c = pio.exec_command;
                        if (ImGui::Checkbox("##execcmd", &exec_c))
                            pio.exec_command = exec_c;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Wait Stalling");
                        ImGui::TableSetColumnIndex(1);
                        bool wait_s = pio.wait_is_stalling;
                        if (ImGui::Checkbox("##waitstall", &wait_s))
                            pio.wait_is_stalling = wait_s;

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Registers"))
                {
                    if (ImGui::BeginTable("RegistersTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Register");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("X");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t x_val = pio.regs.x;
                        if (ImGui::InputScalar("##x", ImGuiDataType_U32, &x_val, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.regs.x = x_val;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Y");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t y_val = pio.regs.y;
                        if (ImGui::InputScalar("##y", ImGuiDataType_U32, &y_val, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.regs.y = y_val;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("ISR");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t isr_val = pio.regs.isr;
                        if (ImGui::InputScalar("##isr", ImGuiDataType_U32, &isr_val, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.regs.isr = isr_val;
                        ImGui::SameLine();
                        ImGui::Text("(Shift Count:");
                        ImGui::SameLine();
                        uint32_t isr_count = pio.regs.isr_shift_count;
                        if (ImGui::InputScalar("##isrcount", ImGuiDataType_U32, &isr_count))
                            pio.regs.isr_shift_count = isr_count;
                        ImGui::SameLine();
                        ImGui::Text(")");

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("OSR");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t osr_val = pio.regs.osr;
                        if (ImGui::InputScalar("##osr", ImGuiDataType_U32, &osr_val, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.regs.osr = osr_val;
                        ImGui::SameLine();
                        ImGui::Text("(Shift Count:");
                        ImGui::SameLine();
                        uint32_t osr_count = pio.regs.osr_shift_count;
                        if (ImGui::InputScalar("##osrcount", ImGuiDataType_U32, &osr_count))
                            pio.regs.osr_shift_count = osr_count;
                        ImGui::SameLine();
                        ImGui::Text(")");

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("PC");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t pc_val = pio.regs.pc;
                        if (ImGui::InputScalar("##pc", ImGuiDataType_U32, &pc_val))
                            pio.regs.pc = pc_val;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Delay");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t delay_val = pio.regs.delay;
                        if (ImGui::InputScalar("##delay", ImGuiDataType_U32, &delay_val))
                            pio.regs.delay = delay_val;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Status");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t status_val = pio.regs.status;
                        if (ImGui::InputScalar("##status", ImGuiDataType_U32, &status_val))
                            pio.regs.status = status_val;

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings"))
                {
                    if (ImGui::BeginTable("SettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Setting");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Sideset Count");
                        ImGui::TableSetColumnIndex(1);
                        int sideset_cnt = pio.settings.sideset_count;
                        if (ImGui::InputInt("##sidesetcnt", &sideset_cnt))
                            pio.settings.sideset_count = sideset_cnt;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Sideset Opt");
                        ImGui::TableSetColumnIndex(1);
                        bool sideset_o = pio.settings.sideset_opt;
                        if (ImGui::Checkbox("##sidesetopt", &sideset_o))
                            pio.settings.sideset_opt = sideset_o;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Sideset to Pindirs");
                        ImGui::TableSetColumnIndex(1);
                        bool sideset_p = pio.settings.sideset_to_pindirs;
                        if (ImGui::Checkbox("##sidesetpindirs", &sideset_p))
                            pio.settings.sideset_to_pindirs = sideset_p;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Sideset Base");
                        ImGui::TableSetColumnIndex(1);
                        int sideset_b = pio.settings.sideset_base;
                        if (ImGui::InputInt("##sidesetbase", &sideset_b))
                            pio.settings.sideset_base = sideset_b;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("In Base");
                        ImGui::TableSetColumnIndex(1);
                        int in_b = pio.settings.in_base;
                        if (ImGui::InputInt("##inbase", &in_b))
                            pio.settings.in_base = in_b;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Out Base");
                        ImGui::TableSetColumnIndex(1);
                        int out_b = pio.settings.out_base;
                        if (ImGui::InputInt("##outbase", &out_b))
                            pio.settings.out_base = out_b;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Set Base");
                        ImGui::TableSetColumnIndex(1);
                        int set_b = pio.settings.set_base;
                        if (ImGui::InputInt("##setbase", &set_b))
                            pio.settings.set_base = set_b;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("JMP Pin");
                        ImGui::TableSetColumnIndex(1);
                        int jmp_p = pio.settings.jmp_pin;
                        if (ImGui::InputInt("##jmppin", &jmp_p))
                            pio.settings.jmp_pin = jmp_p;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Set Count");
                        ImGui::TableSetColumnIndex(1);
                        int set_cnt = pio.settings.set_count;
                        if (ImGui::InputInt("##setcnt", &set_cnt))
                            pio.settings.set_count = set_cnt;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Out Count");
                        ImGui::TableSetColumnIndex(1);
                        int out_cnt = pio.settings.out_count;
                        if (ImGui::InputInt("##outcnt", &out_cnt))
                            pio.settings.out_count = out_cnt;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Push Threshold");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t push_th = pio.settings.push_threshold;
                        if (ImGui::InputScalar("##pushth", ImGuiDataType_U32, &push_th))
                            pio.settings.push_threshold = push_th;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Pull Threshold");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t pull_th = pio.settings.pull_threshold;
                        if (ImGui::InputScalar("##pullth", ImGuiDataType_U32, &pull_th))
                            pio.settings.pull_threshold = pull_th;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("FIFO Level N");
                        ImGui::TableSetColumnIndex(1);
                        int fifo_n = pio.settings.fifo_level_N;
                        if (ImGui::InputInt("##fifon", &fifo_n))
                            pio.settings.fifo_level_N = fifo_n;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Warp Start");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t warp_s = pio.settings.warp_start;
                        if (ImGui::InputScalar("##warps", ImGuiDataType_U32, &warp_s))
                            pio.settings.warp_start = warp_s;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Warp End");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t warp_e = pio.settings.warp_end;
                        if (ImGui::InputScalar("##warpe", ImGuiDataType_U32, &warp_e))
                            pio.settings.warp_end = warp_e;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("In Shift Right");
                        ImGui::TableSetColumnIndex(1);
                        bool in_sr = pio.settings.in_shift_right;
                        if (ImGui::Checkbox("##insr", &in_sr))
                            pio.settings.in_shift_right = in_sr;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Out Shift Right");
                        ImGui::TableSetColumnIndex(1);
                        bool out_sr = pio.settings.out_shift_right;
                        if (ImGui::Checkbox("##outsr", &out_sr))
                            pio.settings.out_shift_right = out_sr;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("In Shift Autopush");
                        ImGui::TableSetColumnIndex(1);
                        bool in_ap = pio.settings.in_shift_autopush;
                        if (ImGui::Checkbox("##inap", &in_ap))
                            pio.settings.in_shift_autopush = in_ap;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Out Shift Autopull");
                        ImGui::TableSetColumnIndex(1);
                        bool out_ap = pio.settings.out_shift_autopull;
                        if (ImGui::Checkbox("##outap", &out_ap))
                            pio.settings.out_shift_autopull = out_ap;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Autopull Enable");
                        ImGui::TableSetColumnIndex(1);
                        bool autopull_e = pio.settings.autopull_enable;
                        if (ImGui::Checkbox("##autopulle", &autopull_e))
                            pio.settings.autopull_enable = autopull_e;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Autopush Enable");
                        ImGui::TableSetColumnIndex(1);
                        bool autopush_e = pio.settings.autopush_enable;
                        if (ImGui::Checkbox("##autopushe", &autopush_e))
                            pio.settings.autopush_enable = autopush_e;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Status Sel");
                        ImGui::TableSetColumnIndex(1);
                        bool status_s = pio.settings.status_sel;
                        if (ImGui::Checkbox("##statussel", &status_s))
                            pio.settings.status_sel = status_s;

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("GPIO"))
                {
                    if (ImGui::BeginTable("GPIOTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Property");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Pin 22 Raw Data");
                        ImGui::TableSetColumnIndex(1);
                        int raw22 = pio.gpio.raw_data[22];
                        if (ImGui::InputInt("##raw22", &raw22, 0))
                            pio.gpio.raw_data[22] = static_cast<int8_t>(raw22 & 1);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Pin 22 Pindir");
                        ImGui::TableSetColumnIndex(1);
                        int pindir22 = pio.gpio.pindirs[22];
                        if (ImGui::InputInt("##pindir22", &pindir22, 0))
                            pio.gpio.pindirs[22] = static_cast<int8_t>(pindir22 & 1);
                        ImGui::SameLine();
                        ImGui::Text("(0=output,1=input)");

                        // Add more pins if needed

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("FIFOs"))
                {
                    if (ImGui::BeginTable("FIFOsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Property");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("TX FIFO Count");
                        ImGui::TableSetColumnIndex(1);
                        int tx_count = pio.fifo.tx_fifo_count;
                        if (ImGui::InputInt("##txcount", &tx_count))
                            pio.fifo.tx_fifo_count = static_cast<uint8_t>(tx_count);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("RX FIFO Count");
                        ImGui::TableSetColumnIndex(1);
                        int rx_count = pio.fifo.rx_fifo_count;
                        if (ImGui::InputInt("##rxcount", &rx_count))
                            pio.fifo.rx_fifo_count = static_cast<uint8_t>(rx_count);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Push Stalling");
                        ImGui::TableSetColumnIndex(1);
                        bool push_s = pio.fifo.push_is_stalling;
                        if (ImGui::Checkbox("##pushstall", &push_s))
                            pio.fifo.push_is_stalling = push_s;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Pull Stalling");
                        ImGui::TableSetColumnIndex(1);
                        bool pull_s = pio.fifo.pull_is_stalling;
                        if (ImGui::Checkbox("##pullstall", &pull_s))
                            pio.fifo.pull_is_stalling = pull_s;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("TX FIFO[0]");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t tx0 = pio.fifo.tx_fifo[0];
                        if (ImGui::InputScalar("##tx0", ImGuiDataType_U32, &tx0, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.fifo.tx_fifo[0] = tx0;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("RX FIFO[0]");
                        ImGui::TableSetColumnIndex(1);
                        uint32_t rx0 = pio.fifo.rx_fifo[0];
                        if (ImGui::InputScalar("##rx0", ImGuiDataType_U32, &rx0, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
                            pio.fifo.rx_fifo[0] = rx0;

                        // Add more FIFO entries if needed

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("IRQs"))
                {
                    if (ImGui::BeginTable("IRQTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Property");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("IRQ Waiting");
                        ImGui::TableSetColumnIndex(1);
                        bool irq_w = pio.irq_is_waiting;
                        if (ImGui::Checkbox("##irqwait", &irq_w))
                            pio.irq_is_waiting = irq_w;

                        for (int i = 0; i < 8; ++i) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("IRQ Flag %d", i);
                            ImGui::TableSetColumnIndex(1);
                            bool irq_f = pio.irq_flags[i];
                            if (ImGui::Checkbox(("##irq" + std::to_string(i)).c_str(), &irq_f))
                                pio.irq_flags[i] = irq_f;
                        }

                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
}