#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "Logger/Logger.h"

class PioStateMachine
{
public:
    std::array<uint16_t, 32> instructionMemory;
    uint16_t currentInstruction;
    uint16_t stateMachineNumber;

    // Runtime emu flags
    int jmp_to = -1;
    bool skip_increase_pc = false;
    bool delay_delay = false;
    // For some instruction delays need to be postponed to after the instruction (e.g. wait) has finished
    bool skip_delay = false;   // (s3.4.5.2) for 'out exec' and 'mov exec' "Delay cycles on the initial OUT are ignored"
    bool exec_command = false; // for 'out exec' and 'mov exec', might alter the logic for get nextInstruction for memory
    int clock = 0;

    // State registers
    struct Registers
    {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t isr = 0;
        uint32_t osr = 0;
        uint32_t isr_shift_count = 0;
        uint32_t osr_shift_count = 0;
        uint32_t pc = 0;
        uint32_t delay = 0;
        uint32_t status = 0;  // Indecate FIFO level > fifo_level_N, status_sel 0 for Tx 1 for Rx
    } regs;

    // Configuration settings
    struct Settings
    {
        int  sideset_count = 0;  // bit count without opt bit
        bool sideset_opt = false;
        bool sideset_to_pindirs = false; 
        int  sideset_base = -1;
        int  in_base = -1;
        int  out_base = -1;
        int  set_base = -1;
        int  jmp_pin = -1;
        int  set_count = -1;
        int  out_count = -1;
        int  push_threshold = 32;
        int  pull_threshold = 32;
        int  fifo_level_N = -1;
        int  warp_start = 0;
        int  warp_end = 31;
        bool in_shift_right = false;
        bool out_shift_right = false;
        bool in_shift_autopush = false;
        bool out_shift_autopull = false;
        bool autopull_enable = false;
        bool autopush_enable = false;
        bool status_sel = false;
    } settings;

    // GPIO regs (見s3.4.5)
    struct GPIORegs
    {
        // because of priority, we have sepreate gpio regs
        std::array<int8_t, 32> raw_data;
        std::array<int8_t, 32> set_data;
        std::array<int8_t, 32> out_data;
        std::array<int8_t, 32> external_data;
        std::array<int8_t, 32> sideset_data;

        // pindirs (0 for output, 1 for input)
        std::array<int8_t, 32> pindirs;  
        std::array<int8_t, 32> set_pindirs;
        std::array<int8_t, 32> out_pindirs;
        std::array<int8_t, 32> sideset_pindirs;
    } gpio;

    // FIFOs
    void push_to_rx_fifo();
    void pull_from_tx_fifo();
    std::array<uint32_t, 4> tx_fifo = { 0 };
    std::array<uint32_t, 4> rx_fifo = { 0 };
    int tx_fifo_count = 0;
    int rx_fifo_count = 0;
    bool push_is_stalling = false; // TODO: use of these variable need check
    bool pull_is_stalling = false;

    // IRQs
    std::array<bool, 8> irq_flags = { 0 };
    bool irq_is_waiting = false;

    void executeInstruction();
    void tick(); // Forward a clock
    PioStateMachine();

    // Instruction handlers
    void executeJmp();
    void executeWait();
    void executeIn();
    void executeOut();
    void executePush();
    void executePull();
    void executeMov();
    void executeIrq();
    void executeSet();

    void doSideSet(uint16_t delay_side_set_field);
    void setAllGpio();

private:
    /*void tick_handle_delay();*/
};
