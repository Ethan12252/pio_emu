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
    bool delay_flag = false;

    // State registers
    struct Registers
    {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t ISR = 0;
        uint32_t OSR = 0;
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
    std::array<uint32_t, 4> TxFIFO;
    std::array<uint32_t, 4> RxFIFO;
    int TxFIFO_in_use_count = 0;
    int RxFIFO_in_use_count = 0;
    bool push_is_stalling = false;
    bool pull_is_stalling = false;


    // IRQs
    std::array<bool, 8> irq_flags;
    bool irq_is_waiting = false;

    void tick(); // Foward a clock
    void executeInstruction();

    // private:
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
