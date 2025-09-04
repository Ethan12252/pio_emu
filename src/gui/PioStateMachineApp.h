
#include "../PioStateMachine.h" 

class PioStateMachineApp {
public:
    PioStateMachine pio;
    
    // GUI state
    bool show_main_window = true;
    bool show_controls = true;
    bool done = false;
    int tick_steps = 1;  // For multi-tick

    int programSize;

    
    void initialize();
    void reset();
    void renderUI();
};