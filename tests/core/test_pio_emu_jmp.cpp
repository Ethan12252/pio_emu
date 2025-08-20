#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

enum class JmpCondition : uint8_t {
    ALWAYS   = 0b000,
    NOT_X    = 0b001,
    X_DEC    = 0b010,
    NOT_Y    = 0b011,
    Y_DEC    = 0b100,
    X_NEQ_Y  = 0b101,
    PIN      = 0b110,
    NOT_OSRE = 0b111
};

const uint8_t JMP_TARGET_MASK = 0x1F;
constexpr uint16_t nop_inst = (0b101 << 13) | (0b010 << 5) | (0b00 << 3) | 0b010;

// Helper to build a JMP instruction
uint16_t buildJmpInstruction(JmpCondition condition, uint8_t address, uint8_t delay = 0)
{
    // JMP: 0b000 (bits 15-13), delay (bits 12-8), condition (bits 7-5), address (bits 4-0)
    return (delay << 8) | (static_cast<uint8_t>(condition) << 5) | (address & JMP_TARGET_MASK);
}

TEST_CASE("JMP Always")
{
    PioStateMachine pio;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::ALWAYS, 5); // Always jump to 5
    pio.tick();
    CHECK(pio.regs.pc == 5);
}

TEST_CASE("JMP !X (scratch X zero)")
{
    PioStateMachine pio;
    pio.regs.x = 0;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::NOT_X, 7);
    pio.tick();
    CHECK(pio.regs.pc == 7);

    pio.regs.x = 1;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::NOT_X, 7);
    pio.tick();
    CHECK(pio.regs.pc == 1); // No jump, pc increments
}

TEST_CASE("JMP X-- (scratch X non-zero, prior to decrement)")
{
    PioStateMachine pio;
    pio.regs.x = 2;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::X_DEC, 9);
    pio.tick();
    CHECK(pio.regs.pc == 9); // jump
    CHECK(pio.regs.x == 1); // X decremented

    pio.regs.x = 0;  // Zero befor decrement, should not jump
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::X_DEC, 9);
    pio.tick();
    CHECK(pio.regs.pc == 1); // No jump
    CHECK(pio.regs.x == 0); // X remains zero
}

TEST_CASE("JMP !Y (scratch Y zero)")
{
    PioStateMachine pio;
    pio.regs.y = 0;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::NOT_Y, 11);
    pio.tick();
    CHECK(pio.regs.pc == 11);  // Jump

    pio.regs.y = 3;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::NOT_Y, 11);
    pio.tick();
    CHECK(pio.regs.pc == 1); // No jump
}

TEST_CASE("JMP Y-- (scratch Y non-zero, prior to decrement)")
{
    PioStateMachine pio;
    pio.regs.y = 2;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::Y_DEC, 13);
    pio.tick();
    CHECK(pio.regs.pc == 13);
    CHECK(pio.regs.y == 1);

    pio.regs.y = 0;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::Y_DEC, 13);
    pio.tick();
    CHECK(pio.regs.pc == 1);
    CHECK(pio.regs.y == 0);
}

TEST_CASE("JMP X!=Y")
{
    PioStateMachine pio;
    pio.regs.x = 5;
    pio.regs.y = 3;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::X_NEQ_Y, 15);
    pio.tick();
    CHECK(pio.regs.pc == 15);

    pio.regs.x = 7;
    pio.regs.y = 7;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::X_NEQ_Y, 15);
    pio.tick();
    CHECK(pio.regs.pc == 1); // No jump
}

TEST_CASE("JMP PIN")
{
    PioStateMachine pio;
    pio.settings.jmp_pin = 1; // pin 1
    pio.gpio.raw_data[1] = 1; // pin1 high, should jump
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::PIN, 17);
    pio.tick();
    CHECK(pio.regs.pc == 17); // jump

    pio.gpio.raw_data[1] = 0; // pin1 low, should NOT jump
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::PIN, 17);
    pio.tick();
    CHECK(pio.regs.pc == 1); // No jump
}

// TODO: Unfinished
TEST_CASE("JMP !OSRE (output shift register not empty)")
{
    // JUMP when osr is not empty
    PioStateMachine pio;
    // regs.osr_shift_count < settings.pull_threshold -> empty
    
    // Simulate OSR empty
    pio.settings.pull_threshold = 32; // when shifted out 32bit, pull from txfifo
    pio.regs.osr_shift_count = 32;    // shifted out all 32 bit (osr empty, should )

    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::NOT_OSRE, 19);
    pio.tick();
    CHECK(pio.regs.pc == 1);         // Not Jump
    
    // Simulate OSR not empty
    pio.settings.pull_threshold = 32; // when shifted out 32bit, pull from txfifo
    pio.regs.osr_shift_count = 8;     // shifted out 8 bit (still not empty)
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::NOT_OSRE, 19);
    pio.tick();
    CHECK(pio.regs.pc == 19); // jump
}

TEST_CASE("JMP Delay always applies")
{
    PioStateMachine pio;
    pio.regs.pc = 0;
    pio.instructionMemory[0] = buildJmpInstruction(JmpCondition::ALWAYS, 5, 3); // Always jump, delay 3
    pio.instructionMemory[5] = nop_inst;

    pio.tick();
    CHECK(pio.regs.pc == 5); // command it self

    pio.tick();
    CHECK(pio.regs.pc == 5); // 1st delay

    pio.tick();
    CHECK(pio.regs.pc == 5); // 2nd delay

    pio.tick();
    CHECK(pio.regs.pc == 5); // 3rd

    pio.tick();
    CHECK(pio.regs.pc == 6); // 5 + 1

    pio.tick();
    CHECK(pio.regs.pc == 7); // 5 + 2
}