#pragma once
#include <cstdint>
#include <array>
#include <vector>

class PioStateMachine
{
public:
    uint32_t instructionMemory[32];
    uint32_t currentInstruction;
    uint32_t stateMachineNumber;
    // Runtime emu flags
    int jmp_to = -1;
    bool skip_increase_pc = false;
    bool delay_delay = false;  // For some instruction delays need to be postponed to after the instruction (e.g. wait) has finished
    bool skip_delay = false;   // (s3.4.5.2) for 'out exec' and 'mov exec' "Delay cycles on the initial OUT are ignored"
    bool exec_command = false; // for 'out exec' and 'mov exec', might alter the logic for get nextInstruction for memory

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
        uint32_t status = 0;
    } regs;

    // Configuration settings
    struct Settings
    {
        int sideset_count = 0;
        bool sideset_opt = false;
        bool sideset_pindirs = false;
        int in_base = -1;
        int out_base = -1;
        int set_base = -1;
        int jmp_pin = -1;
        int set_count = -1;
        int out_count = -1;
        int push_threshold = 32;
        int pull_threshold = 32;
        bool in_shift_right = false;
        bool out_shift_right = false;
        bool in_shift_autopush = false;
        bool out_shift_autopull = false;
    } settings;

    // GPIO regs (見s3.4.5)
    struct GPIORegs
    {
        // because of priority, we have sepreate gpio regs
        std::array<int8_t, 32> raw_data;
        std::array<int8_t, 32> set_data;
        std::array<int8_t, 32> out_data;
        std::array<int8_t, 32> sideset;
        std::array<int8_t, 32> pindirs;
    } gpio;

    // FIFOs
    void push_to_rx_fifo();
    void pull_from_tx_fifo();
    std::array<uint32_t, 4> tx_fifo;
    std::array<uint32_t, 4> rx_fifo;
    int tx_fifo_in_use_count = 0;
    int rx_fifo_in_use_count = 0;
    bool push_is_stalling = false;
    bool pull_is_stalling = false;

    // IRQs
    std::array<bool, 8> irq_flags;
    bool irq_is_waiting = false;

    void tick(); // Foward a clock
    void executeInstruction();
    
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

    void doSideSet();
};
