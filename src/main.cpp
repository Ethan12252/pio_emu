#include <iostream>
#include "PioStateMachine.h"
int main()
{
    PioStateMachine pio0;
    pio0.currentInstruction = 0b0000'0000'0010'0100;
    pio0.settings.sideset_count = 0;
    pio0.settings.sideset_opt = false;

    pio0.executeInstruction();
    return 0;
}