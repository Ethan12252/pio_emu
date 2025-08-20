#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <algorithm>
#include "../../src/PioStateMachine.h"

// Test fixture for PioStateMachine tests
class PioSideSetFixture
{
public:
    const uint16_t nop_inst = (0b101 << 13) | (0b010 << 5) | (0b00 << 3) | 0b010;
protected:
    PioStateMachine pio;

    // Helper to reset GPIO arrays
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
    void setPins(std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t value)
    {
        for (int pin : pins)
        {
            if (pin >= 0 && pin < 32)
                array[pin] = value;
        }
    }

    // Helper to check if all 'pins' in 'array' have the value of expected
    void checkPins(const std::array<int8_t, 32>& array, const std::vector<int>& pins, int8_t expected)
    {
        for (int pin : pins)
        {
            INFO("Checking pin ", pin, " expected value: ", expected, " actual: ", array[pin]);
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
                INFO("Checking pin ", i, " expected value: ", expected, " actual: ", array[i]);
                CHECK(array[i] == expected);
            }
        }
    }
};

TEST_CASE_FIXTURE(PioSideSetFixture, "PioStateMachine::doSideSet tests")
{
    SUBCASE("Side-set with count = 0 shouldn't affect pins")
    {
        resetGpioArrays(-1);
        pio.settings.sideset_count = 0;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = -1;
        pio.settings.sideset_to_pindirs = false;

        pio.doSideSet(0x1F);

        // should not be modified
        checkPinsNotIn(pio.gpio.sideset_data, {}, -1);
    }

    SUBCASE("Side-set count = 1, no enable bit")
    {
        resetGpioArrays(-1);

        pio.settings.sideset_count = 1;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 5; // Starts at pin 5
        pio.settings.sideset_to_pindirs = false;

        // side-set bits 0b00001
        pio.doSideSet(0b00001);
        checkPins(pio.gpio.sideset_data, {5}, 1);
        checkPinsNotIn(pio.gpio.sideset_data, {5}, -1); // pins other than 5 should not be modified

        // side-set bits 0b00000
        resetGpioArrays(-1);
        pio.doSideSet(0b00000);
        checkPins(pio.gpio.sideset_data, {5}, 0);
        checkPinsNotIn(pio.gpio.sideset_data, {5}, -1); // others should not be modified
    }

    SUBCASE("Side-set count = 3, no enable bit")
    {
        resetGpioArrays(-1);

        pio.settings.sideset_count = 3;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 10; // Starts at pin 10
        pio.settings.sideset_to_pindirs = false;

        // sideset 0b00100
        pio.doSideSet(0b00100);

        // pins should set
        checkPins(pio.gpio.sideset_data, {12}, 1);
        checkPins(pio.gpio.sideset_data, {10, 11}, 0);

        // others should not be modified
        checkPinsNotIn(pio.gpio.sideset_data, {10, 11, 12}, -1);
    }

    SUBCASE("Side-set count = 5 , no enable bit")
    {
        resetGpioArrays(-1);

        pio.settings.sideset_count = 5;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 2; // Starts at pin 2
        pio.settings.sideset_to_pindirs = false;

        // 0b10101 pins 2, 3, 4, 5, 6
        pio.doSideSet(0b10100);

        // pins 4, 6 should set
        checkPins(pio.gpio.sideset_data, {4, 6}, 1);
        checkPins(pio.gpio.sideset_data, {2, 3, 5}, 0);

        checkPinsNotIn(pio.gpio.sideset_data, {2, 3, 4, 5, 6}, -1);
    }

    SUBCASE("Side-set with enable bit (enabled)")
    {
        resetGpioArrays(-1);

        pio.settings.sideset_count = 3; // 2 data + 1 enable bit
        pio.settings.sideset_opt = true;  // enable sideset optional
        pio.settings.sideset_base = 8; // Starts at pin 8
        pio.settings.sideset_to_pindirs = false;

        pio.doSideSet(0b10001); // 0b10001 -> enable=1, data=01 TODO: Check opt bit encoding

        // Pins 8, 9 should be modded
        checkPins(pio.gpio.sideset_data, {8}, 1);
        checkPins(pio.gpio.sideset_data, {9}, 0);

        checkPinsNotIn(pio.gpio.sideset_data, {8, 9}, -1);
    }

    SUBCASE("Side-set with enable bit (disabled)")
    {
        resetGpioArrays(-1);

        pio.settings.sideset_count = 3; // 2 data + 1 enable
        pio.settings.sideset_opt = true;  // enable
        pio.settings.sideset_base = 8; // pin 8
        pio.settings.sideset_to_pindirs = false;

        pio.doSideSet(0x03); // 0b00011 → enable=0, data=11

        // No pins should be modded
        checkPinsNotIn(pio.gpio.sideset_data, {}, -1);
    }

    SUBCASE("Side-set to pin directions")
    {
        resetGpioArrays(-1);

        // 2 sideset bit without opt bit
        pio.settings.sideset_count = 2;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 12; // Starts at pin 12
        pio.settings.sideset_to_pindirs = true; // pindir

        pio.doSideSet(0b00010);

        // This should not modify sideset_data
        checkPinsNotIn(pio.gpio.sideset_data, {}, -1);

        // set pindirs for pins 12 and 13
        checkPins(pio.gpio.pindirs, {12}, 0);
        checkPins(pio.gpio.pindirs, {13}, 1);
    }

    SUBCASE("Side-set ignores bits beyond count")
    {
        resetGpioArrays(-1);

        // Configure SM with 2-bit side-set
        pio.settings.sideset_count = 2;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 20; // Starts at pin 20
        pio.settings.sideset_to_pindirs = false;

        // Call doSideSet with value that has extra bits set beyond count
        pio.doSideSet(0x1F); // 0b11111, but we only care about the lowest 2 bits (0b11)

        // Only pins 20 and 21 should be affected, both set to 1
        checkPins(pio.gpio.sideset_data, {20, 21}, 1);

        // Other pins should remain untouched
        checkPinsNotIn(pio.gpio.sideset_data, {20, 21}, -1);
    }

    SUBCASE("Side-set at end of pin range wraps correctly")
    {
        resetGpioArrays(-1);

        // Configure SM with 3-bit side-set at near the end of pin range
        pio.settings.sideset_count = 3;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 30; // Starts at pin 30
        pio.settings.sideset_to_pindirs = false;

        // Call doSideSet with value 0b111 (7 in decimal)
        pio.doSideSet(0x07);

        // Pins 30, 31 and 0 should be affected (wrapping from 31 to 0)
        checkPins(pio.gpio.sideset_data, {30, 31, 0}, 1);

        // Other pins should remain untouched
        checkPinsNotIn(pio.gpio.sideset_data, {30, 31, 0}, -1);
    }

    SUBCASE("Side-set priority against setAllGpio")
    {
        resetGpioArrays(-1);

        // Configure side-set
        pio.settings.sideset_count = 2;
        pio.settings.sideset_opt = false;
        pio.settings.sideset_base = 5;
        pio.settings.sideset_to_pindirs = false;

        // Set up conflicting values
        pio.doSideSet(0x03); // Set pins 5 and 6 to 1 via side-set

        // Set up conflicting OUT values
        setPins(pio.gpio.out_data, {5, 6}, 0);

        // Call setAllGpio to see priority in action
        pio.setAllGpio();

        // Side-set should override OUT
        checkPins(pio.gpio.raw_data, {5, 6}, 1);

        // However, external data should override side-set
        setPins(pio.gpio.external_data, {5}, 0);
        pio.setAllGpio();

        // Pin 5 should now be 0 (external), pin 6 should still be 1 (side-set)
        checkPins(pio.gpio.raw_data, {5}, 0);
        checkPins(pio.gpio.raw_data, {6}, 1);
    }
}
