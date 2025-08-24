#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

TEST_CASE("push instruction test")
{
    PioStateMachine pio;

    SUBCASE("push block")
    {
        pio.instructionMemory[0] = 0x8020; // push block
        pio.regs.isr = 0x00abcdef;
        pio.regs.isr_shift_count = 20;
        pio.rx_fifo_count = 0;
        pio.tick();
        CHECK(pio.regs.isr == 0);  // should be cleared
        CHECK(pio.rx_fifo_count == 1);
    }

    SUBCASE("push block - with full fifo")
    {
        // should stall when rc_fifo is full
        pio.instructionMemory[0] = 0x8020; // push block
        pio.regs.isr = 0x00abcdef;
        pio.regs.isr_shift_count = 20;
        pio.rx_fifo_count = 4;

        pio.tick();  // should stall
        CHECK(pio.push_is_stalling == true);
        CHECK(pio.regs.isr == 0x00abcdef);  // should not changed
        CHECK(pio.regs.isr_shift_count == 20);
        CHECK(pio.rx_fifo_count == 4);

        pio.rx_fifo_count = 3;  

        pio.tick();  // should push
        CHECK(pio.regs.isr == 0);  // should be cleared
        CHECK(pio.rx_fifo_count == 4);
        CHECK(pio.rx_fifo[3] == 0x00abcdef);
    }

    SUBCASE("push ifempty block ")
    {
        // IfFull should do nothing if the isr haven't reached push_thresh
        pio.instructionMemory[0] = 0x8060;  // push  iffull block   
        pio.instructionMemory[1] = 0x8060;  // push  iffull block   
        // TODO: test with 'in' instruction

        pio.regs.isr = 0x00abcdef;
        pio.regs.isr_shift_count = 20;
        pio.settings.push_threshold = 32;
        pio.settings.autopush_enable = false; // disable auto push
        pio.settings.in_shift_autopush = false;

        pio.rx_fifo[0] = 0xb16b00b;
        pio.rx_fifo_count = 1;

        pio.tick(); // should do nothing
        CHECK(pio.regs.pc == 1);
        CHECK(pio.rx_fifo_count == 1); // no change
        CHECK(pio.rx_fifo[0] == 0xb16b00b);
        CHECK(pio.regs.isr == 0x00abcdef);
        CHECK(pio.regs.isr_shift_count == 20);

        pio.regs.isr = 0xdeadbeef;  // isr full
        pio.regs.isr_shift_count = 32;

        pio.tick(); // should do push
        CHECK(pio.regs.pc == 2);
        CHECK(pio.regs.isr == 0);  // cleared
        CHECK(pio.regs.isr_shift_count == 0);
        CHECK(pio.rx_fifo_count == 2); // pushed in
        CHECK(pio.rx_fifo[1] == 0xdeadbeef); // pushed in
    }

    SUBCASE("push noblock with empty")
    {
        // noblock will push whatever in isr to fifo
        pio.instructionMemory[0] = 0x8080; //  2: pull   noblock  
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 0;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 0;  // Fifo empty
        pio.tick();
        CHECK(pio.regs.osr == 0);
        CHECK(pio.tx_fifo_count == 0);
    }
#if 0
    SUBCASE("pull noblock with not-empty")
    {
        // noblock will pull from empty fifo to osr
        pio.instructionMemory[0] = 0x8080; //  2: pull   noblock  
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 0;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;  // Fifo not empty
        pio.tick();
        CHECK(pio.regs.osr == 0xdeadbeef);
        CHECK(pio.tx_fifo_count == 0);
    }

    SUBCASE("pull ifempty noblock - not empty")
    {
        // ifempty: only pull when osr is empty
        // s3.4.7.2: non-blocking pull on empty txfifo should have same effect with 'mov osr, x'
        pio.instructionMemory[0] = 0x80c0; // pull ifempty noblock   
        pio.regs.osr = 0x000abc;
        pio.regs.x = 0xCAFEBABE;
        pio.settings.pull_threshold = 20;
        pio.regs.osr_shift_count = 8;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;
        pio.tick();
        CHECK(pio.regs.osr == 0x000abc); // should not pull, osr not empty
        CHECK(pio.tx_fifo_count == 1);
    }

    SUBCASE("pull ifempty noblock - not empty 2")
    {
        // ifempty: only pull when osr is empty
        pio.instructionMemory[0] = 0x80c0; // pull ifempty noblock   
        pio.instructionMemory[1] = 0x606c, // out    null, 12    
        pio.instructionMemory[2] = 0x80c0; // pull ifempty noblock   

        pio.regs.osr = 0x000abc;
        pio.regs.x = 0xCAFEBABE;
        pio.settings.pull_threshold = 20;
        pio.settings.autopull_enable = false;
        pio.regs.osr_shift_count = 8;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;

        pio.tick(); // pull ifempty noblock  
        CHECK(pio.regs.osr == 0x000abc); // should not pull, osr not empty
        CHECK(pio.tx_fifo_count == 1);

        pio.tick(); // out null, 12    This should empty osr
        CHECK(pio.regs.osr == 0);
        CHECK(pio.regs.osr_shift_count == 20);

        pio.tick(); // pull ifempty noblock  Should Pull
        CHECK(pio.regs.osr == 0xdeadbeef);
        CHECK(pio.regs.osr_shift_count == 0);
        CHECK(pio.tx_fifo_count == 0);
    }

    SUBCASE("pull ifempty noblock - empty")
    {
        // s3.4.7.2: non-blocking pull on empty txfifo should have same effect with 'mov osr, x'
        pio.instructionMemory[0] = 0x80c0; // pull ifempty noblock   
        pio.regs.osr = 0x00abcdef;
        pio.regs.x = 0xCAFEBABE;
        pio.regs.osr_shift_count = 0;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 0;  // fifo empty
        pio.tick();
        CHECK(pio.regs.osr == 0xCAFEBABE); // x to osr
        CHECK(pio.tx_fifo_count == 0);
    }
#endif
}
