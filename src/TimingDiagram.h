#pragma once
#include <vector>

class TimingDiagram
{
private:
    const uint32_t& watchedVar;       // Reference to the variable being monitored
    std::vector<int> bitsToTrack;    // Indices of the bits to track
    std::vector<std::vector<bool>> history; // History of tracked bit states

public:

    // var: what to track, bits: bits to track
    TimingDiagram(const uint32_t& var, const std::vector<int>& bits);

    // Will record the current state of the bits
    void tick();

    // Display the timing diagram
    void display() const;

};

/* Example usage

class Emulator {
public:
    uint32_t someVar = 0; // Example variable

    void tick() {
        // Example update logic
        someVar = (someVar + 1) ^ 0x3;
    }
};

int main() {
    Emulator emu;
    TimingDiagram d1(emu.someVar, { 1, 5, 9 }); // Track bits 1, 5, and 9

    // Simulate ticks
    for (int i = 0; i < 10; ++i) {
        emu.tick(); // Update the emulator
        d1.tick();  // Capture the bit states
        d1.display();
    }

    return 0;
}

*/