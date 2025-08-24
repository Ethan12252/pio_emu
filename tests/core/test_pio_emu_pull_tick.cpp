#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

// Helper function to build IRQ instructions
uint16_t buildIrqInstruction(bool clear, bool wait, uint8_t index)
{
    return (0b110 << 13) | ((clear ? 1 : 0) << 6) | ((wait ? 1 : 0) << 5) | (index & 0x1F);
}


TEST_CASE("pull instruction test")
{
    PioStateMachine pio;

    SUBCASE("pull block")
    {
        // Set IRQ flag 3
        pio.instructionMemory[0] = 0x80a0; //  0: pull block
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 0;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;
        pio.tick();
        CHECK(pio.regs.osr == 0xdeadbeef);
        CHECK(pio.tx_fifo_count == 0);
    }

    SUBCASE("pull block")
    {
        // ifempty should do nothing if the osr_shift_count havent reach pull_thres (like autopull)
        pio.instructionMemory[0] = 0x80e0; // pull ifempty block   
        pio.instructionMemory[1] = 0x607e;  // out    null, 30 
        pio.instructionMemory[2] = 0x0000; // jmp 0
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 8;
        pio.settings.pull_threshold = 32;
        pio.settings.autopull_enable = false; // disable auto pull
        pio.settings.out_shift_right = true;

        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;

        pio.tick(); // should do nothing
        CHECK(pio.regs.pc == 1);
        CHECK(pio.regs.osr == 0x00abcdef);
        CHECK(pio.regs.osr_shift_count == 8);

        pio.tick(); // out osr to empty
        CHECK(pio.regs.pc == 2);
        CHECK(pio.regs.osr == 0);
        CHECK(pio.regs.osr_shift_count == 32);

        pio.tick(); // jump

        pio.tick(); // should do pull
        CHECK(pio.regs.osr == 0xdeadbeef);
        CHECK(pio.regs.osr_shift_count == 0);
        CHECK(pio.tx_fifo_count == 0);
    }
}
