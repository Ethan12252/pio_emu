#include "PioStateMachine.h"
#include  <iostream>
typedef uint32_t u32;

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
    // TODO: auto pull, do delay (Do we want to put delay here?)
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
        if (regs.x != 0)
        {
            doJump = true;
            regs.x--;
        }
        break;
    case 0b011: // !Y: scratch Y zero
        if (regs.y == 0)
            doJump = true;
        break;
    case 0b100: // Y--: scratch Y non-zero, prior to decrement
        if (regs.y != 0)
        {
            doJump = true;
            regs.y--;
        }
        break;
    case 0b101: // X!=Y: scratch X not equal scratch Y
        if (regs.x != regs.y)
            doJump = true;
        break;
    case 0b110: // PIN: branch on input pin
        if (settings.jmp_pin == -1) // jmp_pin not set
        {
            std::cout << "WARN: jmp_pin isn't set before use in JMP pin, continuing"
            break;
        }
        if (gpio.data_raw[settings.jmp_pin] == 1)
            doJump = 1; // Branch if the GPIO is high
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
        if (gpio.data_raw[index] != polarity)
            condIsNotMet = true;
        break;
    case 0b01: // PIN: sm's input io mapping, index select which of the mapped bit to wait
        if (settings.in_base == -1)
        {
            std::cout << "WARN: in_base isn't set before use in wait pin, continuing"
            break;
        }
    // pin is selected by adding Index to the PINCTRL_IN_BASE configuration, modulo 32 (s3.4.3.2)
        if (gpio.data_raw[(settings.in_base + index) % 32] != polarity)
            condIsNotMet = true;
        break;
    case 0b10: // IRQ: wait for PIO IRQ flag selected by Index (見3.4.9.2)  TODO: Need check!
        u32 irq_wait_num = index & 0b00111; // extract bit 2:0, irq number 0~7
        if ((index >> 4) & 1) // index MSB set, add state machine number to irq number
            irq_wait_num = (irq_wait_num + stateMachineNumber) % 4;
        if (irq_flags[irq_wait_num] != polarity)
            condIsNotMet = true;
        else if (rq_flags[irq_wait_num] == polarity && polarity == 1)
            irq_flags[irq_wait_num] = false; // If Polarity is 1 IRQ flag is cleared by the sm when wait cond met.
        break;
    case 0b11: // Reserved
        break;
    default:
        // ERROR: Invalid wait source
        std::cout << "ERROR: Invalid wait source\n";
    }

    if (condIsNotMet == true)
    {
        // wait
        skip_increase_pc = true;
        delayFlag = true; // TODO: Check if we want this
    }
}

void PioStateMachine::executeIn()
{
    // If autopush enable sm should automatically push the ISR to RX FIFO when the ISR is
    // full (i.e.push_threshold met), but if th Rx FIFO is full the "in" instruction STALL
    if (push_is_stalling == true)
    {
        std::cout << "WARN: Push is stalling in 'IN' instruction\n";
        return;
    }

    // Obtain Instruction fields
    u32 source = (currentInstruction >> 5) & 0b111; // bit 7:5
    u32 bitCount = currentInstruction & 0b1'1111; // bit 4:0

    if (bitCount == 0)
        bitCount = 32; // 32 is encoded as 0b00000

    u32 mask = (i << bitCount) - 1;
    u32 data = 0;

    switch (source)
    {
    case 0b000: // PINS, use in mapping (PINCTRL_IN_BASE)
        if (settings.in_base == -1)
            std::cout << "WARN: in_base isn't set before use in 'in pin', continuing\n"

    // Loop through the pins we need to read
        for (u32 i = 0; i < bitCount; i++)
        {
            // Calc the actual GPIO pin number (wrap around if > 31)
            u32 readPinNumber = (settings.in_base + i) % 32;
            // Shift the pin state to the correct position in data
            data |= (gpio.data_raw[readPinNumber] << i)
        }
        break;
    case 0b001: // X
        data = X | mask;
        break;
    case 0b010: // Y
        data = Y | mask;
        break;
    case 0b011: // NULL (all 0)
        data = 0;
        break;
    case 0b100: // Reserved
        break;
    case 0b101: // Reserved
        break;
    case 0b110: // ISR
        data = regs.ISR | mask;
        break;
    case 0b111: // OSR
        data = regs.OSR | mask;
        break;
    default:
        std::cout << "WARN: 'IN' has unknown source, continuing\n";
    }

    // Shift the data into ISR
    if (settings.in_shift_right == true)
    {
        // right shift
        regs.ISR = regs.ISR >> bitCount;
        regs.ISR |= (data << (32 - bitCount));
    } else
    {
        // left shift
        regs.ISR = regs.ISR << bitCount;
        regs.ISR |= data;
    }

    // update ISR_shift_count
    regs.ISR += bitCount;
    if (regs.ISR > settings.push_threshold)  // TODO: Need spec check here!
        regs.ISR = 0;

    // TODO: Auto Push

}

void PioStateMachine::executeOut()
{

}

void PioStateMachine::executePush()
{

}

void PioStateMachine::executePull()
{

}

void PioStateMachine::executeMov()
{

}

void PioStateMachine::executeIrq()
{

}

void PioStateMachine::executeSet()
{

}
