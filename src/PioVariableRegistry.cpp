#include "PioStateMachine.h"

#define REGISTER_VAR(name, member) \
    var_getters[#name] = [this]() { return static_cast<uint32_t>(member); }; \
    var_setters[#name] = [this](uint32_t val) { member = static_cast<decltype(member)>(val); }

void PioStateMachine::setup_var_access() {
    // Core execution state
    REGISTER_VAR(pc, regs.pc);
    REGISTER_VAR(clock, clock);
    REGISTER_VAR(x, regs.x);
    REGISTER_VAR(y, regs.y);

    // FIFO state
    REGISTER_VAR(tx_fifo_count, fifo.tx_fifo_count);
    REGISTER_VAR(rx_fifo_count, fifo.rx_fifo_count);

    // GPIO pins (programmatic registration)
    for (int i = 0; i < 32; ++i) {
        std::string pin_name = "gpio" + std::to_string(i);
        std::string dir_name = "pindir" + std::to_string(i);

        var_getters[pin_name] = [this, i]() {
            return static_cast<uint32_t>(gpio.raw_data[i]);
            };
        var_setters[pin_name] = [this, i](uint32_t val) {
            gpio.raw_data[i] = static_cast<int8_t>(val & 1);
            };

        var_getters[dir_name] = [this, i]() {
            return static_cast<uint32_t>(gpio.pindirs[i]);
            };
    }
}