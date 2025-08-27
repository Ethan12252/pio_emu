#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "../../src/PioStateMachine.h"

TEST_CASE("pull instruction test")
{
    PioStateMachine pio;

    SUBCASE("pull block")
    {
        pio.instructionMemory[0] = 0x80a0; //  0: pull block
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 0;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;
        pio.tick();
        CHECK(pio.regs.osr == 0xdeadbeef);
        CHECK(pio.tx_fifo_count == 0);
    }

    SUBCASE("pull ifempty block ")
    {
        // ifempty should do nothing if the osr_shift_count havent reach pull_thres (like autopull)
        pio.instructionMemory[0] = 0x80e0;  // pull ifempty block   
        pio.instructionMemory[1] = 0x607e;  // out null, 30 
        pio.instructionMemory[2] = 0x0000;  // jmp 0
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 8;
        pio.settings.pull_threshold = 32;
        pio.settings.autopull_enable = false; // disable auto pull
        pio.settings.out_shift_right = true;

        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 1;

        pio.tick(); // should do nothing
        CHECK(pio.regs.pc == 1);
        CHECK(pio.regs.osr == 0x00abcdef);
        CHECK(pio.regs.osr_shift_count == 8);

        pio.tick(); // out osr to empty
        CHECK(pio.regs.pc == 2);
        CHECK(pio.regs.osr == 0);
        CHECK(pio.regs.osr_shift_count == 32);

        pio.tick(); // jump

        pio.tick(); // should do pull
        CHECK(pio.regs.osr == 0xdeadbeef);
        CHECK(pio.regs.osr_shift_count == 0);
        CHECK(pio.tx_fifo_count == 0);
    }
    
    SUBCASE("pull noblock with empty")
    {
        // noblock will pull from empty fifo to osr
        pio.instructionMemory[0] = 0x8080; //  2: pull   noblock  
        pio.regs.osr = 0x00abcdef;
        pio.regs.osr_shift_count = 0;
        pio.tx_fifo[0] = 0xdeadbeef;
        pio.tx_fifo_count = 0;  // Fifo empty
        pio.tick();
        CHECK(pio.regs.osr == 0);
        CHECK(pio.tx_fifo_count == 0);
    }

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
        pio.settings.out_shift_right = true;
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
}
