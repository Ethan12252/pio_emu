#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

TEST_CASE("WAIT on GPIO")
{
    // wait on gpio pin, this is using absolute GPIO index
    PioStateMachine pio;

    SUBCASE("wait for 1")
    {
        for (int i = 0; i < 32; i++)
            pio.instructionMemory[i] = 0x2080 + i; //   wait 1 gpio, i    
        // gpio.raw_data is init to 0
        for (int i = 0; i < 32; i++)
        {
            INFO("testing pin:", i);
            pio.tick();
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            pio.tick(); // should not change
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            // should not wait
            pio.gpio.raw_data[i] = 1;
            pio.tick();
            CHECK(pio.wait_is_stalling == false);
            CHECK(pio.regs.pc == ((i + 1) % 32));
        }
    }

    SUBCASE("wait for 0")
    {
        pio.regs.pc = 0;
        for (int i = 0; i < 32; i++)
            pio.instructionMemory[i] = 0x2000 + i; //   wait 0 gpio, i    
        // gpio.raw_data is init to 0
        for (int i = 0; i < 32; i++)
        {
            INFO("testing pin:", i);
            pio.gpio.raw_data[i] = 1;

            pio.tick();
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            pio.tick(); // should not change
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            // should not wait
            pio.gpio.raw_data[i] = 0;
            pio.tick();
            CHECK(pio.wait_is_stalling == false);
            CHECK(pio.regs.pc == ((i + 1) % 32));
        }
    }
}

TEST_CASE("WAIT on PIN (use 'in' mapping)")
{
    PioStateMachine pio;
    pio.settings.in_base = 5; // Example: input mapping starts at GPIO 5

    SUBCASE("wait for 1")
    {
        // Assembly: wait 1 pin i 
        for (int i = 0; i < 32; i++)
            pio.instructionMemory[i] = 0x20a0 + i;

        for (int i = 0; i < 32; i++)
        {
            INFO("testing pin:", i);
            int mapped_gpio = (pio.settings.in_base + i) % 32;
            pio.gpio.raw_data[mapped_gpio] = 0; // Pin is low

            pio.regs.pc = i;
            pio.tick();
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            pio.gpio.raw_data[mapped_gpio] = 1; // Pin is high
            pio.tick();
            CHECK(pio.wait_is_stalling == false);
            CHECK(pio.regs.pc == ((i + 1) % 32));
        }
    }

    SUBCASE("wait for 0")
    {
        // Assembly: wait 0 pin i
        for (int i = 0; i < 32; i++)
            pio.instructionMemory[i] = 0x2020 + i;

        for (int i = 0; i < 32; i++)
        {
            INFO("testing pin:", i);
            int mapped_gpio = (pio.settings.in_base + i) % 32;
            pio.gpio.raw_data[mapped_gpio] = 1; // Pin is high

            pio.regs.pc = i;
            pio.tick();
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            pio.gpio.raw_data[mapped_gpio] = 0; // Pin is low
            pio.tick();
            CHECK(pio.wait_is_stalling == false);
            CHECK(pio.regs.pc == ((i + 1) % 32));
        }
    }
}

TEST_CASE("WAIT on IRQ")
{
    PioStateMachine pio;

    SUBCASE("wait for 1")
    {
        // Assembly: wait 1 irq i
        // 'wait irq' with pol = 1, the irq flag should be cleared when condition met
        for (int i = 0; i < 8; i++)
            pio.instructionMemory[i] = 0x20c0 + i;

        pio.settings.wrap_end = 7;

        for (int i = 0; i < 8; i++)
        {
            INFO("testing irq:", i);
            pio.irq_flags[i] = false; // IRQ not set

            pio.regs.pc = i;
            pio.tick();
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            pio.irq_flags[i] = true; // IRQ set
            pio.tick();
            CHECK(pio.wait_is_stalling == false);
            CHECK(pio.regs.pc == ((i + 1) % 8));
            CHECK(pio.irq_flags[i] == false); // IRQ should be cleared after wait
        }
    }

    SUBCASE("wait for 0")
    {
        // Assembly: wait 0 irq i
        for (int i = 0; i < 8; i++)
            pio.instructionMemory[i] = 0x2040 + i;

        pio.settings.wrap_end = 7;

        for (int i = 0; i < 8; i++)
        {
            INFO("testing irq:", i);
            pio.irq_flags[i] = true; // IRQ set

            pio.regs.pc = i;
            pio.tick();
            CHECK(pio.wait_is_stalling == true);
            CHECK(pio.regs.pc == i);

            pio.irq_flags[i] = false; // IRQ cleared
            pio.tick();
            CHECK(pio.wait_is_stalling == false);
            CHECK(pio.regs.pc == ((i + 1) % 8));
            // IRQ should remain false
        }
    }
}