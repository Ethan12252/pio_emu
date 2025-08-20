#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"
#include <algorithm>
#include <cstdint>

// Test fixture for PioStateMachine tests
class PioStateMachineFixture
{
public:
    PioStateMachineFixture()
    {
        // Fill the instruction memory with 'nop' ( mov y, y ) 
        const uint16_t nop_inst = (0b101 << 13) | (0b010 << 5) | (0b00 << 3) | 0b010;
        pio.instructionMemory.fill(nop_inst);
        pio.regs.pc = 0;
    }

    ~PioStateMachineFixture()
    {
        resetGpioArrays(-1);
    }

protected:
    PioStateMachine pio;

    // Helper to reset GPIO arrays
    // raw_data and pindir to 0, others to -1
    void resetGpioArrays(int8_t value = -1)
    {
        std::fill(pio.gpio.raw_data.begin(), pio.gpio.raw_data.end(), 0); // 0 by default
        std::fill(pio.gpio.set_data.begin(), pio.gpio.set_data.end(), value);
        std::fill(pio.gpio.out_data.begin(), pio.gpio.out_data.end(), value);
        std::fill(pio.gpio.external_data.begin(), pio.gpio.external_data.end(), value);
        std::fill(pio.gpio.sideset_data.begin(), pio.gpio.sideset_data.end(), value);
        std::fill(pio.gpio.pindirs.begin(), pio.gpio.pindirs.end(), 0); // 0 = output by default
        std::fill(pio.gpio.set_pindirs.begin(), pio.gpio.set_pindirs.end(), value);
        std::fill(pio.gpio.out_pindirs.begin(), pio.gpio.out_pindirs.end(), value);
        std::fill(pio.gpio.sideset_pindirs.begin(), pio.gpio.sideset_pindirs.end(), value);
    }

    // Helper to set all 'pins' in 'array' to value
    static void setPins(std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t value)
    {
        for (int pin : pins)
        {
            if (pin >= 0 && pin < 32)
                array[pin] = value;
        }
    }

    // Helper to check if all 'pins' in 'array' have the value of expected
    static void checkPins(const std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t expected)
    {
        for (int pin : pins)
            CHECK(array[pin] == expected);
    }

    // Helper to check if pins *NOT* in 'pins' have expected values
    static void checkPinsNotIn(const std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t expected)
    {
        for (int i = 0; i < 32; i++)
        {
            if (std::find(pins.begin(), pins.end(), i) == pins.end())
                CHECK(array[i] == expected);
        }
    }
};

TEST_CASE_FIXTURE(PioStateMachineFixture, "PioStateMachine::setAllGpio tests")
{
    SUBCASE("Initial state should not modify raw_data")
    {
        resetGpioArrays(-1);
        pio.tick();

        // Check if raw_data remains 0 (initial value)
        checkPinsNotIn(pio.gpio.raw_data, {}, 0);
    }

    SUBCASE("GPIO priority: out/set (lowest)")
    {
        resetGpioArrays(-1);
        std::fill(pio.gpio.pindirs.begin(), pio.gpio.pindirs.end(), 0); // pindirs to output

        // Set out_data pin 0, 5 to 1
        setPins(pio.gpio.out_data, {0, 5}, 1);

        // Set set_data pin 3, 8 to 1
        setPins(pio.gpio.set_data, {3, 8}, 1);

        pio.tick();

        checkPins(pio.gpio.raw_data, {0, 3, 5, 8}, 1);

        // Check if other pins remains unmodified
        checkPinsNotIn(pio.gpio.raw_data, {0, 3, 5, 8}, 0);
    }

    SUBCASE("GPIO priority: sideset (mid)")
    {
        resetGpioArrays(-1);
        std::fill(pio.gpio.pindirs.begin(), pio.gpio.pindirs.end(), 0); // pindirs output

        // Set out_data pin 0, 1 to 1
        setPins(pio.gpio.out_data, {0, 1}, 1);

        // Set sideset_data with conflicting value on pin 0
        setPins(pio.gpio.sideset_data, {0, 2}, 0);

        pio.tick();

        // sideset should override out for pin 0
        CHECK(pio.gpio.raw_data[0] == 0); // sideset wins
        CHECK(pio.gpio.raw_data[1] == 1); // set by out_data
        CHECK(pio.gpio.raw_data[2] == 0); // set by sideset
    }

    SUBCASE("GPIO priority: external (highest priority)")
    {
        resetGpioArrays(-1);
        std::fill(pio.gpio.pindirs.begin(), pio.gpio.pindirs.end(), 0); // output

        // side-set
        setPins(pio.gpio.out_data, {0, 1}, 1);
        // out
        setPins(pio.gpio.sideset_data, {0, 2}, 0);

        // conflict on pin 0, 2
        setPins(pio.gpio.external_data, {0, 1, 2}, 1);

        pio.tick();

        // external should override everything
        CHECK(pio.gpio.raw_data[0] == 1); // external wins over side-set and out
        CHECK(pio.gpio.raw_data[1] == 1);
        CHECK(pio.gpio.raw_data[2] == 1);
    }

    SUBCASE("Initial state should not modify pindirs")
    {
        resetGpioArrays(-1);
        pio.tick();

        // should reamin default 0 (output)
        checkPinsNotIn(pio.gpio.pindirs, {}, 0);
    }

    SUBCASE("OUT PINDIRS sets pin directions (lowest priority)")
    {
        resetGpioArrays(-1);

        setPins(pio.gpio.out_pindirs, {0, 1, 2}, 1);

        pio.tick();

        // should be output 1
        checkPins(pio.gpio.pindirs, {0, 1, 2}, 1);

        // Check others
        checkPinsNotIn(pio.gpio.pindirs, {0, 1, 2}, 0);
    }

    SUBCASE("SIDESET PINDIRS overrides OUT and SET (mid priority)")
    {
        resetGpioArrays(-1);

        // OUT 0 1 to 1(input)
        setPins(pio.gpio.out_pindirs, {0, 1}, 1);

        // SET 1 2 to 1
        setPins(pio.gpio.set_pindirs, {1, 2}, 1);

        // SIDESET 0,1 to 0(output)
        setPins(pio.gpio.sideset_pindirs, {0, 1}, 0);

        pio.tick();

        // SIDESET should override
        CHECK(pio.gpio.pindirs[0] == 0); // Override
        CHECK(pio.gpio.pindirs[1] == 0); // Override
        CHECK(pio.gpio.pindirs[2] == 1); // by SET PINDIRS
    }

    SUBCASE("Pindirs affects data output")
    {
        resetGpioArrays(-1);

        // Set pins 0,1,2 data to 1 by out_data
        setPins(pio.gpio.out_data, {0, 1, 2}, 1);

        // But set pin 1 to be input by out_pindirs
        setPins(pio.gpio.out_pindirs, {1}, 1);

        pio.tick();

        CHECK(pio.gpio.raw_data[0] == 1);
        CHECK(pio.gpio.raw_data[1] == 0); // not set because is an input
        CHECK(pio.gpio.raw_data[2] == 1);
    }

    SUBCASE("Complex priority test with all pindirs sources")
    {
        resetGpioArrays(-1);

        // conflicting pin directions
        setPins(pio.gpio.out_pindirs, {0, 1, 2, 3}, 0);
        setPins(pio.gpio.set_pindirs, {1, 2, 3, 4}, 1);
        setPins(pio.gpio.sideset_pindirs, {2, 3, 4, 5}, 0);

        // data values
        setPins(pio.gpio.out_data, {0, 1, 2, 3, 4, 5}, 1);

        pio.tick();

        // Check pin directions
        CHECK(pio.gpio.pindirs[0] == 0); // Set by OUT
        CHECK(pio.gpio.pindirs[1] == 1); // SET override OUT
        CHECK(pio.gpio.pindirs[2] == 0); // SIDESET override SET
        CHECK(pio.gpio.pindirs[3] == 0); // SIDESET override SET
        CHECK(pio.gpio.pindirs[4] == 0); // SIDESET override SET
        CHECK(pio.gpio.pindirs[5] == 0); // Set by SIDESET

        // Check raw_data
        // Only outputs should get 1 from out_data
        CHECK(pio.gpio.raw_data[0] == 1);
        CHECK(pio.gpio.raw_data[1] == 0); // Input (not changed)
        CHECK(pio.gpio.raw_data[2] == 1);
        CHECK(pio.gpio.raw_data[3] == 1);
        CHECK(pio.gpio.raw_data[4] == 1);
        CHECK(pio.gpio.raw_data[5] == 1);
    }

    SUBCASE("External data takes highest priority even when pin is an output")  // TODO: Check if this is true IRL (push-pull output)
    {
        resetGpioArrays(-1);

        std::fill(pio.gpio.pindirs.begin(), pio.gpio.pindirs.end(), 0);

        setPins(pio.gpio.out_data, {0, 1}, 0);

        setPins(pio.gpio.external_data, {0}, 1);

        pio.tick();

        CHECK(pio.gpio.raw_data[0] == 1); // External wins
        CHECK(pio.gpio.raw_data[1] == 0); // Set by out_data
    }

    SUBCASE("Unmodified values (-1) are ignored")
    {
        resetGpioArrays(-1);
        std::fill(pio.gpio.pindirs.begin(), pio.gpio.pindirs.end(), 0);

        std::fill(pio.gpio.raw_data.begin(), pio.gpio.raw_data.end(), 0); // raw_data to all 0

        setPins(pio.gpio.out_data, {1, 3}, 1);

        pio.tick();

        checkPins(pio.gpio.raw_data, {1, 3}, 1);
        checkPinsNotIn(pio.gpio.raw_data, {1, 3}, 0);
    }
}
