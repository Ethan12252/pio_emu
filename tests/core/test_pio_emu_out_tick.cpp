#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

uint16_t buildOutInstruction(uint8_t destination, uint8_t bit_count)
{
    uint8_t encoded_bit_count = bit_count == 32 ? 0 : bit_count;
    return (0b011 << 13) | (destination << 5) | encoded_bit_count;
}

enum OutDestination
{
    PINS = 0b000,
    X = 0b001,
    Y = 0b010,
    NULL_DEST = 0b011,
    PINDIRS = 0b100,
    PC = 0b101,
    ISR = 0b110,
    EXEC = 0b111
};

// OSR to X register transfer
TEST_CASE("OUT: OSR to X register transfer (shift right and left)")
{
    PioStateMachine pio;

    SUBCASE("Shift right")
    {
        pio.settings.out_shift_right = true;
        pio.regs.osr = 0xAB'CD'12'34;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 16);
        pio.tick();

        CHECK(pio.regs.x == 0x1234);
        CHECK(pio.regs.osr_shift_count == 16);
        CHECK(pio.regs.osr == 0xABCD);
    }

    SUBCASE("Shift left")
    {
        pio.settings.out_shift_right = false;
        pio.regs.osr = 0xAB'CD'12'34;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;
        pio.regs.pc = 0;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 16);

        pio.tick();

        CHECK(pio.regs.x == 0xABCD);
        CHECK(pio.regs.osr_shift_count == 16);
    }
}

// OSR to Y register transfer
TEST_CASE("OUT: OSR to Y register transfer (shift right and left)")
{
    PioStateMachine pio;

    SUBCASE("Shift right")
    {
        pio.regs.osr = 0x87'65'43'21;
        pio.regs.osr_shift_count = 0;
        pio.regs.y = 0;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::Y, 8);
        pio.tick();

        CHECK(pio.regs.y == 0x21);
        CHECK(pio.regs.osr_shift_count == 8);
    }

    SUBCASE("Shift left")
    {
        pio.regs.osr = 0x87654321;
        pio.regs.osr_shift_count = 0;
        pio.regs.y = 0;
        pio.settings.out_shift_right = false;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::Y, 8);
        pio.tick();

        CHECK(pio.regs.y == 0x87);
        CHECK(pio.regs.osr_shift_count == 8);
    }
}

// OSR to NULL (discard bits)
TEST_CASE("OUT: OSR to NULL (discard bits)")
{
    PioStateMachine pio;
    pio.regs.osr = 0xFFFFFFFF;
    pio.settings.out_shift_right = true;
    pio.regs.osr_shift_count = 0;
    pio.instructionMemory[0] = buildOutInstruction(OutDestination::NULL_DEST, 12);
    pio.tick();

    CHECK(pio.regs.osr == 0x000FFFFF);
    CHECK(pio.regs.osr_shift_count == 12);
}

// 32-bit OUT transfer (encoded as 0)
TEST_CASE("OUT: 32-bit transfer (encoded as 0)")
{
    PioStateMachine pio;
    pio.regs.osr = 0xDEADBEEF;
    pio.regs.osr_shift_count = 0;
    pio.regs.x = 0;
    pio.settings.out_shift_right = true;
    pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 0);
    pio.tick();

    CHECK(pio.regs.x == 0xDEADBEEF);
    CHECK(pio.regs.osr_shift_count == 32);
}

// OSR shift count saturation
TEST_CASE("OUT: OSR shift count saturation")
{
    PioStateMachine pio;
    pio.regs.osr = 0x12345678;
    pio.regs.osr_shift_count = 20;
    pio.regs.x = 0;
    pio.settings.out_shift_right = true;
    pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 16);
    pio.tick();

    CHECK(pio.regs.x == 0x5678);
    CHECK(pio.regs.osr_shift_count == 32);
}

// OUT to PINS
TEST_CASE("OUT: to PINS")
{
    PioStateMachine pio;
    for (int i = 0; i < 32; i++)
    {
        pio.gpio.out_data[i] = -1;
    }

    SUBCASE("Set pins 2-6 with 11111")
    {
        pio.regs.osr = 0x0000001F;
        pio.settings.out_base = 2;
        pio.settings.out_count = 5;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::PINS, 5);
        pio.tick();

        CHECK(pio.gpio.out_data[2] == 1);
        CHECK(pio.gpio.out_data[3] == 1);
        CHECK(pio.gpio.out_data[4] == 1);
        CHECK(pio.gpio.out_data[5] == 1);
        CHECK(pio.gpio.out_data[6] == 1);
        CHECK(pio.gpio.out_data[1] == -1);
        CHECK(pio.gpio.out_data[7] == -1);
    }

    SUBCASE("Set pins 2-6 with 01010")
    {
        pio.regs.osr = 0x0000000A;
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.out_data[i] = -1;
        }
        pio.settings.out_base = 2;
        pio.settings.out_count = 5;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::PINS, 5);
        pio.tick();

        CHECK(pio.gpio.out_data[2] == 0);
        CHECK(pio.gpio.out_data[3] == 1);
        CHECK(pio.gpio.out_data[4] == 0);
        CHECK(pio.gpio.out_data[5] == 1);
        CHECK(pio.gpio.out_data[6] == 0);
    }
}

// OUT to PINS with wrap-around
TEST_CASE("OUT: to PINS with wrap-around")
{
    PioStateMachine pio;
    pio.regs.osr = 0x0000001F;
    pio.settings.out_base = 30;
    pio.settings.out_count = 5;
    pio.settings.out_shift_right = true;
    for (int i = 0; i < 32; i++)
    {
        pio.gpio.out_data[i] = -1;
    }
    pio.instructionMemory[0] = buildOutInstruction(OutDestination::PINS, 5);
    pio.tick();

    CHECK(pio.gpio.out_data[30] == 1);
    CHECK(pio.gpio.out_data[31] == 1);
    CHECK(pio.gpio.out_data[0] == 1);
    CHECK(pio.gpio.out_data[1] == 1);
    CHECK(pio.gpio.out_data[2] == 1);

    for (int i = 3; i < 30; i++)
    {
        CHECK(pio.gpio.out_data[i] == -1);
    }
}

// OUT to PINDIRS
TEST_CASE("OUT: to PINDIRS")
{
    PioStateMachine pio;
    pio.regs.osr = 0x0000000A;
    pio.settings.out_base = 5;
    pio.settings.out_count = 4;
    pio.settings.out_shift_right = true;
    for (int i = 0; i < 32; i++)
    {
        pio.gpio.out_pindirs[i] = -1;
    }
    pio.instructionMemory[0] = buildOutInstruction(OutDestination::PINDIRS, 4);
    pio.tick();

    CHECK(pio.gpio.out_pindirs[5] == 0);
    CHECK(pio.gpio.out_pindirs[6] == 1);
    CHECK(pio.gpio.out_pindirs[7] == 0);
    CHECK(pio.gpio.out_pindirs[8] == 1);
    CHECK(pio.gpio.out_pindirs[4] == -1);
    CHECK(pio.gpio.out_pindirs[9] == -1);
}

// OUT to ISR
TEST_CASE("OUT: to ISR (shift right and left)")
{
    PioStateMachine pio;

    SUBCASE("Shift right")
    {
        pio.regs.osr = 0xABCD1234;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::ISR, 16);
        pio.tick();

        CHECK(pio.regs.isr == 0x1234);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("Shift left")
    {
        pio.regs.osr = 0xABCD1234;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.out_shift_right = false;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::ISR, 16);
        pio.tick();

        CHECK(pio.regs.isr == 0xABCD);
        CHECK(pio.regs.isr_shift_count == 16);
    }
}

// OUT to PC (unconditional jump)
TEST_CASE("OUT: to PC (unconditional jump)")
{
    PioStateMachine pio;
    pio.regs.osr = 0x00000007;
    pio.regs.pc = 2;
    pio.settings.out_shift_right = true;
    pio.instructionMemory[2] = buildOutInstruction(OutDestination::PC, 8);
    pio.tick();

    CHECK(pio.regs.pc == 7);
}

// OUT with autopull
TEST_CASE("OUT: with autopull")
{
    PioStateMachine pio;

    SUBCASE("Autopull threshold not reached")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 24;
        pio.regs.osr = 0x00ABCDEF;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.tx_fifo[0] = 0x87654321;
        pio.tx_fifo_count = 1;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 8);
        pio.instructionMemory[1] = 0xa042; // nop
        pio.tick();
        CHECK(pio.regs.pc == 1); // pc should increase normally
        pio.tick();

        CHECK(pio.regs.x == 0xEF);
        CHECK(pio.regs.osr_shift_count == 16);
        CHECK(pio.regs.osr == 0x0000ABCD);
        CHECK(pio.tx_fifo_count == 1);
    }

    SUBCASE("Autopull threshold exactly reached")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 16;
        pio.regs.osr = 0x000000ab;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.tx_fifo[0] = 0x12345678;
        pio.tx_fifo_count = 1;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 8);
        pio.tick();
        CHECK(pio.regs.pc == 0); // should spend one more cycle
        pio.tick();

        CHECK(pio.regs.x == 0xab);
        CHECK(pio.regs.osr_shift_count == 0); // nothing shifted
        CHECK(pio.regs.osr == 0x12345678);    // should be the new data
        CHECK(pio.tx_fifo_count == 0);        // be taken out
    }

    SUBCASE("Autopull threshold exactly reached2")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 32;
        pio.regs.osr = 0x0000dead;
        pio.regs.osr_shift_count = 16;
        pio.regs.x = 0xffffffff;
        pio.settings.out_shift_right = true;
        pio.tx_fifo[0] = 0x12345678;
        pio.tx_fifo_count = 1;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 16);
        pio.tick();
        CHECK(pio.regs.pc == 0); // should spend one more cycle
        pio.tick();

        CHECK(pio.regs.x == 0x0000dead);      // TODO: check should it clear x?
        CHECK(pio.regs.osr_shift_count == 0); // nothing shifted
        CHECK(pio.regs.osr == 0x12345678);    // should be the new data
        CHECK(pio.tx_fifo_count == 0);        // be taken out
    }

    SUBCASE("Autopull threshold exceeded")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 12;
        pio.regs.osr = 0x0000000A;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.tx_fifo[0] = 0x00000BBB;
        pio.tx_fifo[1] = 0x00000fff;
        pio.tx_fifo_count = 2;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 8); // out x, 8
        pio.tick();
        CHECK(pio.regs.pc == 0); // should spend one more cycle
        pio.tick();

        CHECK(pio.regs.x == 0xAB);
        CHECK(pio.regs.osr_shift_count == 4);
        CHECK(pio.regs.osr == 0x000000BB);
        CHECK(pio.tx_fifo[0] == 0x00000fff);
        CHECK(pio.tx_fifo_count == 1);
    }

    SUBCASE("Autopull threshold exceeded2")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 20;
        pio.regs.osr = 0b1010'1011;
        pio.regs.osr_shift_count = 12;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.tx_fifo[0] = 0b0001'0010'0011'0100'0101'0110'0111'1000;
        pio.tx_fifo[1] = 0x87654321;
        pio.tx_fifo_count = 2;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 18); // out x, 18
        pio.tick();
        CHECK(pio.regs.pc == 0); // should spend one more cycle
        pio.tick();

        CHECK(pio.regs.x == 0b1010'1011'10'0111'1000);
        CHECK(pio.regs.osr_shift_count == 10);
        CHECK(pio.regs.osr == 0b0001'0010'0011'0100'0101'01);
        CHECK(pio.tx_fifo[0] == 0x87654321);
        CHECK(pio.tx_fifo_count == 1);
    }

    SUBCASE("Autopull threshold exceeded3")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 32;
        pio.regs.osr = 0x0deadbee;
        pio.regs.osr_shift_count = 4;
        pio.regs.x = 0xffffffff;
        pio.settings.out_shift_right = true;
        pio.tx_fifo[0] = 0xABCEEFBD;
        pio.tx_fifo[1] = 0xffffffff;
        pio.tx_fifo[2] = 0xf12356ff;
        pio.tx_fifo_count = 3;

        pio.regs.pc = 0;
        pio.instructionMemory[0] = 0x6020;
        pio.tick();
        CHECK(pio.regs.pc == 0); // should spend one more cycle
        pio.tick();

        CHECK(pio.regs.x == 0xdeadbeed);
        CHECK(pio.regs.osr_shift_count == 4);
        CHECK(pio.regs.osr == 0x0ABCEEFB);
        CHECK(pio.tx_fifo[0] == 0xffffffff);
        CHECK(pio.tx_fifo[1] == 0xf12356ff);
        CHECK(pio.tx_fifo_count == 2);
    }

    SUBCASE("Autopull with empty TX FIFO")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 16;
        pio.regs.osr = 0x00'00'00'78;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.tx_fifo_count = 0;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 8);
        pio.instructionMemory[1] = buildOutInstruction(OutDestination::X, 8);
        pio.tick();
        CHECK(pio.regs.pc == 0); // should spend one more cycle
        pio.tick();

        CHECK(pio.regs.x == 0x78);
        CHECK(pio.regs.osr_shift_count == 16);
        CHECK(pio.pull_is_stalling == true);
    }
}

// OUT to EXEC
TEST_CASE("OUT: to EXEC")
{
    PioStateMachine pio;

    SUBCASE("Basic EXEC functionality")
    {
        uint32_t set_instruction = (0b111 << 13) | (0b001 << 5) | 10;
        pio.regs.osr = set_instruction;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::EXEC, 16);
        pio.tick();

        CHECK(pio.currentInstruction == set_instruction);
        CHECK(pio.exec_command == true);
        CHECK(pio.skip_delay == true);
    }

    SUBCASE("Bitcount should not affect OUT EXEC")
    {
        uint32_t set_instruction = (0b111 << 13) | (0b001 << 5) | 10;
        pio.regs.osr = set_instruction;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::EXEC, 16);
        pio.tick();

        CHECK(pio.currentInstruction == set_instruction);
        CHECK(pio.exec_command == true);
        CHECK(pio.skip_delay == true);
    }
}

// OUT with different bit counts
TEST_CASE("OUT: with different bit counts")
{
    PioStateMachine pio;

    SUBCASE("Partial bit transfers")
    {
        pio.regs.osr = 0xFFFFFFFF;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.instructionMemory[0] = buildOutInstruction(OutDestination::X, 1);
        pio.tick();

        CHECK(pio.regs.x == 0x1);
        CHECK(pio.regs.osr_shift_count == 1);

        pio.regs.osr = 0xFFFFFFFF;
        pio.regs.x = 0;
        pio.regs.osr_shift_count = 0;
        pio.instructionMemory[1] = buildOutInstruction(OutDestination::X, 4);
        pio.tick();

        CHECK(pio.regs.x == 0xF);
        CHECK(pio.regs.osr_shift_count == 4);

        pio.regs.osr = 0xFFFFFFFF;
        pio.regs.x = 0;
        pio.regs.osr_shift_count = 0;
        pio.instructionMemory[2] = buildOutInstruction(OutDestination::X, 31);
        pio.tick();

        CHECK(pio.regs.x == 0x7FFFFFFF);
        CHECK(pio.regs.osr_shift_count == 31);
    }
}
