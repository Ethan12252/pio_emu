#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

uint16_t buildOutInstruction(uint8_t destination, uint8_t bit_count)
{
    // OUT encoding: 0 1 1| Delay/side-set |Destination |Bit count
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

TEST_CASE("executeOut() basic functionality")
{
    PioStateMachine pio;

    SUBCASE("OSR to X register transfer")
    {
        pio.settings.out_shift_right = true;
        pio.regs.osr = 0xABCD1234;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;

        // OUT X, 16
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 16);
        pio.executeOut();

        CHECK(pio.regs.x == 0x1234);
        CHECK(pio.regs.osr_shift_count == 16);

        // shift right setting
        pio.regs.osr = 0xABCD1234;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        pio.currentInstruction = buildOutInstruction(OutDestination::X, 16);
        pio.executeOut();

        CHECK(pio.regs.x == 0x1234);
        CHECK(pio.regs.osr_shift_count == 16);

        // shift left setting
        pio.regs.osr = 0xABCD1234;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;
        pio.settings.out_shift_right = false;

        pio.currentInstruction = buildOutInstruction(OutDestination::X, 16);
        pio.executeOut();

        CHECK(pio.regs.x == 0xABCD);
        CHECK(pio.regs.osr_shift_count == 16);
    }

    SUBCASE("OSR to Y register transfer")
    {
        pio.regs.osr = 0x87654321;
        pio.regs.osr_shift_count = 0;
        pio.regs.y = 0;
        pio.settings.out_shift_right = true;

        // OUT Y, 8 
        pio.currentInstruction = buildOutInstruction(OutDestination::Y, 8);
        pio.executeOut();

        CHECK(pio.regs.y == 0x21);
        CHECK(pio.regs.osr_shift_count == 8);

        // Test left shift
        pio.regs.osr = 0x87654321;
        pio.regs.osr_shift_count = 0;
        pio.regs.y = 0;
        pio.settings.out_shift_right = false;

        pio.currentInstruction = buildOutInstruction(OutDestination::Y, 8);
        pio.executeOut();

        CHECK(pio.regs.y == 0x87);
        CHECK(pio.regs.osr_shift_count == 8);
    }

    SUBCASE("OSR to NULL (discard bits)")
    {
        pio.regs.osr = 0xFFFFFFFF;
        pio.settings.out_shift_right = true;
        pio.regs.osr_shift_count = 0;

        // OUT NULL, 12
        pio.currentInstruction = buildOutInstruction(OutDestination::NULL_DEST, 12);
        pio.executeOut();

        CHECK(pio.regs.osr == 0x000FFFFF);
        CHECK(pio.regs.osr_shift_count == 12);
    }

    SUBCASE("32-bit OUT transfer (encoded as 0)")
    {
        pio.regs.osr = 0xDEADBEEF;
        pio.regs.osr_shift_count = 0;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        // OUT X, 32 (encoded as 0)
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 0);
        pio.executeOut();

        // Should transfer all 32-bit value
        CHECK(pio.regs.x == 0xDEADBEEF);
        CHECK(pio.regs.osr_shift_count == 32);
    }

    SUBCASE("OSR shift count saturation")
    {
        pio.regs.osr = 0x12345678;
        pio.regs.osr_shift_count = 20;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        // OUT X, 16 
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 16);
        pio.executeOut();

        CHECK(pio.regs.x == 0x5678);
        CHECK(pio.regs.osr_shift_count == 32); // Saturated at 32, not 36
    }
}

TEST_CASE("executeOut() to PINS and PINDIRS")
{
    PioStateMachine pio;

    for (int i = 0; i < 32; i++)
    {
        pio.gpio.out_data[i] = -1;
        pio.gpio.pindirs[i] = -1;
    }

    SUBCASE("OUT to PINS")
    {
        pio.regs.osr = 0x0000001F; //0b11111
        pio.settings.out_base = 2;
        pio.settings.out_count = 5;
        pio.settings.out_shift_right = true;

        pio.currentInstruction = buildOutInstruction(OutDestination::PINS, 5);
        pio.executeOut();

        // Check if pins 2-6 have been set correctly with 11111
        CHECK(pio.gpio.out_data[2] == 1);
        CHECK(pio.gpio.out_data[3] == 1);
        CHECK(pio.gpio.out_data[4] == 1);
        CHECK(pio.gpio.out_data[5] == 1);
        CHECK(pio.gpio.out_data[6] == 1);
        CHECK(pio.gpio.out_data[1] == -1); // Unaffected
        CHECK(pio.gpio.out_data[7] == -1); // Unaffected

        // with different pattern
        pio.regs.osr = 0x0000000A; // 0b1010
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.out_data[i] = -1;
        }

        pio.currentInstruction = buildOutInstruction(OutDestination::PINS, 5);
        pio.executeOut();

        // pins 2-6 should be set with the 01010 (LSB to MSB)
        CHECK(pio.gpio.out_data[2] == 0);
        CHECK(pio.gpio.out_data[3] == 1);
        CHECK(pio.gpio.out_data[4] == 0);
        CHECK(pio.gpio.out_data[5] == 1);
        CHECK(pio.gpio.out_data[6] == 0);
    }

    SUBCASE("OUT to PINS with wrap-around")
    {
        pio.regs.osr = 0x0000001F; // 0b11111
        pio.settings.out_base = 30;
        pio.settings.out_count = 5;
        pio.settings.out_shift_right = true;

        for (int i = 0; i < 32; i++)
        {
            pio.gpio.out_data[i] = -1;
        }

        pio.currentInstruction = buildOutInstruction(OutDestination::PINS, 5);
        pio.executeOut();

        // Check if pins wrap around correctly
        CHECK(pio.gpio.out_data[30] == 1);
        CHECK(pio.gpio.out_data[31] == 1);
        CHECK(pio.gpio.out_data[0] == 1);
        CHECK(pio.gpio.out_data[1] == 1);
        CHECK(pio.gpio.out_data[2] == 1);

        // Check if other pins are unaffected
        for (int i = 3; i < 30; i++)
        {
            CHECK(pio.gpio.out_data[i] == -1);
        }
    }

    SUBCASE("OUT to PINDIRS")
    {
        pio.regs.osr = 0x0000000A; // 0b1010
        pio.settings.out_base = 5;
        pio.settings.out_count = 4;
        pio.settings.out_shift_right = true;

        for (int i = 0; i < 32; i++)
        {
            pio.gpio.pindirs[i] = -1;
        }

        pio.currentInstruction = buildOutInstruction(OutDestination::PINDIRS, 4);
        pio.executeOut();

        // Check if pindirs 5-8 have been set correctly with the pattern 1010
        CHECK(pio.gpio.pindirs[5] == 0);
        CHECK(pio.gpio.pindirs[6] == 1);
        CHECK(pio.gpio.pindirs[7] == 0);
        CHECK(pio.gpio.pindirs[8] == 1);
        CHECK(pio.gpio.pindirs[4] == -1); // Unaffected
        CHECK(pio.gpio.pindirs[9] == -1); // Unaffected
    }
}

TEST_CASE("executeOut() to ISR and PC")
{
    PioStateMachine pio;

    SUBCASE("OUT to ISR")
    {
        pio.regs.osr = 0xABCD1234;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.out_shift_right = true;

        pio.currentInstruction = buildOutInstruction(OutDestination::ISR, 16);
        pio.executeOut();


        CHECK(pio.regs.isr == 0x1234);
        CHECK(pio.regs.isr_shift_count == 16);

        // Test with shift left
        pio.regs.osr = 0xABCD1234;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.out_shift_right = false;

        pio.currentInstruction = buildOutInstruction(OutDestination::ISR, 16);
        pio.executeOut();

        CHECK(pio.regs.isr == 0xABCD);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("OUT to PC (unconditional jump)")
    {
        pio.regs.osr = 0x00000007; // Jump to address 7
        pio.regs.pc = 2;
        pio.settings.out_shift_right = true;

        pio.currentInstruction = buildOutInstruction(OutDestination::PC, 8);
        pio.executeOut();

        // Check if jmp_to is set properly for the next instruction
        CHECK(pio.jmp_to == 7);
    }
}

TEST_CASE("executeOut() with autopull")
{
    PioStateMachine pio;

    SUBCASE("Autopull threshold not reached")
    {
        // autopull but don't reach threshold
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 24;
        pio.regs.osr = 0xABCDEF12;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        pio.tx_fifo[0] = 0x87654321;
        pio.tx_fifo_count = 1;

        // OUT X, 8 (shift count will be 16, below threshold)
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 8);
        pio.executeOut();

        // no autopull should happened
        CHECK(pio.regs.x == 0x12); // 8 lsb
        CHECK(pio.regs.osr_shift_count == 16); // 8+8
        CHECK(pio.regs.osr == 0x00ABCDEF);
        CHECK(pio.tx_fifo_count == 1); // FIFO still contains the value
    }

    SUBCASE("Autopull threshold exactly reached")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 16;
        pio.regs.osr = 0x12345678;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        // always pull from tx_fifo[0]
        pio.tx_fifo[0] = 0x87654321;
        pio.tx_fifo_count = 1;

        // OUT X, 8 (shift count will exactly be 16)
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 8);
        pio.executeOut();
        pio.executeOut(); // ** auto pull will cause one more cycle (datasheet p.339)

        CHECK(pio.regs.x == 0x78);
        CHECK(pio.regs.osr_shift_count == 0); // Reset to 0
        CHECK(pio.regs.osr == 0x87654321); // OSR with new value
        CHECK(pio.tx_fifo_count == 0); // FIFO now empty
    }

    SUBCASE("Autopull threshold exceeded")
    {
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 12;
        pio.regs.osr = 0xAAAAAAAA;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        pio.tx_fifo[0] = 0xBBBBBBBB;
        pio.tx_fifo[1] = 0xAAAAAAAA;
        pio.tx_fifo_count = 2;

        // OUT X, 8 (shift count will exceed 12)
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 8); // 8+8 > 12 -> autopush
        pio.executeOut();
        pio.executeOut(); // ** auto pull will cause one more cycle (datasheet p.339)

        CHECK(pio.regs.x == 0xAA);
        CHECK(pio.regs.osr_shift_count == 0);
        CHECK(pio.regs.osr == 0xBBBBBBBB);
        CHECK(pio.tx_fifo[0] == 0xAAAAAAAA); // Also check if tx_fifo shifted
        CHECK(pio.tx_fifo_count == 1);
    }

    SUBCASE("Autopull with empty TX FIFO")
    {
        // with empty FIFO (should stall)
        pio.settings.autopull_enable = true;
        pio.settings.pull_threshold = 16;
        pio.regs.osr = 0x12345678;
        pio.regs.osr_shift_count = 8;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;
        pio.tx_fifo_count = 0; // Empty FIFO

        // OUT X, 8 (shift count reaches threshold but FIFO empty)
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 8);
        pio.executeOut();
        pio.executeOut(); // ** auto pull will cause one more cycle (datasheet p.339)

        // Check stall
        CHECK(pio.regs.x == 0x78); // Value still output
        CHECK(pio.regs.osr_shift_count == 16); // Count still increase
        CHECK(pio.pull_is_stalling == true);
    }
}

TEST_CASE("executeOut() to EXEC")
{
    PioStateMachine pio;

    SUBCASE("Basic EXEC functionality")
    {
        // Setup OSR with SET X, 10 instruction bit pattern
        uint32_t set_instruction = (0b111 << 13) | (0b001 << 5) | 10; // SET X, 10
        pio.regs.osr = set_instruction;
        pio.settings.out_shift_right = true;

        // OUT EXEC, 16 (shift count 16 to get the whole instruction)
        pio.currentInstruction = buildOutInstruction(OutDestination::EXEC, 16);
        pio.executeOut();

        // Check if the next instruction is in currentInstruction ,and EXEC flag set
        CHECK(pio.currentInstruction == ((0b111 << 13) | (0b001 << 5) | 10)); // SET X, 10
        CHECK(pio.exec_command == true);
        CHECK(pio.skip_delay == true); // Should skip delay on initial OUT
    }
    SUBCASE("Bitcount should not affect OUT EXEC")
    {
        // Setup OSR with SET X, 10 instruction bit pattern
        uint32_t set_instruction = (0b111 << 13) | (0b001 << 5) | 10; // SET X, 10
        pio.regs.osr = set_instruction;
        pio.settings.out_shift_right = true;

        // OUT EXEC, 30
        pio.currentInstruction = buildOutInstruction(OutDestination::EXEC, 16);
        pio.executeOut();

        // Check if the next instruction is in currentInstruction ,and EXEC flag set
        CHECK(pio.currentInstruction == ((0b111 << 13) | (0b001 << 5) | 10)); // SET X, 10
        CHECK(pio.exec_command == true);
        CHECK(pio.skip_delay == true); // Should skip delay on initial OUT
    }
}

TEST_CASE("executeOut() with different bit counts")
{
    PioStateMachine pio;

    SUBCASE("Partial bit transfers")
    {
        pio.regs.osr = 0xFFFFFFFF;
        pio.regs.x = 0;
        pio.settings.out_shift_right = true;

        // OUT X, 1 (just 1 bit)
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 1);
        pio.executeOut();

        CHECK(pio.regs.x == 0x1);
        CHECK(pio.regs.osr_shift_count == 1);

        // OUT X, 4 (4 bits)
        pio.regs.osr = 0xFFFFFFFF;
        pio.regs.x = 0;
        pio.regs.osr_shift_count = 0;
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 4);
        pio.executeOut();

        CHECK(pio.regs.x == 0xF); // 4 bits set
        CHECK(pio.regs.osr_shift_count == 4);

        // OUT X, 31 (31 bits)
        pio.regs.osr = 0xFFFFFFFF;
        pio.regs.x = 0;
        pio.regs.osr_shift_count = 0;
        pio.currentInstruction = buildOutInstruction(OutDestination::X, 31);
        pio.executeOut();

        CHECK(pio.regs.x == 0x7FFFFFFF); // 31 bits set
        CHECK(pio.regs.osr_shift_count == 31);
    }
}
