#include <fmt/format.h>
#include <cstdint>
#include <cassert>
//#include "TimingDiagram.h"
#include "PioStateMachine.h"

int main() {
    
    PioStateMachine pio;

    static const uint16_t ws2812_program_instructions[] = {
        //     .wrap_target
        0x6321, //  0: out    x, 1    side 0 [3] ; Side-set still takes place when instruction stalls
        0x1223, //  1: jmp    !x, 3   side 1 [2] ; Branch on the bit we shifted out. Positive pulse
        0x1200, //  2: jmp    0       side 1 [2] ; Continue driving high, for a long pulse
        0xa242, //  3: nop            side 0 [2] ; Or drive low, for a short pulse
        //     .wrap
    };

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
    pio.settings.warp_start = 0;
    pio.settings.warp_end = 3;

    pio.gpio.pindirs[22] = 0;   // pin22 pindir to output

    // data
    pio.tx_fifo[0] = 0xba'ab'ff'00;
    pio.tx_fifo[1] = 0xff'12'ff'00;
    pio.tx_fifo_count = 2;

    for (size_t i = 0; i < 24; i++)
        fmt::print("{:<2}: {:#010x}\n\n", i, pio.tx_fifo[0] << i);

    // tick
    for (int i = 0; i < 245 * 2; ++i) 
    {
        pio.tick(); // Update the emulator
        //fmt::print("{}", pio.gpio.raw_data[22] ? '-' : '_');
        fmt::print("clock: {:<3}, pin22: {}, pc: {}, osr: {:#010x} osr_count: {:<2}\n", pio.clock, pio.gpio.raw_data[22], pio.regs.pc, pio.regs.osr, pio.regs.osr_shift_count);
        if (!((pio.clock - 5) % 10))
        {
            static int ist = 1;
            fmt::print("data: {:<2}\n", ist++);
        }
    }

    return 0;}