#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

// Helper function to build IRQ instructions
uint16_t buildIrqInstruction(bool clear, bool wait, uint8_t index)
{
    return (0b110 << 13) | ((clear ? 1 : 0) << 6) | ((wait ? 1 : 0) << 5) | (index & 0x1F);
}

TEST_CASE("executeIrq() test")
{
    PioStateMachine pio;

    // Init IRQ flags to false
    for (int i = 0; i < 8; i++)
    {
        pio.irq_flags[i] = false;
    }

    SUBCASE("Set IRQ flag")
    {
        // Set IRQ flag 3
        pio.currentInstruction = buildIrqInstruction(false, false, 3);
        pio.executeIrq();
        CHECK(pio.irq_flags[3] == true);

        // Check other flags are unaffected
        CHECK(pio.irq_flags[0] == false);
        CHECK(pio.irq_flags[1] == false);
        CHECK(pio.irq_flags[2] == false);
        CHECK(pio.irq_flags[4] == false);
        CHECK(pio.irq_flags[5] == false);
        CHECK(pio.irq_flags[6] == false);
        CHECK(pio.irq_flags[7] == false);
    }

    SUBCASE("Clear IRQ flag")
    {
        // Set all flags to true
        for (int i = 0; i < 8; i++)
        {
            pio.irq_flags[i] = true;
        }

        // Clear IRQ flag 5
        pio.currentInstruction = buildIrqInstruction(true, false, 5);
        pio.executeIrq();
        CHECK(pio.irq_flags[5] == false);

        // Check other flags are unaffected
        CHECK(pio.irq_flags[0] == true);
        CHECK(pio.irq_flags[1] == true);
        CHECK(pio.irq_flags[2] == true);
        CHECK(pio.irq_flags[3] == true);
        CHECK(pio.irq_flags[4] == true);
        CHECK(pio.irq_flags[6] == true);
        CHECK(pio.irq_flags[7] == true);
    }

    SUBCASE("IRQ with Wait bit")
    {
        // Set IRQ flag 2 with Wait bit
        pio.currentInstruction = buildIrqInstruction(false, true, 2);
        pio.executeIrq();

        // Check flag is set
        CHECK(pio.irq_flags[2] == true);

        // Check that irq_is_waiting is set to true
        CHECK(pio.irq_is_waiting == true);
    }

    SUBCASE("Clear IRQ with Wait bit (should be ignored)")
    {
        // Set all flags to true
        for (int i = 0; i < 8; i++)
        {
            pio.irq_flags[i] = true;
        }

        // Clear flag 4 with Wait bit (Wait should be ignored when Clear is set)
        pio.currentInstruction = buildIrqInstruction(true, true, 4);
        pio.executeIrq();

        // Check flag is cleared
        CHECK(pio.irq_flags[4] == false);

        // Check that irq_is_waiting is NOT set (Clear bit should make Wait bit have no effect)
        CHECK(pio.irq_is_waiting == false);
    }

    SUBCASE("IRQ with relative addressing - add state machine ID")
    {
        pio.stateMachineNumber = 2;

        // Set IRQ flag with MSB set for relative addressing (0x10 | 1 = 0x11)
        // With state machine 2, this should affect flag 3 (1+2=3)
        pio.currentInstruction = buildIrqInstruction(false, false, 0x11);
        pio.executeIrq();

        CHECK(pio.irq_flags[3] == true);

        // Now clear a different flag using relative addressing
        // With state machine 2, 0x13 should clear flag 1 (3+2=5, but only the 2 LSBs matter for modulo-4 addition)
        pio.currentInstruction = buildIrqInstruction(true, false, 0x13);
        pio.executeIrq();

        // Check the correct flag is affected
        CHECK(pio.irq_flags[1] == false);
    }

    SUBCASE("IRQ with relative addressing - bit 2 unaffected")
    {
        // Set state machine ID to 1
        pio.stateMachineNumber = 1;

        // Set IRQ flag with MSB and bit 2 set (0x14)
        // With state machine 1, this should affect flag 5 (4+1=5, where bit 2 remains unchanged)
        pio.currentInstruction = buildIrqInstruction(false, false, 0x14);
        pio.executeIrq();

        // Check the correct flag is set
        CHECK(pio.irq_flags[5] == true);

        // Set state machine ID to 3
        pio.stateMachineNumber = 3;

        // Clear IRQ flag with MSB and bit 2 set (0x14)
        // With state machine 3, this should affect flag 7 (4+3=7, where bit 2 remains unchanged)
        pio.currentInstruction = buildIrqInstruction(true, false, 0x14);
        pio.executeIrq();

        // Check the correct flag is affected
        CHECK(pio.irq_flags[7] == false);
    }

    SUBCASE("IRQ index out of bounds")
    {
        // Try to set IRQ flag 8 (out of bounds)
        pio.currentInstruction = buildIrqInstruction(false, false, 8);
        pio.executeIrq();

        CHECK(pio.irq_flags[0] == true);  // should set irq 0 (8=0b1000) we use the last 3 bits
        // Check that no other flags are affected
        for (int i = 1; i < 8; i++)
        {
            CHECK(pio.irq_flags[i] == false);
        }
    }

    SUBCASE("IRQ delay handling")
    {
        // Set IRQ with Wait bit and delay cycles
        // According to the datasheet: "If Wait is set, Delay cycles do not begin until after the wait period elapses"
        // We'll test that delay_delay is set to true when Wait is set
        pio.delay_delay = false;
        pio.regs.delay = 0;
        pio.currentInstruction = buildIrqInstruction(false, true, 0);
        pio.executeIrq();

        // Check that delay_delay is set to true (delaying the delay cycles)
        CHECK(pio.delay_delay == true);

        // Now test without Wait bit - delay should not be postponed
        pio.delay_delay = false;
        pio.regs.delay = 0;
        pio.currentInstruction = buildIrqInstruction(false, false, 1);
        pio.executeIrq();

        // Check that delay_delay is not set
        CHECK(pio.delay_delay == false);
    }
}
