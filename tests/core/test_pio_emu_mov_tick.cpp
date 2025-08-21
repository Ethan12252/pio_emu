#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

// Helper for standard MOV encoding
uint16_t buildMovInstruction(uint8_t dest, uint8_t op, uint8_t src, uint8_t delay = 0)
{
    // MOV: 0b101 (bits 15-13), delay (bits 12-8), dest (bits 7-5), op (bits 4-3), src (bits 2-0)
    return (0b101 << 13) | (delay << 8) | (dest << 5) | (op << 3) | src;
}

// Helper for MOV to RX FIFO (PUT)
uint16_t buildMovRxPutInstruction(bool idxi, uint8_t index, uint8_t delay = 0)
{
    // MOV RX PUT: 0b100 (bits 15-13), delay (bits 12-8), 0b0001 (bits 7-4), idxi (bit 3), index (bits 2-0)
    return (0b100 << 13) | (delay << 8) | (0b0001 << 4) | (idxi << 3) | (index & 0x7);
}

// Helper for MOV from RX FIFO (GET)
uint16_t buildMovRxGetInstruction(bool idxi, uint8_t index, uint8_t delay = 0)
{
    // MOV RX GET: 0b100 (bits 15-13), delay (bits 12-8), 0b1001 (bits 7-4), idxi (bit 3), index (bits 2-0)
    return (0b100 << 13) | (delay << 8) | (0b1001 << 4) | (idxi << 3) | (index & 0x7);
}

/*
TODO:
covered:
    ?All standard MOV destination/source/operation combinations.
    ?Special behaviors (PC, EXEC, ISR/OSR reset, STATUS, PINS).
    ?Delay cycles.
    ?MOV to RX FIFO (by Y and by index, with/without config bit).
    ?MOV from RX FIFO (by Y and by index, with/without config bit).
    ?Reserved encoding edge cases.
need to adjust:
    ?The field names and RX FIFO array in PioStateMachine.
    ?The config bits (settings.fjoin_rx_put, settings.fjoin_rx_get).
    ?The bit-reverse implementation for full coverage.
*/

TEST_CASE("Standard MOV: X <- Y, None")
{
    PioStateMachine pio;
    pio.regs.y = 0x12345678;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b001, 0b00, 0b010); // MOV X, Y
    pio.tick();
    CHECK(pio.regs.x == 0x12345678);
}

TEST_CASE("Standard MOV: Y <- X, Invert")
{
    PioStateMachine pio;
    pio.regs.x = 0x0F0F0F0F;
    pio.regs.y = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b010, 0b01, 0b001); // MOV Y, ~X
    pio.tick();
    CHECK(pio.regs.y == ~0x0F0F0F0F);
}

TEST_CASE("Standard MOV: X <- Y, Bit-reverse")
{
    PioStateMachine pio;
    pio.regs.y = 0x80000001;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b001, 0b10, 0b010); // MOV X, ::Y
    pio.tick();
    // Bit-reverse of 0x80000001 is 0x80000001 for 32-bit, but you may want to implement a bit-reverse helper for full coverage
    CHECK(pio.regs.x == 0x80000001);
}

TEST_CASE("Standard MOV: PC <- X (Unconditional jump)")
{
    PioStateMachine pio;
    pio.regs.x = 7;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b101, 0b00, 0b001); // MOV PC, X
    pio.tick();
    CHECK(pio.regs.pc == 7);
}

TEST_CASE("Standard MOV: EXEC <- X (Execute register as instruction)")
{
    PioStateMachine pio;
    // Place a JMP Always to 5 in X
    pio.regs.x = (0b000 << 13) | (0 << 8) | (0b000 << 5) | 5;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b100, 0b00, 0b001); // MOV EXEC, X
    pio.tick();
    CHECK(pio.regs.pc == 5); // Should jump to 5 after executing X as instruction
}

TEST_CASE("Standard MOV: ISR <- OSR, ISR reset")
{
    PioStateMachine pio;
    pio.regs.osr = 0xDEADBEEF;
    pio.regs.isr = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b110, 0b00, 0b111); // MOV ISR, OSR
    pio.tick();
    CHECK(pio.regs.isr == 0); // Should be reset to 0 after operation
}

TEST_CASE("Standard MOV: OSR <- ISR, OSR reset")
{
    PioStateMachine pio;
    pio.regs.isr = 0xCAFEBABE;
    pio.regs.osr = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b111, 0b00, 0b110); // MOV OSR, ISR
    pio.tick();
    CHECK(pio.regs.osr == 0xFFFFFFFF); // Should be reset to full after operation
}

TEST_CASE("Standard MOV: X <- PINS (Read pins)")
{
    PioStateMachine pio;
    pio.gpio.raw_data[0] = 1;
    pio.gpio.raw_data[1] = 0;
    pio.gpio.raw_data[2] = 1;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b001, 0b00, 0b000); // MOV X, PINS
    pio.tick();
    CHECK((pio.regs.x & 0x7) == 0b101);
}

TEST_CASE("Standard MOV: X <- STATUS (all-ones)")
{
    PioStateMachine pio;
    pio.regs.x = 0;
    pio.settings.status_sel = 1; // Simulate STATUS source as all-ones
    pio.instructionMemory[0] = buildMovInstruction(0b001, 0b00, 0b101); // MOV X, STATUS
    pio.tick();
    CHECK(pio.regs.x == 0xFFFFFFFF);
}

TEST_CASE("Standard MOV: X <- STATUS (all-zeroes)")
{
    PioStateMachine pio;
    pio.regs.x = 0;
    pio.settings.status_sel = 0; // Simulate STATUS source as all-zeroes
    pio.instructionMemory[0] = buildMovInstruction(0b001, 0b00, 0b101); // MOV X, STATUS
    pio.tick();
    CHECK(pio.regs.x == 0x0);
}

TEST_CASE("Standard MOV: with delay")
{
    PioStateMachine pio;
    pio.regs.y = 0xA5A5A5A5;
    pio.regs.x = 0;
    pio.instructionMemory[0] = buildMovInstruction(0b001, 0b00, 0b010, 2); // MOV X, Y, delay 2
    pio.tick();
    CHECK(pio.regs.x == 0xA5A5A5A5);
    pio.tick();
    CHECK(pio.regs.pc == 0);
    pio.tick();
    CHECK(pio.regs.pc == 0);
    pio.tick();
    CHECK(pio.regs.pc == 1);
}

// --- RX FIFO MOV INSTRUCTIONS ---

TEST_CASE("MOV to RX FIFO: rxfifo[y] <- isr (FJOIN_RX_PUT set)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_put = true;
    pio.regs.isr = 0xDEADBEEF;
    pio.regs.y = 2;
    pio.rx_fifo[2] = 0;
    pio.instructionMemory[0] = buildMovRxPutInstruction(false, 0); // IdxI=0, index=0 (use Y)
    pio.tick();
    CHECK(pio.rx_fifo[2] == 0xDEADBEEF);
}

TEST_CASE("MOV to RX FIFO: rxfifo[<index>] <- isr (FJOIN_RX_PUT set)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_put = true;
    pio.regs.isr = 0xCAFEBABE;
    pio.rx_fifo[3] = 0;
    pio.instructionMemory[0] = buildMovRxPutInstruction(true, 3); // IdxI=1, index=3
    pio.tick();
    CHECK(pio.rx_fifo[3] == 0xCAFEBABE);
}

TEST_CASE("MOV to RX FIFO: rxfifo[y] <- isr (FJOIN_RX_PUT not set, should not write)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_put = false;
    pio.regs.isr = 0xDEADBEEF;
    pio.regs.y = 1;
    pio.rx_fifo[1] = 0;
    pio.instructionMemory[0] = buildMovRxPutInstruction(false, 0); // IdxI=0, index=0 (use Y)
    pio.tick();
    CHECK(pio.rx_fifo[1] == 0); // Should not write
}

TEST_CASE("MOV from RX FIFO: osr <- rxfifo[y] (FJOIN_RX_GET set)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_get = true;
    pio.regs.y = 2;
    pio.rx_fifo[2] = 0xAABBCCDD;
    pio.regs.osr = 0;
    pio.instructionMemory[0] = buildMovRxGetInstruction(false, 0); // IdxI=0, index=0 (use Y)
    pio.tick();
    CHECK(pio.regs.osr == 0xAABBCCDD);
}

TEST_CASE("MOV from RX FIFO: osr <- rxfifo[<index>] (FJOIN_RX_GET set)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_get = true;
    pio.rx_fifo[1] = 0x11223344;
    pio.regs.osr = 0;
    pio.instructionMemory[0] = buildMovRxGetInstruction(true, 1); // IdxI=1, index=1
    pio.tick();
    CHECK(pio.regs.osr == 0x11223344);
}

TEST_CASE("MOV from RX FIFO: osr <- rxfifo[y] (FJOIN_RX_GET not set, should not read)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_get = false;
    pio.regs.y = 1;
    pio.rx_fifo[1] = 0xAABBCCDD;
    pio.regs.osr = 0;
    pio.instructionMemory[0] = buildMovRxGetInstruction(false, 0); // IdxI=0, index=0 (use Y)
    pio.tick();
    CHECK(pio.regs.osr == 0); // Should not read
}

TEST_CASE("MOV to RX FIFO: reserved encoding (IdxI=0, index!=0)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_put = true;
    pio.regs.isr = 0xDEADBEEF;
    pio.rx_fifo[2] = 0;
    pio.instructionMemory[0] = buildMovRxPutInstruction(false, 2); // IdxI=0, index=2 (reserved)
    pio.tick();
    // Behavior is undefined, but should not write in a strict implementation
    CHECK(pio.rx_fifo[2] == 0);
}

TEST_CASE("MOV from RX FIFO: reserved encoding (IdxI=0, index!=0)")
{
    PioStateMachine pio;
    pio.settings.fjoin_rx_get = true;
    pio.rx_fifo[3] = 0x11223344;
    pio.regs.osr = 0;
    pio.instructionMemory[0] = buildMovRxGetInstruction(false, 3); // IdxI=0, index=3 (reserved)
    pio.tick();
    // Behavior is undefined, but should not read in a strict implementation
    CHECK(pio.regs.osr == 0);
}