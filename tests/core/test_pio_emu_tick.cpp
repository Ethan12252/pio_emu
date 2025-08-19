#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

enum PioDestination
{
    PINS = 0b000,
    X = 0b001,
    Y = 0b010,
    RESERVED_3 = 0b011,
    PINDIRS = 0b100,
    RESERVED_5 = 0b101,
    RESERVED_6 = 0b110,
    RESERVED_7 = 0b111
};

uint16_t buildSetInstruction(PioDestination destination, uint8_t value)
{
    return (0b111 << 13) | (static_cast<uint8_t>(destination) << 5) | (value & 0x1F);
}


TEST_CASE("SET instruction only")
{
    PioStateMachine pio;

    SUBCASE("SET XY Register")
    {
        /* Build Instructions */

        pio.instructionMemory[0] = buildSetInstruction(PioDestination::X, 15);
        pio.instructionMemory[1] = buildSetInstruction(PioDestination::X, 31);
        pio.instructionMemory[2] = buildSetInstruction(PioDestination::X, 0xFF);

        pio.regs.pc = 0;
        
        // 1: Set X to a non-zero value to confirm it's properly cleared
        pio.regs.x = 0xFFFFFFFF;
        pio.tick();
        CHECK(pio.regs.x == 15); // Only 5 LSBs set, others cleared

        // 2: Test max 5-bit value
        pio.regs.x = 0xFFFFFFFF;
        pio.tick();
        CHECK(pio.regs.x == 31);

        // 3: Test value masking to 5 bits (0x1F)
        pio.regs.x = 0xFFFFFFFF;
        pio.tick();
        CHECK(pio.regs.x == 31); // 0xFF & 0x1F = 31
    }

    SUBCASE("SET Y Register")
    {
        // Set Y to a non-zero value to confirm it's properly cleared
        pio.regs.y = 0xFFFFFFFF;
        pio.currentInstruction = buildSetInstruction(PioDestination::Y, 15);
        pio.executeSet();
        CHECK(pio.regs.y == 15); // Only 5 LSBs set, others cleared

        // Test max 5-bit value
        pio.regs.y = 0xFFFFFFFF;
        pio.currentInstruction = buildSetInstruction(PioDestination::Y, 31);
        pio.executeSet();
        CHECK(pio.regs.y == 31);

        // Test value masking to 5 bits
        pio.regs.y = 0xFFFFFFFF;
        pio.currentInstruction = buildSetInstruction(PioDestination::Y, 0xFF);
        pio.executeSet();
        CHECK(pio.regs.y == 31); // 0xFF & 0x1F = 31
    }
}


