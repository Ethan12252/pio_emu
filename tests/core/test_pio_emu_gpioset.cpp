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
        std::fill(sm.gpio.raw_data.begin(), sm.gpio.raw_data.end(), 0);
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
        {
            INFO("Checking pin ", pin, " expected value: ", (int)expected, " actual: ", (int)array[pin]);  // should contruct if the check fail
            CHECK(array[pin] == expected);
        }
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
        // Setup: All pin values are -1 (unmodified)
        resetGpioArrays(-1);

        // Call the function
        sm.setAllGpio();

        // Check all raw_data remains at 0 (initial value)
        checkPinsNotIn(sm.gpio.raw_data, {}, 0);
    }

    SUBCASE("GPIO priority: out/set (lowest priority)")
    {
        // Setup: Set pindirs to output
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // 0 = output

        // Set data through out_data (pins 0, 5)
        setPins(sm.gpio.out_data, {0, 5}, 1);

        // Set data through set_data (pins 3, 8)
        setPins(sm.gpio.set_data, {3, 8}, 1);

        // Call the function
        sm.setAllGpio();

        // Check that pins were set
        checkPins(sm.gpio.raw_data, {0, 3, 5, 8}, 1);

        // Check other pins remain unmodified
        std::vector<int> modifiedPins = {0, 3, 5, 8};
        checkPinsNotIn(sm.gpio.raw_data, modifiedPins, 0);
    }

    SUBCASE("GPIO priority: sideset (medium priority)")
    {
        // Setup
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // 0 = output

        // Set out_data
        setPins(sm.gpio.out_data, {0, 1}, 1);

        // Set sideset_data with conflicting value on pin 0
        setPins(sm.gpio.sideset_data, {0, 2}, 0);

        // Call the function
        sm.setAllGpio();

        // sideset should override out_data for pin 0
        CHECK(sm.gpio.raw_data[0] == 0); // sideset wins
        CHECK(sm.gpio.raw_data[1] == 1); // only set by out_data
        CHECK(sm.gpio.raw_data[2] == 0); // only set by sideset
    }

    SUBCASE("GPIO priority: external (highest priority)")
    {
        // Setup
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // 0 = output

        // Set out_data
        setPins(sm.gpio.out_data, {0, 1}, 1);

        // Set sideset_data
        setPins(sm.gpio.sideset_data, {0, 2}, 0);

        // Set external_data with conflicting values
        setPins(sm.gpio.external_data, {0, 1, 2}, 1);

        // Call the function
        sm.setAllGpio();

        // external should override everything
        CHECK(sm.gpio.raw_data[0] == 1); // external wins over sideset and out
        CHECK(sm.gpio.raw_data[1] == 1); // external wins over out
        CHECK(sm.gpio.raw_data[2] == 1); // external wins over sideset
    }

    SUBCASE("Pin directions affect output")
    {
        // Setup
        resetGpioArrays(-1);

        // Set some pins as inputs (1 = input)
        setPins(sm.gpio.pindirs, {1, 3, 5}, 1);

        // Set out_data for all pins
        for (int i = 0; i < 6; i++)
        {
            sm.gpio.out_data[i] = 1;
        }

        // Call the function
        sm.setAllGpio();

        // Check that output pins were set
        checkPins(sm.gpio.raw_data, {0, 2, 4}, 1);

        // Input pins should not be affected by out_data
        // Note: According to implementation, warnings would be logged
        checkPins(sm.gpio.raw_data, {1, 3, 5}, 0);
    }

    SUBCASE("External inputs override even when pin is output")
    {
        // Setup
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0); // all are outputs

        // Set out_data
        setPins(sm.gpio.out_data, {0, 1}, 0);

        // Set external_data with conflicting value
        setPins(sm.gpio.external_data, {0, 1}, 1);

        // Call the function
        sm.setAllGpio();

        // External should override even though pins are outputs
        // This would generate a warning according to implementation
        checkPins(sm.gpio.raw_data, {0, 1}, 1);
    }

    SUBCASE("Unmodified values (-1) are ignored")
    {
        // Setup
        resetGpioArrays(-1);
        std::fill(sm.gpio.pindirs.begin(), sm.gpio.pindirs.end(), 0);

        // Set initial raw_data
        std::fill(sm.gpio.raw_data.begin(), sm.gpio.raw_data.end(), 0);

        // Only modify specific pins
        setPins(sm.gpio.out_data, {1, 3}, 1);
        // Leave other pins unmodified (-1)

        // Call the function
        sm.setAllGpio();

        // Check only modified pins changed
        checkPins(sm.gpio.raw_data, {1, 3}, 1);

        // Other pins should remain at initial value
        std::vector<int> modifiedPins = {1, 3};
        checkPinsNotIn(sm.gpio.raw_data, modifiedPins, 0);
    }
}
