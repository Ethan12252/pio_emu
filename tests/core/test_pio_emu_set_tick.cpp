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

TEST_CASE("SET PINS Instructions")
{
    PioStateMachine pio;

    // Initialize pins to -1 (unset)
    for (int i = 0; i < 32; i++)
    {
        pio.gpio.set_data[i] = -1;
        pio.gpio.raw_data[i] = 0;
        pio.gpio.out_data[i] = -1;
        pio.gpio.set_data[i] = -1;
        pio.gpio.sideset_data[i] = -1;
        pio.gpio.external_data[i] = -1;

    }

    SUBCASE("Basic PINS operation")
    {
        pio.settings.set_base = 0;
        pio.settings.set_count = 5;
        for (int i = 0; i < 5; i++)
            pio.gpio.sideset_pindirs[i] = 0;

        unsigned int inst = buildSetInstruction(PioDestination::PINS, 5); // 0b00101
        pio.instructionMemory[0] = inst;
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.currentInstruction == inst);

        CHECK(pio.gpio.set_data[0] == 1);
        CHECK(pio.gpio.set_data[1] == 0);
        CHECK(pio.gpio.set_data[2] == 1);
        CHECK(pio.gpio.set_data[3] == 0);
        CHECK(pio.gpio.set_data[4] == 0);
        for (int i = 5; i < 32; i++)
        {
            CHECK(pio.gpio.set_data[i] == -1); // check other pins are unaffected
        }
    }

    SUBCASE("PINS with custom base")
    {
        pio.settings.set_base = 5;
        pio.settings.set_count = 3;

        for (int i = pio.settings.set_base; i < ((pio.settings.set_base + pio.settings.set_count) % 32); i++)
            pio.gpio.sideset_pindirs[i] = 0;

        unsigned int inst = buildSetInstruction(PioDestination::PINS, 6);
        pio.instructionMemory[0] = inst;
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.gpio.set_data[5] == 0);
        CHECK(pio.gpio.set_data[6] == 1);
        CHECK(pio.gpio.set_data[7] == 1);
        CHECK(pio.gpio.set_data[4] == -1);
        CHECK(pio.gpio.set_data[8] == -1);
    }

    SUBCASE("PINS with high base")
    {
        pio.settings.set_base = 16;
        pio.settings.set_count = 5;

        for (int i = pio.settings.set_base; i < ((pio.settings.set_base + pio.settings.set_count) % 32); i++)
            pio.gpio.sideset_pindirs[i] = 0;

        unsigned int inst = buildSetInstruction(PioDestination::PINS, 21);
        pio.instructionMemory[0] = inst;
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.gpio.set_data[16] == 1);
        CHECK(pio.gpio.set_data[17] == 0);
        CHECK(pio.gpio.set_data[18] == 1);
        CHECK(pio.gpio.set_data[19] == 0);
        CHECK(pio.gpio.set_data[20] == 1);
        CHECK(pio.gpio.set_data[15] == -1);
        CHECK(pio.gpio.set_data[21] == -1);
    }

    SUBCASE("PINS with wrap-around")
    {
        pio.settings.set_base = 30;
        pio.settings.set_count = 5;

        for (int i = pio.settings.set_base; i < ((pio.settings.set_base + pio.settings.set_count) % 32); i++)
            pio.gpio.sideset_pindirs[i] = 0;

        unsigned int inst = buildSetInstruction(PioDestination::PINS, 31);
        pio.instructionMemory[0] = inst;
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.gpio.set_data[30] == 1);
        CHECK(pio.gpio.set_data[31] == 1);
        CHECK(pio.gpio.set_data[0] == 1);
        CHECK(pio.gpio.set_data[1] == 1);
        CHECK(pio.gpio.set_data[2] == 1);
        CHECK(pio.gpio.set_data[29] == -1);
    }

    SUBCASE("PINS with zero count")
    {
        pio.settings.set_base = 10;
        pio.settings.set_count = 0;

        for (int i = pio.settings.set_base; i < ((pio.settings.set_base + pio.settings.set_count) % 32); i++)
            pio.gpio.sideset_pindirs[i] = 0;

        unsigned int inst = buildSetInstruction(PioDestination::PINS, 15);
        pio.instructionMemory[0] = inst;
        pio.regs.pc = 0;
        pio.tick();
        for (int i = 0; i < 32; i++)
        {
            CHECK(pio.gpio.set_data[i] == -1);
        }
    }

}

# if 1
TEST_CASE("SET X Y")
{
    PioStateMachine pio;
        
    // Initialize pins to -1 (unset)
    for (int i = 0; i < 32; i++)
    {
        pio.gpio.set_data[i] = -1;
        pio.gpio.raw_data[i] = 0;
        pio.gpio.out_data[i] = -1;
        pio.gpio.set_data[i] = -1;
        pio.gpio.sideset_data[i] = -1;
        pio.gpio.external_data[i] = -1;

    }

    SUBCASE("SET X Register")
    {
        // Set X to a non-zero value to confirm it's properly cleared
        pio.regs.x = 0xFFFFFFFF;
        pio.currentInstruction = buildSetInstruction(PioDestination::X, 15);
        pio.executeSet(); 
        CHECK(pio.regs.x == 15); // Only 5 LSBs set, others cleared

        // Test max 5-bit value
        pio.regs.x = 0xFFFFFFFF;
        pio.currentInstruction = buildSetInstruction(PioDestination::X, 31);
        pio.executeSet();
        CHECK(pio.regs.x == 31);

        // Test value masking to 5 bits (0x1F)
        pio.regs.x = 0xFFFFFFFF;
        pio.currentInstruction = buildSetInstruction(PioDestination::X, 0xFF);
        pio.executeSet();
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

TEST_CASE("SET PINDIRS Instructions")
{
    PioStateMachine pio;

    for (int i = 0; i < 32; i++)
    {
        pio.gpio.set_pindirs[i] = 0;
    }

    SUBCASE("Basic PINDIRS operation")
    {
        pio.settings.set_base = 10;
        pio.settings.set_count = 4;

        pio.currentInstruction = buildSetInstruction(PioDestination::PINDIRS, 3);
        pio.executeSet();

        CHECK(pio.gpio.set_pindirs[10] == 1);
        CHECK(pio.gpio.set_pindirs[11] == 1);
        CHECK(pio.gpio.set_pindirs[12] == 0);
        CHECK(pio.gpio.set_pindirs[13] == 0);
        CHECK(pio.gpio.set_pindirs[9] == 0);
        CHECK(pio.gpio.set_pindirs[14] == 0);
    }

    SUBCASE("PINDIRS with high base")
    {
        pio.settings.set_base = 24;
        pio.settings.set_count = 8;

        pio.currentInstruction = buildSetInstruction(PioDestination::PINDIRS, 15);
        pio.executeSet();

        CHECK(pio.gpio.set_pindirs[24] == 1);
        CHECK(pio.gpio.set_pindirs[25] == 1);
        CHECK(pio.gpio.set_pindirs[26] == 1);
        CHECK(pio.gpio.set_pindirs[27] == 1);
        CHECK(pio.gpio.set_pindirs[28] == 0);
        CHECK(pio.gpio.set_pindirs[29] == 0);
        CHECK(pio.gpio.set_pindirs[30] == 0);
        CHECK(pio.gpio.set_pindirs[31] == 0);
        CHECK(pio.gpio.set_pindirs[23] == 0);
    }

    SUBCASE("PINDIRS with wrap-around")
    {
        pio.settings.set_base = 30;
        pio.settings.set_count = 4;

        pio.currentInstruction = buildSetInstruction(PioDestination::PINDIRS, 7);
        pio.executeSet();

        CHECK(pio.gpio.set_pindirs[30] == 1);
        CHECK(pio.gpio.set_pindirs[31] == 1);
        CHECK(pio.gpio.set_pindirs[0] == 1);
        CHECK(pio.gpio.set_pindirs[1] == 0);
        CHECK(pio.gpio.set_pindirs[2] == 0);
    }
}

TEST_CASE("Reserved destinations")
{
    PioStateMachine pio;
    // Testing reserved destinations should have no effect
    pio.regs.x = 0;
    pio.regs.y = 0;

    SUBCASE("Reserved destination 011")
    {
        // Reserved destination 0b011
        pio.currentInstruction = buildSetInstruction(PioDestination::RESERVED_3, 15);
        pio.executeSet();
        CHECK(pio.regs.x == 0); // unchanged
        CHECK(pio.regs.y == 0);
    }


    SUBCASE("Reserved destination 101")
    {
        pio.currentInstruction = buildSetInstruction(PioDestination::RESERVED_5, 15);
        pio.executeSet();
        CHECK(pio.regs.x == 0);
        CHECK(pio.regs.y == 0);
    }


    SUBCASE("Reserved destination 110")
    {
        pio.currentInstruction = buildSetInstruction(PioDestination::RESERVED_6, 15);
        pio.executeSet();
        CHECK(pio.regs.x == 0);
        CHECK(pio.regs.y == 0);
    }


    SUBCASE("Reserved destination 111")
    {
        pio.currentInstruction = buildSetInstruction(PioDestination::RESERVED_7, 15);
        pio.executeSet();
        CHECK(pio.regs.x == 0);
        CHECK(pio.regs.y == 0);
    }
}
#endif