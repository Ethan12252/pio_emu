#include <fmt/core.h>
#include <fmt/ranges.h>
#include <vector>
#include <bitset>
#include "TimingDiagram.h"

TimingDiagram::TimingDiagram(const uint32_t& var, const std::vector<int>& bits)
    : watchedVar(var), bitsToTrack(bits) {
}

// Record the current state of the bits
void TimingDiagram::tick() 
{
    std::vector<bool> currentBits;
    for (int bit : bitsToTrack) 
        currentBits.push_back((watchedVar >> bit) & 1); // Extract bit value
    
    history.push_back(currentBits); // Append to history
}

// Display the timing diagram 
void TimingDiagram::display() const 
{
    // Print Separator
    for (size_t i = 0; i < history.size() * 3 + 10; i++)
        fmt::print("-");
    fmt::print("\n");
    
    // Print time (columns)
    fmt::print("Time:  ");
    for (size_t t = 0; t < history.size(); ++t) 
        fmt::print("{:>2} ", t);
    
    fmt::print("\n");

    // Print each bit's timeline
    for (size_t i = 0; i < bitsToTrack.size(); ++i) 
    {
        fmt::print("Bit {:>2}: ", bitsToTrack[i]);
        for (const auto& state : history) 
            fmt::print("{}  ", state[i] ? "1" : "0");
        
        fmt::print("\n");
    }
}


