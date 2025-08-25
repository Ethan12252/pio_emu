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
    uint8_t encoded_bit_count = (bit_count == 32) ? 0 : bit_count;
    return (0b010 << 13) | (static_cast<uint8_t>(source) << 5) | (encoded_bit_count & 0x1F);
}

TEST_CASE("PIO IN instruction via tick()")
{
    PioStateMachine pio;

    SUBCASE("IN from PINS")
    {
        pio.settings.in_base = 5;
        for (int i = 0; i < 32; i++)
            pio.gpio.raw_data[i] = (i % 2); // 0, 1, 0, 1... patterns

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        SUBCASE("in pins, 3")
        {
            pio.instructionMemory[0] = buildInInstruction(PINS, 3);
            pio.regs.pc = 0;
            pio.tick();

            CHECK(pio.regs.isr == 0b101);
            CHECK(pio.regs.isr_shift_count == 3);

            SUBCASE("in pins, +4")
            {
                pio.instructionMemory[1] = buildInInstruction(PINS, 4);
                pio.regs.pc = 1;
                pio.tick();

                CHECK(pio.regs.isr == 0b1010101);
                CHECK(pio.regs.isr_shift_count == 7);
            }
        }
    }

    SUBCASE("IN from PINS with right shift")
    {
        pio.settings.in_base = 10;
        for (int i = 0; i < 32; i++)
            pio.gpio.raw_data[i] = (i % 2);// 0, 1... pattern

        pio.regs.isr = 0xF0000000;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = true;

        pio.instructionMemory[0] = buildInInstruction(PINS, 3);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0x5E000000);
        CHECK(pio.regs.isr_shift_count == 3);
    }

    SUBCASE("IN from X register")
    {
        pio.regs.x = 0xA5A5A5A5;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        SUBCASE("in x, 8")
        {
            pio.instructionMemory[0] = buildInInstruction(X, 8);
            pio.regs.pc = 0;
            pio.tick();

            CHECK(pio.regs.isr == 0xA5);
            CHECK(pio.regs.isr_shift_count == 8);

            SUBCASE("in x, +4")
            {
                pio.instructionMemory[1] = buildInInstruction(X, 4);
                pio.regs.pc = 1;
                pio.tick();

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

        pio.instructionMemory[0] = buildInInstruction(Y, 16);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0xFF00);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("IN from NULL source")
    {
        pio.regs.isr = 0xABCDEF01;
        pio.regs.isr_shift_count = 8;
        pio.settings.in_shift_right = false;

        pio.instructionMemory[0] = buildInInstruction(NULL_SRC, 8);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0xCDEF0100);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("IN from ISR")
    {
        pio.settings.in_base = 0;
        pio.regs.isr = 0x12345678;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        pio.instructionMemory[0] = buildInInstruction(ISR, 8);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0x34567878);
        CHECK(pio.regs.isr_shift_count == 8);
    }

    SUBCASE("IN from OSR")
    {
        pio.regs.osr = 0xFEDCBA98;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        pio.instructionMemory[0] = buildInInstruction(OSR, 16);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0xBA98);
        CHECK(pio.regs.isr_shift_count == 16);
    }

    SUBCASE("IN with maximum bit count (32)")
    {
        pio.regs.x = 0xABCDEF01;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;

        pio.instructionMemory[0] = buildInInstruction(X, 32);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0xABCDEF01);
        CHECK(pio.regs.isr_shift_count == 32);
    }

    SUBCASE("IN with ISR shift count saturation")
    {
        pio.regs.isr = 0x12345678;
        pio.regs.isr_shift_count = 30;
        pio.settings.in_shift_right = false;

        pio.regs.x = 0xF;
        pio.instructionMemory[0] = buildInInstruction(X, 4);
        pio.regs.pc = 0;
        pio.tick();

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

        pio.settings.in_base = 0;
        pio.gpio.raw_data[0] = 1;
        pio.gpio.raw_data[1] = 1;
        pio.instructionMemory[0] = buildInInstruction(PINS, 2);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.rx_fifo_count == 1);
        CHECK(pio.rx_fifo[0] == 0x2AB);
        CHECK(pio.regs.isr == 0);
        CHECK(pio.regs.isr_shift_count == 0);
    }

    SUBCASE("IN with exact threshold match")
    {
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 8;
        pio.rx_fifo_count = 0;
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.regs.x = 0xAB;

        pio.instructionMemory[0] = buildInInstruction(X, 8);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.rx_fifo_count == 1);
        CHECK(pio.rx_fifo[0] == 0xAB);
        CHECK(pio.regs.isr == 0);
        CHECK(pio.regs.isr_shift_count == 0);
    }

    SUBCASE("Reserved sources")
    {
        pio.regs.isr = 0x12345678;
        pio.regs.isr_shift_count = 10;

        SUBCASE("source 101")
        {
            pio.instructionMemory[0] = buildInInstruction(RESERVED_4, 8);
            pio.regs.pc = 0;
            pio.tick();

            CHECK(pio.regs.isr == 0x12345678);
            CHECK(pio.regs.isr_shift_count == 10);
        }

        SUBCASE("source 100")
        {
            pio.instructionMemory[0] = buildInInstruction(RESERVED_5, 8);
            pio.regs.pc = 0;
            pio.tick();

            CHECK(pio.regs.isr == 0x12345678);
            CHECK(pio.regs.isr_shift_count == 10);
        }
    }

    SUBCASE("IN from PINS with different base values")
    {
        for (int i = 0; i < 32; i++)
            pio.gpio.raw_data[i] = i % 2 ? 0 : 1;

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 0;
        pio.settings.in_shift_right = false;
        pio.instructionMemory[0] = buildInInstruction(PINS, 4);
        pio.regs.pc = 0;
        pio.tick();
        CHECK(pio.regs.isr == 0b0101);

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 16;
        pio.instructionMemory[1] = buildInInstruction(PINS, 4);
        pio.regs.pc = 1;
        pio.tick();
        CHECK(pio.regs.isr == 0b0101);

        for (int i = 0; i < 32; i++)
            pio.gpio.raw_data[i] = 0;
        pio.gpio.raw_data[24] = 1;
        pio.gpio.raw_data[25] = 1;

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 23;
        pio.instructionMemory[2] = buildInInstruction(PINS, 5);
        pio.regs.pc = 2;
        pio.tick();
        CHECK(pio.regs.isr == 0b00110);
    }

    SUBCASE("IN from PINS with wrap-around")
    {
        for (int i = 0; i < 32; i++)
            pio.gpio.raw_data[i] = (i % 3 == 0) ? 1 : 0;

        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_base = 30;
        pio.settings.in_shift_right = false;
        pio.instructionMemory[0] = buildInInstruction(PINS, 5);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.regs.isr == 0b00101);
    }

    SUBCASE("IN with RX FIFO full stalling")
    {
        pio.settings.in_shift_autopush = true;
        pio.settings.autopush_enable = true;
        pio.settings.push_threshold = 8;

        pio.rx_fifo_count = 4;
        for (int i = 0; i < 4; i++)
            pio.rx_fifo[i] = 0xF0 + i;

        pio.regs.isr = 0xAA;
        pio.regs.isr_shift_count = 4;
        pio.regs.x = 0xF;
        pio.instructionMemory[0] = buildInInstruction(X, 4);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.push_is_stalling == true);
        CHECK(pio.regs.isr == 0xAAF);
        CHECK(pio.regs.isr_shift_count == 8);
    }

    SUBCASE("IN with single bit operations")
    {
        pio.regs.isr = 0;
        pio.regs.isr_shift_count = 0;
        pio.settings.in_shift_right = false;
        pio.regs.x = 1;

        for (int i = 0; i < 8; i++)
        {
            pio.instructionMemory[i] = buildInInstruction(X, 1);
            pio.regs.pc = i;
            pio.tick();
        }

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

        for (int i = 0; i < 3; i++)
        {
            pio.regs.isr = 0;
            pio.regs.isr_shift_count = 0;
            pio.instructionMemory[i] = buildInInstruction(X, 4);
            pio.regs.pc = i;
            pio.tick();

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
        pio.regs.x = 0xAB;

        pio.instructionMemory[0] = buildInInstruction(X, 8);
        pio.regs.pc = 0;
        pio.tick();

        CHECK(pio.rx_fifo_count == 1);
        CHECK(pio.rx_fifo[0] == (0xAB << 24)); // 0xAB000000
        CHECK(pio.regs.isr == 0);
        CHECK(pio.regs.isr_shift_count == 0);
    }
}
