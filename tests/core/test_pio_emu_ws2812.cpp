#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"


static const uint16_t ws2812_program_instructions[] = {
    //     .wrap_target
    0x6321, //  0: out    x, 1    side 0 [3] ; Side-set still takes place when instruction stalls
    0x1223, //  1: jmp    !x, 3   side 1 [2] ; Branch on the bit we shifted out. Positive pulse
    0x1200, //  2: jmp    0       side 1 [2] ; Continue driving high, for a long pulse
    0xa242, //  3: nop            side 0 [2] ; Or drive low, for a short pulse
    //     .wrap
};

void check1(PioStateMachine& pio)
{
    for (int i = 0; i < 6; ++i)
    {
        pio.tick(); // Update the emulator
        CHECK(pio.gpio.raw_data[22] == 1);
    }
    for (int i = 0; i < 4; ++i)
    {
        pio.tick(); // Update the emulator
        CHECK(pio.gpio.raw_data[22] == 0);
    }
}

void check0(PioStateMachine& pio)
{
    for (int i = 0; i < 3; ++i)
    {
        pio.tick(); // Update the emulator
        CHECK(pio.gpio.raw_data[22] == 1);
    }
    for (int i = 0; i < 7; ++i)
    {
        pio.tick(); // Update the emulator
        CHECK(pio.gpio.raw_data[22] == 0);
    }
}


TEST_CASE("JMP Always")
{
    PioStateMachine pio;

    // Put in instructions 
    for (size_t i = 0; i < _countof(ws2812_program_instructions); i++)
        pio.instructionMemory[i] = ws2812_program_instructions[i];

    // settings
    pio.settings.sideset_opt = false;    // side-set bit is mandatory 
    pio.settings.sideset_count = 1;      // 1bit side-set, 4bit delay
    pio.settings.sideset_base = 22;      // gpio pin 22
    pio.settings.pull_threshold = 24;    // 24bit gbr data
    pio.settings.out_shift_right = false;// shift left
    pio.settings.autopull_enable = true; // we need auto pull
    pio.settings.wrap_start = 0;
    pio.settings.wrap_end = 3;

    pio.gpio.pindirs[22] = 0;   // pin22 pindir to output

    // data
    uint32_t grbdata = 0xba'ab'ff'00;  // g:10111010 r:10101011 b:11111111 00000000 
    pio.fifo.tx_fifo[0] = grbdata;
    pio.fifo.tx_fifo_count++;

    // tick
    pio.tick(); pio.tick(); pio.tick(); pio.tick(); pio.tick();
    for (int i = 0; i < 24; ++i)
    {
        INFO("Bit:", i, " osr:", pio.regs.osr);
        if (grbdata & (1 << (31 - i)))
            check1(pio);
        else
            check0(pio);
    }
}

