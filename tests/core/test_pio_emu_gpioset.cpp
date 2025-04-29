#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"
#include <algorithm>

// Test fixture for PioStateMachine tests
class PioStateMachineFixture
{
protected:
    PioStateMachine sm;

    // Helper to reset GPIO arrays
    // raw_data and pindir to 0, others to -1
    void resetGpioArrays(int8_t value = -1)
    {
        std::fill(sm.gpio.raw_data.begin(), sm.gpio.raw_data.end(), 0); // 0 by default
        std::fill(sm.gpio.set_data.begin(), sm.gpio.set_data.end(), value);
        std::fill(sm.gpio.out_data.begin(), sm.gpio.out_data.end(), value);
        std::fill(sm.gpio.external_data.begin(), sm.gpio.external_data.end(), value);
        std::fill(sm.gpio.sideset_data.begin(), sm.gpio.sideset_data.end(), value);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // 0 = output by default
    }

    // Helper to set all 'pins' in 'array' to value
    void setPins(std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t value)
    {
        for (int pin : pins)
        {
            if (pin >= 0 && pin < 32)
            {
                array[pin] = value;
            }
        }
    }

    // Helper to check if all 'pins' in 'array' have the value of expected
    void checkPins(const std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t expected)
    {
        for (int pin : pins)
            CHECK(array[pin] == expected);
    }

    // Helper to check if pins *NOT* in 'pins' have expected values
    void checkPinsNotIn(const std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t expected)
    {
        for (int i = 0; i < 32; i++)
        {
            if (std::find(pins.begin(), pins.end(), i) == pins.end())
            {
                INFO("Checking pin ", i, " expected value: ", (int)expected, " actual: ", (int)array[i]);
                CHECK(array[i] == expected);
            }
        }
    }
};

TEST_CASE_FIXTURE(PioStateMachineFixture, "PioStateMachine::setAllGpio tests")
{
    SUBCASE("Initial state should not modify raw_data")
    {
        resetGpioArrays(-1);

        sm.setAllGpio();

        // Check if raw_data remains 0 (initial value)
        checkPinsNotIn(sm.gpio.raw_data, {}, 0);
    }

    SUBCASE("GPIO priority: out/set (lowest)")
    {
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // pindirs to output

        // Set out_data pin 0, 5 to 1
        setPins(sm.gpio.out_data, {0, 5}, 1);

        // Set set_data pin 3, 8 to 1
        setPins(sm.gpio.set_data, {3, 8}, 1);

        sm.setAllGpio();

        checkPins(sm.gpio.raw_data, {0, 3, 5, 8}, 1);

        // Check if other pins remains unmodified
        checkPinsNotIn(sm.gpio.raw_data, {0, 3, 5, 8}, 0);
    }

    SUBCASE("GPIO priority: sideset (mid)")
    {
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // pindirs output

        // Set out_data pin 0, 1 to 1
        setPins(sm.gpio.out_data, {0, 1}, 1);

        // Set sideset_data with conflicting value on pin 0
        setPins(sm.gpio.sideset_data, {0, 2}, 0);

        sm.setAllGpio();

        // sideset should override out for pin 0
        CHECK(sm.gpio.raw_data[0] == 0); // sideset wins
        CHECK(sm.gpio.raw_data[1] == 1); // set by out_data
        CHECK(sm.gpio.raw_data[2] == 0); // set by sideset
    }

    SUBCASE("GPIO priority: external (highest priority)")
    {
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // output

        // side-set
        setPins(sm.gpio.out_data, {0, 1}, 1);
        // out
        setPins(sm.gpio.sideset_data, {0, 2}, 0);

        // conflict on pin 0, 2
        setPins(sm.gpio.external_data, {0, 1, 2}, 1);

        sm.setAllGpio();

        // external should override everything
        CHECK(sm.gpio.raw_data[0] == 1); // external wins over side-set and out
        CHECK(sm.gpio.raw_data[1] == 1);
        CHECK(sm.gpio.raw_data[2] == 1);
    }

    SUBCASE("Pin directions affect output")
    {
        resetGpioArrays(-1);

        // some pindirs as inputs
        setPins(sm.gpio.pindirs, {1, 3, 5}, 1);

        // Set out_data for all pins (pin0~5)
        for (int i = 0; i < 6; i++)
            sm.gpio.out_data[i] = 1;

        sm.setAllGpio();

        // Check if output pins were set
        checkPins(sm.gpio.raw_data, {0, 2, 4}, 1);

        // Input pins should not be modified
        checkPins(sm.gpio.raw_data, {1, 3, 5}, 0);
    }

    SUBCASE("External inputs should override even pindir is output")  // TODO: Check if this is true IRL
    {
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // all outputs

        setPins(sm.gpio.out_data, {0, 1}, 0);

        // Set external_data with conflicting value
        setPins(sm.gpio.external_data, {0, 1}, 1);

        sm.setAllGpio();

        // External inputs should override even pindir is output
        checkPins(sm.gpio.raw_data, {0, 1}, 1);
    }

    SUBCASE("Unmodified values (-1) are ignored")
    {
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0);

        std::fill(sm.gpio.raw_data.begin(), sm.gpio.raw_data.end(), 0);  // raw_data to all 0

        setPins(sm.gpio.out_data, {1, 3}, 1);

        sm.setAllGpio();

        checkPins(sm.gpio.raw_data, {1, 3}, 1);
        checkPinsNotIn(sm.gpio.raw_data, {1, 3}, 0);
    }
}
