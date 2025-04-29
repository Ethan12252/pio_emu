#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

enum PioSource
{
    PINS = 0b000,
    X = 0b001,
    Y = 0b010,
    NULL_SRC = 0b011,
    RESERVED_4 = 0b100,
    RESERVED_5 = 0b101,
    ISR = 0b110,
    OSR = 0b111
};

uint16_t buildInInstruction(PioSource source, uint8_t bit_count)
{
    // IN encoding: 010 (opcode) | source (3 bits) | bit_count (5 bits)
    // If bit_count is 32, encode as 00000
    uint8_t encoded_bit_count = (bit_count == 32) ? 0 : bit_count;
    return (0b010 << 13) | (static_cast<uint8_t>(source) << 5) | (encoded_bit_count & 0x1F);
}

TEST_CASE("executeIn() test")
{
    PioStateMachine pio;

    SUBCASE("IN from PINS")
    {
        pio.settings.in_base = 5;
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.raw_data[i] = (i % 2); // 0, 1, 0, 1... patterns
        }

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false; // Shift left

        SUBCASE("in pins, 3")
        {
            // IN PINS, 3 should read pins 5, 6, 7
            pio.currentInstruction = buildInInstruction(PioSource::PINS, 3);
            pio.executeIn();

            // 101 (pins 5=1, 6=0, 7=1)
            CHECK(pio.regs.isr == 0b101);
            CHECK(pio.regs.isr_shift_count == 3);

            SUBCASE("in pins, +4") // Shift in 4 more bits
            {
                pio.currentInstruction = buildInInstruction(PioSource::PINS, 4);
                pio.executeIn();

                // 101 shifted left 4 places + 0101 (pins 5=1, 6=0, 7=1, 8=0)
                // Result: 0b1010101
                CHECK(pio.regs.isr == 0b1010101);
                CHECK(pio.regs.isr_shift_count == 7);
            }
        }
    }

    SUBCASE("IN from PINS with right shift")
    {
        pio.settings.in_base = 10;
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.raw_data[i] = (i % 2); // 0, 1... pattern
        }

        // w right shift
        pio.regs.isr = 0xF0000000;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = true;

        // IN PINS, 3
        pio.currentInstruction = buildInInstruction(PioSource::PINS, 3);
        pio.executeIn();

        // read from pins : 010( pins 10=0, 11=1, 12=0)
        // 0xF0000000 >> 3 = 0x1E000000, then 010 in high bits = 0x5E000000
        CHECK(pio.regs.isr == 0x5E000000);
        CHECK(pio.regs.isr_shift_count == 3);
    }

    SUBCASE("IN from X register")
    {
        pio.regs.x = 0xA5A5A5A5; // 0b10100101...
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false; // left shift

        SUBCASE("in x, 8")
        {
            // IN X, 8
            pio.currentInstruction = buildInInstruction(PioSource::X, 8);
            pio.executeIn();

            CHECK(pio.regs.isr == 0xA5);
            CHECK(pio.regs.isr_shift_count == 8);

            SUBCASE("in x, +4") // 4 more bits
            {
                pio.currentInstruction = buildInInstruction(PioSource::X, 4);
                pio.executeIn();

                // new isr = 0xA5A5A5A5
                // 0xA5 + 0x5
                CHECK(pio.regs.isr == 0xA55);
                CHECK(pio.regs.isr_shift_count == 12);
            }
        }
    }

    SUBCASE("IN from Y register")
    {
        pio.regs.y = 0x33CCFF00;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        // IN Y, 16
        pio.currentInstruction = buildInInstruction(PioSource::Y, 16);
        pio.executeIn();

        // lowest 16 bits of Y (0xFF00)
        CHECK(pio.regs.isr == 0xFF00);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("IN from NULL source")
    {
        pio.regs.isr = 0xABCDEF01;
        pio.regs.isr_shift_count = 8;
        pio.settings.in_shift_right = false;

        // IN NULL, 8 - should shift in 8 zero bits
        pio.currentInstruction = buildInInstruction(PioSource::NULL_SRC, 8);
        pio.executeIn();

        // 0xAB'CDEF'01 << 8 + 0 = 0xCDEF01'00
        CHECK(pio.regs.isr == 0xCDEF0100);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("IN from ISR")
    {
        pio.settings.in_base = 0;
        pio.regs.isr = 0x12345678;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false; // left shift

        // IN ISR, 8 should copy 8 bits from ISR back into itself
        pio.currentInstruction = buildInInstruction(PioSource::ISR, 8);
        pio.executeIn();

        // isr: (old_ISR << 8)(0x34567800) + low 8 bit of old_ISR (0x78)
        CHECK(pio.regs.isr == 0x34567878);
        CHECK(pio.regs.isr_shift_count == 8);
    }

    SUBCASE("IN from OSR")
    {
        pio.regs.osr = 0xFEDCBA98;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        // IN OSR, 16
        pio.currentInstruction = buildInInstruction(PioSource::OSR, 16);
        pio.executeIn();

        // low 16 bits of OSR (0xBA98)
        CHECK(pio.regs.isr == 0xBA98);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("IN with maximum bit count (32)")
    {
        pio.regs.x = 0xABCDEF01;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        // IN X, 32 - should copy all x
        pio.currentInstruction = buildInInstruction(PioSource::X, 32);
        pio.executeIn();

        CHECK(pio.regs.isr == 0xABCDEF01);
        CHECK(pio.regs.isr_shift_count == 32);
    }

    SUBCASE("IN with ISR shift count saturation")
    {
        // With shift count near max
        pio.regs.isr = 0x12345678;
        pio.regs.isr_shift_count = 30;
        pio.settings.in_shift_right = false;

        // Add 4 more bits - should saturate at 32
        pio.currentInstruction = buildInInstruction(PioSource::X, 4);
        pio.regs.x = 0xF;
        pio.executeIn();

        // The ISR shift count should be capped at 32
        CHECK(pio.regs.isr_shift_count == 32);
    }

    SUBCASE("IN with autopush enabled and threshold reached")
    {
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 8;
        pio.settings.in_shift_right = false;

        pio.rx_fifo_count = 0;

        pio.regs.isr = 0xAA;
        pio.regs.isr_shift_count = 7;

        // IN PINS, 2 - over threshold
        pio.settings.in_base = 0;
        pio.gpio.raw_data[0] = 1;
        pio.gpio.raw_data[1] = 1;
        pio.currentInstruction = buildInInstruction(PioSource::PINS, 2);
        pio.executeIn();

        // Should have pushed to RX FIFO and reset ISR
        CHECK(pio.rx_fifo_count == 1);
        CHECK(pio.rx_fifo[0] == 0x2AB); // 0xAA with 2 new bits = 0b0010'1010'1011
        CHECK(pio.regs.isr == 0); // ISR cleared after push
        CHECK(pio.regs.isr_shift_count == 0); // Shift count reset
    }

    SUBCASE("IN with exact threshold match")
    {
        // Enable autopush
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 8;

        pio.rx_fifo_count = 0;

        // Set up ISR with empty state
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;

        // IN X, 8 - exactly matches threshold
        pio.regs.x = 0xAB;
        pio.currentInstruction = buildInInstruction(PioSource::X, 8);
        pio.executeIn();

        // Should have pushed to RX FIFO and reset ISR
        CHECK(pio.rx_fifo_count == 1);
        CHECK(pio.rx_fifo[0] == 0xAB);
        CHECK(pio.regs.isr == 0);
        CHECK(pio.regs.isr_shift_count == 0);
    }

    SUBCASE("Reserved sources")
    {
        // sources 100 and 101 are reserved
        // should has no effect on ISR

        pio.regs.isr = 0x12345678;
        pio.regs.isr_shift_count = 10;

        SUBCASE("source 101")
        {
            // 100
            pio.currentInstruction = buildInInstruction(PioSource::RESERVED_4, 8);
            pio.executeIn();

            // unchanged
            CHECK(pio.regs.isr == 0x12345678);
            CHECK(pio.regs.isr_shift_count == 10);
        }


        SUBCASE("source 100")
        {
            // 101
            pio.currentInstruction = buildInInstruction(PioSource::RESERVED_5, 8);
            pio.executeIn();

            // unchanged
            CHECK(pio.regs.isr == 0x12345678);
            CHECK(pio.regs.isr_shift_count == 10);
        }
    }

    SUBCASE("IN from PINS with different base values")
    {
        // all pins to 1
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.raw_data[i] = i % 2 ? 0 : 1; // even i -> 1
        }

        // base 0
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 0;
        pio.settings.in_shift_right = false;
        pio.currentInstruction = buildInInstruction(PioSource::PINS, 4);
        pio.executeIn();
        CHECK(pio.regs.isr == 0b0101);

        // base 16
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 16;
        pio.currentInstruction = buildInInstruction(PioSource::PINS, 4);
        pio.executeIn();
        CHECK(pio.regs.isr == 0b0101);

        // base 23
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.raw_data[i] = 0;
        }
        pio.gpio.raw_data[24] = 1;
        pio.gpio.raw_data[25] = 1;

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 23;
        pio.currentInstruction = buildInInstruction(PioSource::PINS, 5);
        pio.executeIn();
        CHECK(pio.regs.isr == 0b00110);
    }

    SUBCASE("IN from PINS with wrap-around")
    {
        for (int i = 0; i < 32; i++)
        {
            pio.gpio.raw_data[i] = (i % 3 == 0) ? 1 : 0; // 1,0,0,1,0,0,...
        }

        // Test with base near the end of the pin range
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 30;
        pio.settings.in_shift_right = false;
        pio.currentInstruction = buildInInstruction(PioSource::PINS, 5);
        pio.executeIn();

        // Expected pin30: 1 pin1: 1 others: 0
        CHECK(pio.regs.isr == 0b00101);
    }

    SUBCASE("IN with RX FIFO full stalling")
    {
        // Enable autopush
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 8;

        // Fill the RX FIFO
        pio.rx_fifo_count = 4;
        for (int i = 0; i < 4; i++)
        {
            pio.rx_fifo[i] = 0xF0 + i;
        }

        pio.regs.isr = 0xAA;
        pio.regs.isr_shift_count = 4;

        // Perform IN operation that would trigger autopush
        pio.regs.x = 0xF;
        pio.currentInstruction = buildInInstruction(PioSource::X, 4);
        pio.executeIn();

        // Should set the stalling flag when FIFO is full
        CHECK(pio.push_is_stalling == true);

        // The ISR should still be updated even if push is stalled
        CHECK(pio.regs.isr == 0xAAF); // Shifted old value + new 4 bits
        CHECK(pio.regs.isr_shift_count == 8); // Count increased to threshold
    }

    SUBCASE("IN with single bit operations")
    {
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;
        pio.regs.x = 1;

        pio.currentInstruction = buildInInstruction(PioSource::X, 1);
        pio.executeIn();

        CHECK(pio.regs.isr == 1);
        CHECK(pio.regs.isr_shift_count == 1);

        // Do 8 times
        for (int i = 0; i < 7; i++)
        {
            pio.currentInstruction = buildInInstruction(PioSource::X, 1);
            pio.executeIn();
        }

        // Should have 8 bits all set to 1
        CHECK(pio.regs.isr == 0xFF);
        CHECK(pio.regs.isr_shift_count == 8);
    }

    SUBCASE("IN with multiple autopushes")
    {
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 4;

        pio.rx_fifo_count = 0;

        pio.regs.x = 0xF;

        // Multiple operations, each trigger an autopush
        for (int i = 0; i < 3; i++)
        {
            pio.regs.isr = 0;
            pio.regs.isr_shift_count = 0;
            pio.currentInstruction = buildInInstruction(PioSource::X, 4);
            pio.executeIn();

            // Each operation should trigger a push
            CHECK(pio.rx_fifo_count == i + 1);
            CHECK(pio.rx_fifo[i] == 0xF);
            CHECK(pio.regs.isr == 0);
            CHECK(pio.regs.isr_shift_count == 0);
        }
    }

    SUBCASE("IN with right shift and autopush")
    {
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 8;
        pio.settings.in_shift_right = true;

        pio.rx_fifo_count = 0;

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;

        // Input 8 bits to trigger autopush
        pio.regs.x = 0xAB;
        pio.currentInstruction = buildInInstruction(PioSource::X, 8);
        pio.executeIn();

        CHECK(pio.rx_fifo_count == 1);
        CHECK(pio.rx_fifo[0] == (0xAB << 24)); // 0xAB000000
        CHECK(pio.regs.isr == 0);
        CHECK(pio.regs.isr_shift_count == 0);
    }
}
