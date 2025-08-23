#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <algorithm>
#include "../../src/PioStateMachine.h"

enum class MovDestination : uint8_t {
    PINS = 0b000,
    X = 0b001,
    Y = 0b010,
    // 0b011 is Reserved
    EXEC = 0b100,
    PC = 0b101,
    ISR = 0b110,
    OSR = 0b111
};

enum class MovSource : uint8_t {
    PINS = 0b000,
    X = 0b001,
    Y = 0b010,
    NULL_ = 0b011,
    // 0b100 is Reserved
    STATUS = 0b101,
    ISR = 0b110,
    OSR = 0b111
};

enum class MovOperation : uint8_t {
    NONE = 0b00,
    INVERT = 0b01,
    BIT_REVERSE = 0b10,
    // 0b11 is Reserved
};

// Helper for MOV encoding
uint16_t buildMovInstruction(MovDestination dest, MovOperation op, MovSource src, uint8_t delay = 0)
{
    // MOV: 0b101 (bits 15-13), delay (bits 12-8), dest (bits 7-5), op (bits 4-3), src (bits 2-0)
    return (0b101 << 13) | (delay << 8) | (static_cast<uint8_t>(dest) << 5) |
        (static_cast<uint8_t>(op) << 3) | static_cast<uint8_t>(src);
}

//// Keep the old function for backward compatibility if needed
//uint16_t buildMovInstruction(uint8_t dest, uint8_t op, uint8_t src, uint8_t delay = 0)
//{
//    return (0b101 << 13) | (delay << 8) | (dest << 5) | (op << 3) | src;
//}

TEST_CASE("Standard MOV: X <- Y, None")
{
    PioStateMachine pio;
    pio.regs.y = 0x12345678;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::X, MovOperation::NONE, MovSource::Y);
    pio.tick();
    CHECK(pio.regs.y == 0x12345678);
    CHECK(pio.regs.x == 0x12345678);
}

TEST_CASE("MOV: Y <- X, Invert")
{
    PioStateMachine pio;
    pio.regs.x = 0x0F0F0F0F;
    pio.regs.y = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::Y, MovOperation::INVERT, MovSource::X);
    pio.tick();
    CHECK(pio.regs.y == ~0x0F0F0F0F);
}

TEST_CASE("MOV: X <- Y, Bit-reverse") {
    PioStateMachine pio;
    pio.regs.y = 0x80000001; // MSB and LSB set
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::X, MovOperation::BIT_REVERSE, MovSource::Y);
    pio.tick();
    CHECK(pio.regs.x == 0x80000001); // Should be bit-reversed
}

TEST_CASE("MOV: X <- NULL") {
    PioStateMachine pio;
    pio.regs.x = 0xFFFFFFFF;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::X, MovOperation::NONE, MovSource::NULL_);
    pio.tick();
    CHECK(pio.regs.x == 0); // NULL should provide zero
}

TEST_CASE("MOV: PINS <- X") {
    PioStateMachine pio;
    pio.regs.x = 0b100;
    pio.settings.out_base = 0;
    pio.settings.out_count = 3;
    std::fill(pio.gpio.out_pindirs.begin(), pio.gpio.out_pindirs.begin() + 3, 0); // set to output
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::PINS, MovOperation::NONE, MovSource::X);
    pio.tick();
    // Check that pins were set according to OUT pin mapping
    CHECK(pio.gpio.raw_data[0] == 0);
    CHECK(pio.gpio.raw_data[1] == 0);
    CHECK(pio.gpio.raw_data[2] == 1);
}

TEST_CASE("MOV: PC <- X (Unconditional jump)")
{
    PioStateMachine pio;
    pio.regs.x = 7;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::PC, MovOperation::NONE, MovSource::X);
    pio.tick();
    CHECK(pio.regs.pc == 7);
}

TEST_CASE("MOV: EXEC <- X (Execute register as instruction)")
{
    PioStateMachine pio;
    // Place a JMP Always to 5 in X
    pio.regs.x = 0x0004;  // jmp 4
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::EXEC, MovOperation::NONE, MovSource::X);
    pio.tick();
    pio.tick();
    CHECK(pio.regs.pc == 4); // Should jump to 4 after executing X as instruction
}

// TODO: Need check
TEST_CASE("MOV: ISR <- OSR, ISR reset")
{
    PioStateMachine pio;
    pio.regs.osr = 0xDEADBEEF;
    pio.regs.isr = 0x69;
    pio.regs.isr_shift_count = 8;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::ISR, MovOperation::NONE, MovSource::OSR);
    pio.tick();
    CHECK(pio.regs.isr == 0xDEADBEEF);
    CHECK(pio.regs.isr_shift_count == 0); // s3.4.8.2 Input shift counter is reset to 0 by this op (i.e. empty)
    //CHECK(pio.regs.osr == 0); TODO: Should source also cleared? Current code does not
}

// TODO: Need check
TEST_CASE("MOV: OSR <- ISR, OSR reset")
{
    PioStateMachine pio;
    pio.regs.isr = 0xCAFEBABE;
    pio.regs.osr = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::OSR, MovOperation::NONE, MovSource::ISR);
    pio.tick();
    CHECK(pio.regs.osr == 0xCAFEBABE);
    CHECK(pio.regs.osr_shift_count == 0); // s3.4.8.2 Output shift counter is reset to 0 by this op (i.e. full)
    //CHECK(pio.regs.isr == 0); TODO: Should source also cleared? Current code does not
}

TEST_CASE("MOV: X <- PINS (Read pins)")
{
    PioStateMachine pio;
    // uses 'in' command's pin configuration
    pio.settings.in_base = 0;
    pio.gpio.raw_data[0] = 0;
    pio.gpio.raw_data[1] = 1;
    pio.gpio.raw_data[2] = 1;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::X, MovOperation::NONE, MovSource::PINS);
    pio.tick();
    CHECK((pio.regs.x & 0x7) == 0b110);
}

TEST_CASE("MOV: X <- STATUS (all-ones)")
{
    PioStateMachine pio;
    pio.regs.x = 0;
    pio.settings.status_sel = 1; // STATUS source txfifo
    pio.settings.fifo_level_N = 4;
    pio.tx_fifo_count = 4; // full
    pio.instructionMemory[0] = 0xa042; // nop 
    pio.instructionMemory[1] = buildMovInstruction(MovDestination::X, MovOperation::NONE, MovSource::STATUS);
    pio.tick();
    pio.tick();
    CHECK(pio.regs.x == 0xFFFFFFFF);
}

TEST_CASE("MOV: X <- STATUS (all-zeroes)")
{
    PioStateMachine pio;
    pio.regs.x = 0;
    pio.settings.status_sel = 0; // Simulate STATUS source as all-zeroes
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::X, MovOperation::NONE, MovSource::STATUS);
    pio.tick();
    CHECK(pio.regs.x == 0x0);
}

TEST_CASE("MOV: with delay")
{
    PioStateMachine pio;
    pio.regs.y = 0xA5A5A5A5;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::X, MovOperation::NONE, MovSource::Y, 2);
    pio.instructionMemory[1] = 0xa042;  // nop

    pio.tick();  // Instruction it self
    CHECK(pio.regs.x == 0xA5A5A5A5);
    CHECK(pio.regs.pc == 1);

    pio.tick();  // First delay
    CHECK(pio.regs.pc == 1);

    pio.tick();  // Second delay
    CHECK(pio.regs.pc == 1);

    pio.tick();
    CHECK(pio.regs.pc == 2);
}

#if 1

// Tests for PINS to ISR/OSR
TEST_CASE("MOV: ISR <- PINS")
{
    PioStateMachine pio;
    // Set up pin configuration for IN instruction (MOV PINS uses same config)
    pio.settings.in_base = 0;
    pio.gpio.raw_data[0] = 0;
    pio.gpio.raw_data[1] = 1;
    pio.gpio.raw_data[2] = 1;
    pio.regs.isr = 0x69;
    pio.regs.isr_shift_count = 8; // Some existing data

    pio.instructionMemory[0] = buildMovInstruction(MovDestination::ISR, MovOperation::NONE, MovSource::PINS);
    pio.tick();

    CHECK((pio.regs.isr & 0x7) == 0b110); // Should read pins 0-2: 101
    CHECK(pio.regs.isr_shift_count == 0); // ISR shift counter reset to 0 by MOV
}

TEST_CASE("MOV: OSR <- PINS")
{
    PioStateMachine pio;
    // Set up pin configuration 
    pio.settings.in_base = 2;
    pio.gpio.raw_data[0] = 1;
    pio.gpio.raw_data[1] = 0;
    pio.gpio.raw_data[2] = 1;
    pio.gpio.raw_data[3] = 1;
    pio.regs.osr = 0xFFFFFFFF;
    pio.regs.osr_shift_count = 16; // Some existing shift count

    pio.instructionMemory[0] = buildMovInstruction(MovDestination::OSR, MovOperation::NONE, MovSource::PINS);
    pio.tick();

    CHECK((pio.regs.osr & 0x7) == 0b011); // Should read pins 2-4: 011 (pins 2=1, 3=1, 4=0 from raw_data[2,3] assuming pin4 default 0)
    CHECK(pio.regs.osr_shift_count == 0); // OSR shift counter reset to 0 by MOV
}

// Tests for NULL to PC/ISR/OSR/PINS
TEST_CASE("MOV: PC <- NULL")
{
    PioStateMachine pio;
    pio.regs.pc = 5;
    pio.instructionMemory[5] = buildMovInstruction(MovDestination::PC, MovOperation::NONE, MovSource::NULL_);
    pio.tick();
    CHECK(pio.regs.pc == 0); // NULL provides 0, so PC should be 0
}

TEST_CASE("MOV: ISR <- NULL")
{
    PioStateMachine pio;
    pio.regs.isr = 0xDEADBEEF;
    pio.regs.isr_shift_count = 16;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::ISR, MovOperation::NONE, MovSource::NULL_);
    pio.tick();
    CHECK(pio.regs.isr == 0); // NULL provides 0
    CHECK(pio.regs.isr_shift_count == 0); // ISR shift counter reset by MOV
}

TEST_CASE("MOV: OSR <- NULL")
{
    PioStateMachine pio;
    pio.regs.osr = 0xCAFEBABE;
    pio.regs.osr_shift_count = 8;
    pio.instructionMemory[0] = buildMovInstruction(MovDestination::OSR, MovOperation::NONE, MovSource::NULL_);
    pio.tick();
    CHECK(pio.regs.osr == 0); // NULL provides 0
    CHECK(pio.regs.osr_shift_count == 0); // OSR shift counter reset by MOV
}

TEST_CASE("MOV: PINS <- NULL")
{
    PioStateMachine pio;
    // Set up output pins
    pio.settings.out_base = 0;
    pio.settings.out_count = 3;
    std::fill(pio.gpio.out_pindirs.begin(), pio.gpio.out_pindirs.begin() + 3, 0); // set to output
    // Set pins to some initial values
    pio.gpio.raw_data[0] = 1;
    pio.gpio.raw_data[1] = 1;
    pio.gpio.raw_data[2] = 1;

    pio.instructionMemory[0] = buildMovInstruction(MovDestination::PINS, MovOperation::NONE, MovSource::NULL_);
    pio.tick();

    // NULL should output 0 to all pins
    CHECK(pio.gpio.raw_data[0] == 0);
    CHECK(pio.gpio.raw_data[1] == 0);
    CHECK(pio.gpio.raw_data[2] == 0);
}

// Tests with operations applied
TEST_CASE("MOV: ISR <- PINS, Invert")
{
    PioStateMachine pio;
    pio.settings.in_base = 0;
    pio.gpio.raw_data[0] = 1;
    pio.gpio.raw_data[1] = 0;
    pio.gpio.raw_data[2] = 1;
    pio.regs.isr = 0;

    pio.instructionMemory[0] = buildMovInstruction(MovDestination::ISR, MovOperation::INVERT, MovSource::PINS);
    pio.tick();

    // Should read 0b101, then invert to get ~0b101 = 0xFFFFFFFA
    CHECK(pio.regs.isr == ~0x5u); // ~(0b101)
    CHECK(pio.regs.isr_shift_count == 0);
}

TEST_CASE("MOV: PC <- NULL, Bit-reverse")
{
    PioStateMachine pio;
    pio.regs.pc = 10;
    pio.instructionMemory[10] = buildMovInstruction(MovDestination::PC, MovOperation::BIT_REVERSE, MovSource::NULL_);
    pio.tick();
    // NULL is 0, bit-reverse of 0 is still 0
    CHECK(pio.regs.pc == 0);
}

TEST_CASE("MOV: PINS <- NULL, Invert")
{
    PioStateMachine pio;
    pio.settings.out_base = 0;
    pio.settings.out_count = 4;
    std::fill(pio.gpio.out_pindirs.begin(), pio.gpio.out_pindirs.begin() + 4, 0); // set to output
    // Set pins to initial values
    std::fill(pio.gpio.raw_data.begin(), pio.gpio.raw_data.begin() + 4, 0);

    pio.instructionMemory[0] = buildMovInstruction(MovDestination::PINS, MovOperation::INVERT, MovSource::NULL_);
    pio.tick();

    // NULL=0, inverted = 0xFFFFFFFF, but only out_count bits are used
    // Should set pins 0-3 to 1 (from the inverted NULL)
    CHECK(pio.gpio.raw_data[0] == 1);
    CHECK(pio.gpio.raw_data[1] == 1);
    CHECK(pio.gpio.raw_data[2] == 1);
    CHECK(pio.gpio.raw_data[3] == 1);
}
#endif 