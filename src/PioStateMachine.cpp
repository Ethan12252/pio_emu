#include "PioStateMachine.h"
#include <iostream>
#include <bit>

typedef uint32_t u32;

void PioStateMachine::push_to_rx_fifo()
{
    // Push data to the rx fifo (assume already check if there is space)
    rx_fifo[rx_fifo_count] = regs.isr;
    rx_fifo_count++;
    regs.isr_shift_count = 0;
    regs.isr = 0;
}

void PioStateMachine::pull_from_tx_fifo()
{
    // Pull data from tx fifo (assume already check for space)
    regs.osr = tx_fifo[0];
    // shift the whole tx fifo
    for (int i = 0; i < tx_fifo_count - 1; i++)
    {
        tx_fifo[i] = tx_fifo[i + 1];
    }
    tx_fifo[tx_fifo_count - 1] = 0; // clear the last element (got duplicated)
    tx_fifo_count--;
    regs.osr_shift_count = 0;
}

void PioStateMachine::tick()
{
}

void PioStateMachine::doSideSet()
{
}

void PioStateMachine::executeInstruction()
{
    // Get Opcode field (15:13)
    u32 opcode = (currentInstruction & 0xe0) >> 13;
    u32 isPush = (currentInstruction >> 7) & 1u; // bit 7 = 0 為 PUSH, = 1 為 PULL

    // Calculate the delay/sideset bits (5 total) (見s3.5.1 和 P.319)
    u32 delayBitCount = 5 - settings.sideset_count - settings.sideset_opt;

    // Do side set (s3.5.1: Sideset take place before the instrucion)
    PioStateMachine::doSideSet();

    switch (opcode)
    {
    case 0b000: // JMP
        PioStateMachine::executeJmp();
        break;
    case 0b001: // WAIT
        PioStateMachine::executeWait();
        break;
    case 0b010: // IN
        PioStateMachine::executeIn();
        break;
    case 0b011: // OUT
        PioStateMachine::executeOut();
        break;
    case 0b100: // PUSH or PULL
        if (isPush)
            PioStateMachine::executePush();
        else
            PioStateMachine::executePull();
        break;
    case 0b101: // MOV
        PioStateMachine::executeMov();
        break;
    case 0b110: // IRQ
        PioStateMachine::executeIrq();
        break;
    case 0b111: // SET
        PioStateMachine::executeSet();
        break;
    default:
        // Error: Unknow instruction
        std::cout << "ERROR: Invalid instruction opcode\n";
    }

    // When not pull or mov, do autopull (if enabled) (s3.5.4)
    // TODO: auto pull
}

void PioStateMachine::executeJmp()
{
    // Obtain Instruction fields
    // bit 7:5 -> condition
    u32 condition = (currentInstruction >> 5) & 0b111;
    // bit 4:0 -> Address
    u32 address = currentInstruction & 0b1'1111;

    bool doJump = false;

    switch (condition) // From s3.4.2.2
    {
    case 0b000: // No condition (Always jmp)
        doJump = true;
        break;
    case 0b001: // !X: scratch X zero
        if (regs.x == 0)
            doJump = true;
        break;
    case 0b010: // X--: scratch X non-zero, prior to decrement
        //before the decrement took place: if the register is initially nonzero, the branch is taken.  (s3.4.2.2)
        if (regs.x != 0)
            doJump = true;
        regs.x--; // JMP X-- and JMP Y-- always decrement scratch register
        break;
    case 0b011: // !Y: scratch Y zero
        if (regs.y == 0)
            doJump = true;
        break;
    case 0b100: // Y--: scratch Y non-zero, prior to decrement
        if (regs.y != 0)
            doJump = true;
        regs.y--;
        break;
    case 0b101: // X!=Y: scratch X not equal scratch Y
        if (regs.x != regs.y)
            doJump = true;
        break;
    case 0b110: // PIN: branch on input pin
        if (settings.jmp_pin == -1) // jmp_pin not set
        {
            std::cout << "WARN: jmp_pin isn't set before use in JMP pin, continuing";
            break;
        }
        if (gpio.raw_data[settings.jmp_pin] == 1)
            doJump = true; // Branch if the GPIO is high
        break;
    case 0b111: // !OSRE: output shift register not empty
        if (regs.osr_shift_count < settings.pull_threshold) // Compair with SHIFTCTRL_PULL_THRESH (P.324)
            doJump = true;
        break;
    default:
        // ERROR: Invalid condition
        std::cout << "ERROR: Invalid Jmp condition\n";
    }

    // Do jump
    if (doJump == true)
    {
        jmp_to = address;
        skip_increase_pc = true;
    }
}

void PioStateMachine::executeWait()
{
    // Obtain Instruction fields
    u32 polarity = (currentInstruction >> 7) & 1; // bit 7
    u32 source = (currentInstruction >> 5) & 0b11; // bit 6:5
    u32 index = currentInstruction & 0b1'1111; // bit 4:0

    bool condIsNotMet = true;
    switch (source)
    {
    case 0b00: // GPIO: wait for gpio input selected by index (absolute)
        if (gpio.raw_data[index] != polarity)
            condIsNotMet = true;
        break;
    case 0b01: // PIN: sm's input io mapping, index select which of the mapped bit to wait
        if (settings.in_base == -1)
        {
            std::cout << "WARN: in_base isn't set before use in wait pin, continuing";
            break;
        }
    // pin is selected by adding Index to the PINCTRL_IN_BASE configuration, modulo 32 (s3.4.3.2)
        if (gpio.raw_data[(settings.in_base + index) % 32] != polarity)
            condIsNotMet = true;
        break;
    case 0b10: // IRQ: wait for PIO IRQ flag selected by Index (見3.4.9.2)  TODO: Need check!(irq number calculation)
        {
            u32 index_msb = (index >> 4) & 1; // bit 4
            u32 irq_wait_num = index & 0b111; // The 3 LSBs specify an IRQ index from 0-7. (s3.4.9.2)
            //If the MSB is set, the sm ID (0…3) is added to the IRQ index, by way of modulo-4 addition on the "two LSBs" (s3.4.9.2)  TODO:Need spec check
            if (index_msb == 1)
            {
                u32 index_2lsbs = irq_wait_num & 0b11; // get the 2 LSBs
                irq_wait_num = (index_2lsbs + stateMachineNumber) % 4; // modulo-4 addition on the "two LSBs"
                irq_wait_num |= (index & 0b100); // Preserve bit 2
            }
            if (irq_flags[irq_wait_num] != polarity)
                condIsNotMet = true;
            else if (irq_flags[irq_wait_num] == polarity && polarity == 1)
                irq_flags[irq_wait_num] = false;
            // If Polarity is 1 IRQ flag is cleared by the sm when wait cond met. (s3.4.3.2)
            break;
        }
    case 0b11: // Reserved
        break;
    default:
        // ERROR: Invalid wait source
        std::cout << "ERROR: Invalid wait source\n";
    }

    if (condIsNotMet == true)
    {
        // wait:A WAIT instruction’s condition is not yet met (s3.2.4)
        skip_increase_pc = true;
        delay_delay = true;
    }
}

void PioStateMachine::executeIn()
{
    // If autopush enable sm should automatically push the ISR to RX FIFO when the ISR is
    // full (i.e.push_threshold met), but if th Rx FIFO is full the "in" instruction STALL
    if (push_is_stalling == true)  // TODO:Need function check
    {
        std::cout << "WARN: Push is stalling in 'IN' instruction\n";
        return;
    }

    // Obtain Instruction fields
    u32 source = (currentInstruction >> 5) & 0b111; // bit 7:5
    u32 bitCount = currentInstruction & 0b1'1111; // bit 4:0

    if (bitCount == 0)
        bitCount = 32; // 32 is encoded as 0b00000

    u32 mask = (1 << bitCount) - 1;
    u32 data = 0;

    switch (source)
    {
    case 0b000: // PINS, use in mapping (PINCTRL_IN_BASE)
        if (settings.in_base == -1)
        {
            std::cout << "WARN: in_base isn't set before use in 'in pin', continuing\n";
            return;
        }
    // Loop through the pins we need to read
        for (u32 i = 0; i < bitCount; i++)
        {
            // Calc the actual GPIO pin number (wrap around if > 31)
            u32 readPinNumber = (settings.in_base + i) % 32;
            // Shift the pin state to the correct position in data
            data |= ((gpio.raw_data[readPinNumber] & 1) << i);
        }
        break;
    case 0b001: // X
        data = regs.x & mask;
        break;
    case 0b010: // Y
        data = regs.y & mask;
        break;
    case 0b011: // NULL (all 0)
        data = 0;
        break;
    case 0b100: // Reserved
    case 0b101:
        break;
    case 0b110: // ISR
        data = regs.isr & mask;
        break;
    case 0b111: // OSR
        data = regs.osr & mask;
        break;
    default:
        std::cout << "WARN: 'IN' has unknown source, continuing\n";
        return;
    }

    // Shift the data into ISR
    if (settings.in_shift_right == true)
    {
        // right shift
        regs.isr = regs.isr >> bitCount;
        regs.isr |= (data << (32 - bitCount));
    }
    else
    {
        // left shift
        regs.isr = regs.isr << bitCount;
        regs.isr |= data;
    }

    // update ISR_shift_count
    regs.isr_shift_count += bitCount;
    if (regs.isr_shift_count >= 32) // TODO: Need spec check here! (P.338)
        regs.isr_shift_count = 32;

    // Auto push(if enabled) or stall
    if (settings.in_shift_autopush == true && regs.isr_shift_count >= settings.push_threshold)
    {
        if (rx_fifo_count < 4)
        {
            push_to_rx_fifo();
            push_is_stalling = false;
        }
        else
        {
            // stall
            skip_increase_pc = true;
            delay_delay = true;
            push_is_stalling = true;
        }
    }
}

void PioStateMachine::executeOut()
{
}

void PioStateMachine::executePush()
{
    // Obtain Instruction fields
    u32 ifFull = (currentInstruction >> 6) & 1; // bits 6
    u32 block = (currentInstruction >> 5) & 1; // bits 5

    // Check if there's space for rx fifo
    if (rx_fifo_count < 4)
    {
        // have space
        // IfFull: If 1, do nothing unless the total input shift count has reached its threshold,
        // SHIFTCTRL_PUSH_THRESH (the same as for autopush; see Section 3.5.4)
        if (ifFull)
        {
            if (regs.isr_shift_count >= settings.push_threshold)
                push_to_rx_fifo();
        }
        else
            push_to_rx_fifo();
    }
    else
    {
        // No space in rx fifo to push (block or skip?)
        if (block)
        {
            // stall (block)
            skip_increase_pc = true;
            delay_delay = true;
            push_is_stalling = true;
        }
        else
        {
            // not block
            // continue, but clear isr (s3.4.6.2)
            push_is_stalling = false;
            regs.isr = 0;
        }
    }
}

void PioStateMachine::executePull()
{
    // Obtain Instruction fields
    u32 ifEmpty = (currentInstruction >> 6) & 1; // bits 6
    u32 block = (currentInstruction >> 5) & 1; // bits 5

    // Check if tx fifo is empty (nothing to pull)
    if (tx_fifo_count != 0)
    {
        if (ifEmpty)
        {
            // Only pull if the osr counter is larger then the threshold
            if (regs.osr_shift_count >= settings.pull_threshold)
                pull_from_tx_fifo();
        }
        else
        {
            // Pull anyway (overwrite osr content)
            pull_from_tx_fifo();
        }
    }
    else
    {
        // tx fifo is empty (nothing to pull)
        if (block)
        {
            // stall (block)
            skip_increase_pc = true;
            delay_delay = true;
            pull_is_stalling = true;
        }
        else
        {
            // non-block pull
            // (s3.4.7.2): A nonblocking PULL on an empty FIFO has the same effect as 'MOV OSR, X'
            regs.osr = regs.x;
            pull_is_stalling = false;
            // LOG('WARN: a non-blocking PULL on an empty FIFO has the same effect as 'MOV OSR, X', continuing\n')
        }
    }
}

void PioStateMachine::executeMov()
{
    // Obtain Instruction fields
    u32 destination = (currentInstruction >> 5) & 0b111; // bits 7:5
    u32 op = (currentInstruction >> 3) & 0b11; // bits 4:3
    u32 source = currentInstruction & 0b111; // bits 2:0

    // data to be moved
    u32 data = 0;

    // --- Source Handling ---
    switch (source)
    {
    case 0b000: // PINS (use 'in' mapping)
        if (settings.in_base == -1)
        {
            std::cout << "WARN: in_base isn't set before use in 'mov dst, pin', continuing\n";
            return;
        }
    // Loop through the pins we need to read
        for (u32 i = 0; i < 32; i++)
        {
            // Calc the actual GPIO pin number (wrap around if > 31)
            u32 readPinNumber = (settings.in_base + i) % 32;
            // Shift the pin state to the correct position in data
            data |= ((gpio.raw_data[readPinNumber] & 1) << i);
        }
        break;
    case 0b001: // X
        data = regs.x;
        break;
    case 0b010: // Y
        data = regs.y;
        break;
    case 0b011: // NULL
        data = 0;
        break;
    case 0b100: //Reserved
        // TODO:Log unknow source
        return;
    case 0b101: // STATUS
        data = regs.status; // Assumes regs.status is pre-configured
        break;
    case 0b110: //ISR
        data = regs.isr;
        break;
    case 0b111: // OSR
        data = regs.osr;
        break;
    //default:
    // TODO: Log unknow source
    }

    // --- Operation ---
    // Apply the operation on data
    switch (op)
    {
    case 0b00: // none
        break;
    case 0b01: // invert (bit-wise not)
        data = ~data;
        break;
    case 0b10: // bit-reverse
        {
            u32 old_data = data;
            data = 0;
            for (int i = 0; i < 32; i++) // function checked
            {
                data |= ((old_data >> i) & 1) << (31 - i);
            }
            break;
        }
    case 0b11: // reserved
        break;
    //TODO: add default to log unknow option
    }

    // --- Destination ---
    // Place data to destination
    switch (destination)
    {
    case 0b000: // PINS (use 'out' mapping)
        if (settings.out_base == -1)
        {
            std::cout << "WARN: out_base isn't set before use in 'mov pin, src, continuing\n";
            return;
        }
        if (settings.out_count == -1)
        {
            std::cout << "WARN: out_count isn't set before use in 'mov pin, src, continuing\n";
            return;
        }
    // Loop through the pins we need to write
        for (u32 i = 0; i < settings.out_count; i++)
        // P.337 OUT_COUNT: The number of pins asserted by ... MOV PINS instruction.
        {
            // Calc the actual GPIO pin number (wrap around if > 31)
            u32 writePinNumber = (settings.out_base + i) % 32;
            gpio.out_data[writePinNumber] = (data & (1 << i)) ? 1 : 0;
        }
        break;
    case 0b001: // X
        regs.x = data;
        break;
    case 0b010: // Y
        regs.y = data;
        break;
    case 0b011: // Reserved
        // TODO:Log unknow source
        return;
    case 0b100: // EXEC  TODO: Need function check!!
        skip_increase_pc = true;
        skip_delay = true;
        exec_command = true;
        currentInstruction = data; // Next instruction (we dont increace pc next cycle)
        break;
    case 0b101: // PC
        skip_increase_pc = true;
        jmp_to = data & 0b1'1111; // 0 ~ 32
        break;
    case 0b110: //ISR
        regs.isr = data;
        regs.isr_shift_count = 0;
        break;
    case 0b111: // OSR
        regs.osr = data;
        regs.osr_shift_count = 0;
        break;
    //default:
    // TODO: Log unknow dest
    }
}

void PioStateMachine::executeIrq()
{
    // Obtain Instruction fields
    u32 clear = (currentInstruction >> 5) & 1; // bit 5
    u32 wait = (currentInstruction >> 6) & 1; // bit 6
    u32 index = currentInstruction & 0b1'1111; // bits 4:0
    u32 index_msb = (index >> 4) & 1; // bit 4

    u32 irqNum = index & 0b111; // The 3 LSBs specify an IRQ index from 0-7. (s3.4.9.2)
    //If the MSB is set, the sm ID (0…3) is added to the IRQ index, by way of modulo-4 addition on the "two LSBs" (s3.4.9.2)  // TODO:Need spec check
    if (index_msb == 1)
    {
        u32 index_2lsbs = irqNum & 0b11; // get the 2 LSBs
        irqNum = (index_2lsbs + stateMachineNumber) % 4; // modulo-4 addition on the "two LSBs"
        irqNum |= (index & 0b100); // Preserve bit 2
    }

    // TODO: Controll flow Need spec check!
    // The irq instruction is already waiting for clearing
    if (irq_is_waiting == true)
    {
        // Check if we need to keep waiting
        if (irq_flags[irqNum] == true)
        {
            // Still waiting for IRQ to clear
            skip_increase_pc = true;
            delay_delay = true;
            return;
        }
        else
        {
            // Irq have been cleared
            irq_is_waiting = false;
            return;
        }
    }

    if (clear == 1)
        irq_flags[irqNum] = false; // clear irq
    else
    {
        // set irq
        irq_flags[irqNum] = true;
        if (wait == 1)
        {
            irq_is_waiting = true;
            skip_increase_pc = true;
            delay_delay = true;
        }
    }
}

void PioStateMachine::executeSet()
{
    // Obtain Instruction fields
    u32 destinition = (currentInstruction >> 5) & 0b111; // bit 7:5
    u32 data = currentInstruction & 0b1'1111; // bit 4:0

    switch (destinition)
    {
    case 000: // PINS
        if (settings.set_base == -1)
            std::cout << "WARN: 'set_base' isn't set before use in SET instruction, continuing\n";
        if (settings.set_count == -1)
            std::cout << "WARN: 'set_count' isn't set before use in SET instruction, continuing\n";
        else
        {
            for (int i = 0; i < settings.set_count; i++)
            {
                u32 setPin = (settings.set_base + i) % 32;
                gpio.set_data[setPin] = (data & (1 << i)) ? 1 : 0;
            }
        }
        break;
    case 001: // X
        regs.x = data;
        break;
    case 010: // Y
        regs.y = data;
        break;
    case 011: // Reserved
        break;
    case 100: // PINDIRS
        if (settings.set_base == -1)
            std::cout << "WARN: 'set_base' isn't set before use in SET instruction, continuing\n";
        if (settings.set_count == -1)
            std::cout << "WARN: 'set_count' isn't set before use in SET instruction, continuing\n";
        else
        {
            for (int i = 0; i < settings.set_count; i++)
            {
                u32 setPin = (settings.set_base + i) % 32;
                gpio.pindirs[setPin] = (data & (1 << i)) ? 1 : 0;
            }
        }
        break;
    case 101: // Reserved
    case 110:
    case 111:
        break;
    default:
        "WARN: 'set' has unknown destination, continuing\n";
    }
}
