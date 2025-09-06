#include "PioStateMachine.h"

#define REGISTER_VAR(name, member) \
    var_getters[name] = [this]() { return static_cast<uint32_t>(member); }; \
    var_setters[name] = [this](uint32_t val) { member = static_cast<decltype(member)>(val); }

void PioStateMachine::setup_var_access()
{
    // Registers
    REGISTER_VAR("pc", regs.pc);
    REGISTER_VAR("clock", clock);
    REGISTER_VAR("x", regs.x);
    REGISTER_VAR("y", regs.y);
    REGISTER_VAR("delay", regs.delay);
    REGISTER_VAR("isr", regs.isr);
    REGISTER_VAR("osr", regs.osr);
    REGISTER_VAR("isr_shift_count", regs.isr_shift_count);
    REGISTER_VAR("osr_shift_count", regs.osr_shift_count);

    REGISTER_VAR("irq_is_waiting", irq_is_waiting);
    REGISTER_VAR("pull_is_stalling", fifo.pull_is_stalling);
    REGISTER_VAR("push_is_stalling", fifo.push_is_stalling);
    REGISTER_VAR("wait_is_stalling", wait_is_stalling);

    // FIFOs
    REGISTER_VAR("tx_fifo_count", fifo.tx_fifo_count);
    REGISTER_VAR("rx_fifo_count", fifo.rx_fifo_count);
    for (int i = 0; i < 4; ++i)
    {
        // tx fifo
        std::string pin_name = "tx_fifo" + std::to_string(i);
        var_getters[pin_name] = [this, i]() {
            return static_cast<uint32_t>(fifo.tx_fifo[i]);
            };
        var_setters[pin_name] = [this, i](uint32_t val) {
            fifo.tx_fifo[i] = val;
            };

        // rx fifo
        pin_name = "rx_fifo" + std::to_string(i);
        var_getters[pin_name] = [this, i]() {
            return static_cast<uint32_t>(fifo.rx_fifo[i]);
            };
        var_setters[pin_name] = [this, i](uint32_t val) {
            fifo.rx_fifo[i] = val;
            };
    }

    // GPIO pins 
    for (int i = 0; i < 32; ++i)
    {
        // gpio raw data
        std::string pin_name = "gpio" + std::to_string(i);
        var_getters[pin_name] = [this, i]() {
            return static_cast<uint32_t>(gpio.raw_data[i]);
            };
        var_setters[pin_name] = [this, i](uint32_t val) {
            gpio.raw_data[i] = static_cast<int8_t>(val & 1);  // TODO: check this correct
            };

        // pindir
        std::string dir_name = "pindir" + std::to_string(i);
        var_getters[dir_name] = [this, i]() {
            return static_cast<uint32_t>(gpio.pindirs[i]);
            };
        var_setters[dir_name] = [this, i](uint32_t val) {
            gpio.pindirs[i] = static_cast<int8_t>(val);
            };
    }

    // IRQ
    for (int i = 0; i < irq_flags.size(); ++i)
    {
        // gpio raw data
        std::string pin_name = "irq" + std::to_string(i);
        var_getters[pin_name] = [this, i]() {
            return static_cast<uint32_t>(irq_flags[i]);
            };
        var_setters[pin_name] = [this, i](uint32_t val) {
            irq_flags[i] = static_cast<bool>(val & 1);
            };
    }
}

uint32_t PioStateMachine::get_var(const std::string& name) const {
    auto it = var_getters.find(name);
    if (it != var_getters.end())
        return it->second();
    return 0;
}

void PioStateMachine::set_var(const std::string& name, uint32_t value) {
    auto it = var_setters.find(name);
    if (it != var_setters.end()) {
        it->second(value);
    }
}

std::vector<std::string> PioStateMachine::get_available_set_vars() const
{
    std::vector<std::string> keys;
    keys.reserve(var_setters.size());  // Preallocate memory

    for (const auto& pair : var_setters)
        keys.push_back(pair.first);

    return keys;
}

std::vector<std::string> PioStateMachine::get_available_get_vars() const
{
    std::vector<std::string> keys;
    keys.reserve(var_getters.size());  // Preallocate memory

    for (const auto& pair : var_getters)
        keys.push_back(pair.first);

    return keys;
}

bool PioStateMachine::run_until_var(const std::string& var_name, uint32_t target, int max_cycles) {
    for (int i = 0; i < max_cycles; ++i)
    {
        auto var = get_var(var_name);
        //fmt::println("***value: {} ***pc: {}", var, regs.pc);
        if (var == target)
            return true;
        else
            tick();
    }
    return false;
}