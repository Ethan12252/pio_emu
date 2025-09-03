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
        pio.fifo.rx_fifo_count = 0;
        pio.tick();
        CHECK(pio.regs.isr == 0);  // should be cleared
        CHECK(pio.fifo.rx_fifo_count == 1);
    }

    SUBCASE("push block - with full fifo")
    {
        // should stall when rc_fifo is full
        pio.instructionMemory[0] = 0x8020; // push block
        pio.regs.isr = 0x00abcdef;
        pio.regs.isr_shift_count = 20;
        pio.fifo.rx_fifo_count = 4;

        pio.tick();  // should stall
        CHECK(pio.fifo.push_is_stalling == true);
        CHECK(pio.regs.isr == 0x00abcdef);  // should not changed
        CHECK(pio.regs.isr_shift_count == 20);
        CHECK(pio.fifo.rx_fifo_count == 4);

        pio.fifo.rx_fifo_count = 3;  

        pio.tick();  // should push
        CHECK(pio.regs.isr == 0);  // should be cleared
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[3] == 0x00abcdef);
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

        pio.fifo.rx_fifo[0] = 0xb16b00b;
        pio.fifo.rx_fifo_count = 1;

        pio.tick(); // should do nothing
        CHECK(pio.regs.pc == 1);
        CHECK(pio.fifo.rx_fifo_count == 1); // no change
        CHECK(pio.fifo.rx_fifo[0] == 0xb16b00b);
        CHECK(pio.regs.isr == 0x00abcdef);
        CHECK(pio.regs.isr_shift_count == 20);

        pio.regs.isr = 0xdeadbeef;  // isr full
        pio.regs.isr_shift_count = 32;

        pio.tick(); // should do push
        CHECK(pio.regs.pc == 2);
        CHECK(pio.regs.isr == 0);  // cleared
        CHECK(pio.regs.isr_shift_count == 0);
        CHECK(pio.fifo.rx_fifo_count == 2); // pushed in
        CHECK(pio.fifo.rx_fifo[1] == 0xdeadbeef); // pushed in
    }

    SUBCASE("push noblock with full fifo")
    {
        // noblock when fifo is full, should just goto next instruction, the contents of fifo and isr is unchanged
        pio.instructionMemory[0] = 0x8000; // push noblock    
        pio.regs.isr = 0x00abcdef;
        pio.regs.isr_shift_count = 8;
        pio.fifo.rx_fifo[0] = 0xDEADBEEF;
        pio.fifo.rx_fifo[1] = 0xABADBABE;
        pio.fifo.rx_fifo[2] = 0xBEEFBABE;
        pio.fifo.rx_fifo[3] = 0xFACEFEED;
        pio.fifo.rx_fifo_count = 4;  // Fifo full

        pio.tick();
        CHECK(pio.regs.pc == 1);
        CHECK(pio.fifo.push_is_stalling == false);
        pio.tick();

        CHECK(pio.regs.pc == 2);

        // should not change
        CHECK(pio.regs.isr == 0);  // but clear isr (s3.4.6.2)
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[0] == 0xDEADBEEF);
        CHECK(pio.fifo.rx_fifo[1] == 0xABADBABE);
        CHECK(pio.fifo.rx_fifo[2] == 0xBEEFBABE);
        CHECK(pio.fifo.rx_fifo[3] == 0xFACEFEED);
        CHECK(pio.regs.isr_shift_count == 0);  
    }

    SUBCASE("push noblock with not full fifo")
    {
        // noblock when fifo is full, should just goto next instruction, the contents of fifo and isr is unchanged
        pio.instructionMemory[0] = 0x8000; // push noblock    
        pio.regs.isr = 0x00abcdef;
        pio.regs.isr_shift_count = 8;
        pio.fifo.rx_fifo[0] = 0xDEADBEEF;
        pio.fifo.rx_fifo[1] = 0xABADBABE;
        pio.fifo.rx_fifo[2] = 0xBEEFBABE;
        pio.fifo.rx_fifo[3] = 0xFACEFEED;
        pio.fifo.rx_fifo_count = 4;  // Fifo full

        pio.tick();
        CHECK(pio.regs.pc == 1);
        CHECK(pio.fifo.push_is_stalling == false);
        pio.tick();

        CHECK(pio.regs.pc == 2);

        // should not change
        CHECK(pio.regs.isr == 0);  // but clear isr (s3.4.6.2)
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[0] == 0xDEADBEEF);
        CHECK(pio.fifo.rx_fifo[1] == 0xABADBABE);
        CHECK(pio.fifo.rx_fifo[2] == 0xBEEFBABE);
        CHECK(pio.fifo.rx_fifo[3] == 0xFACEFEED);
        CHECK(pio.regs.isr_shift_count == 0);
    }

    SUBCASE("push noblock IfFull with not-full fifo and full isr")
    {
        // noblock: do nothing (nop) if rxfifo is full, but still clear isr (s3.4.6.2) 
        // iffull : do nothing unless the isr is full

        // rxfifo-not-full isr-full -> should push
        pio.instructionMemory[0] = 0x8040;  // push iffull noblock 
        pio.regs.isr = 0xFACEFEED;
        pio.settings.push_threshold = 20;
        pio.regs.isr_shift_count = 20;
        pio.fifo.rx_fifo[0] = 0xdeadbeef;
        pio.fifo.rx_fifo[1] = 0xdeadbee1;
        pio.fifo.rx_fifo[2] = 0xdeadbee2;
        pio.fifo.rx_fifo_count = 3;  // Fifo not empty

        pio.tick();  // should push normally

        CHECK(pio.regs.isr == 0);  // cleared
        CHECK(pio.regs.isr_shift_count == 0);  
        CHECK(pio.fifo.push_is_stalling == false); // should not stall
        // pushed
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[3] == 0xFACEFEED);
    }

    SUBCASE("push noblock IfFull with full fifo and full isr")
    {
        // noblock: do nothing (nop) if rxfifo is full, but still clear isr (s3.4.6.2)
        // iffull : do nothing unless the isr is full

        // rxfifo-full isr-full -> should NOT push
        pio.instructionMemory[0] = 0x8040;  // push iffull noblock 
        pio.regs.isr = 0xFACEFEED;
        pio.settings.push_threshold = 20;
        pio.regs.isr_shift_count = 20;
        pio.fifo.rx_fifo[0] = 0xdeadbeef;
        pio.fifo.rx_fifo[1] = 0xdeadbee1;
        pio.fifo.rx_fifo[2] = 0xdeadbee2;
        pio.fifo.rx_fifo[3] = 0xdeadbee3;
        pio.fifo.rx_fifo_count = 4;  // Fifo not empty

        pio.tick();  // should push normally

        CHECK(pio.regs.isr == 0);  // but still clear isr (s3.4.6.2)
        CHECK(pio.regs.isr_shift_count == 0);
        CHECK(pio.fifo.push_is_stalling == false); // should not stall
        // not pushed (unchanged)
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[0] == 0xdeadbeef);
        CHECK(pio.fifo.rx_fifo[1] == 0xdeadbee1);
        CHECK(pio.fifo.rx_fifo[2] == 0xdeadbee2);
        CHECK(pio.fifo.rx_fifo[3] == 0xdeadbee3);
    }

    SUBCASE("push noblock IfFull with full fifo and not-full isr")
    {
        // noblock: do nothing (nop) if rxfifo is full, but still clear isr (s3.4.6.2)
        // iffull : do nothing unless the isr is full

        // rxfifo-full isr-not-full -> should NOT push
        pio.instructionMemory[0] = 0x8040;  // push iffull noblock 
        pio.regs.isr = 0xFACEFEED;
        pio.settings.push_threshold = 20;
        pio.regs.isr_shift_count = 10;
        pio.fifo.rx_fifo[0] = 0xdeadbeef;
        pio.fifo.rx_fifo[1] = 0xdeadbee1;
        pio.fifo.rx_fifo[2] = 0xdeadbee2;
        pio.fifo.rx_fifo[3] = 0xdeadbee3;
        pio.fifo.rx_fifo_count = 4;  // Fifo not empty

        pio.tick();  // should not push normally

        CHECK(pio.regs.isr == 0);  // but still clear isr (s3.4.6.2)
        CHECK(pio.regs.isr_shift_count == 0);
        CHECK(pio.fifo.push_is_stalling == false); // should not stall
        // not pushed (unchanged)
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[0] == 0xdeadbeef);
        CHECK(pio.fifo.rx_fifo[1] == 0xdeadbee1);
        CHECK(pio.fifo.rx_fifo[2] == 0xdeadbee2);
        CHECK(pio.fifo.rx_fifo[3] == 0xdeadbee3);
    }

#if 0
    // Following is unfinished, just place holder
    SUBCASE("push noblock IfFull with not-full fifo and full isr")
    {
        // noblock: do nothing (nop) if rxfifo is full, but still clear isr (s3.4.6.2)
        // iffull : do nothing unless the isr is full

        // rxfifo-not-full isr-full -> should NOT push, presever isr
        pio.instructionMemory[0] = 0x8040;  // push iffull noblock 
        pio.regs.isr = 0xFACEFEED;
        pio.settings.push_threshold = 20;
        pio.regs.isr_shift_count = 10;
        pio.fifo.rx_fifo[0] = 0xdeadbeef;
        pio.fifo.rx_fifo[1] = 0xdeadbee1;
        pio.fifo.rx_fifo[2] = 0xdeadbee2;
        pio.fifo.rx_fifo[3] = 0xdeadbee3;
        pio.fifo.rx_fifo_count = 4;  // Fifo not empty

        pio.tick();  // should not push normally

        CHECK(pio.regs.isr == 0);  // but still clear isr (s3.4.6.2)
        CHECK(pio.regs.isr_shift_count == 0);
        CHECK(pio.fifo.push_is_stalling == false); // should not stall
        // not pushed (unchanged)
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[0] == 0xdeadbeef);
        CHECK(pio.fifo.rx_fifo[1] == 0xdeadbee1);
        CHECK(pio.fifo.rx_fifo[2] == 0xdeadbee2);
        CHECK(pio.fifo.rx_fifo[2] == 0xdeadbee3);
    }

    SUBCASE("push noblock IfFull with not-full fifo and not-full isr")
    {
        // noblock: do nothing (nop) if rxfifo is full, but still clear isr (s3.4.6.2)
        // iffull : do nothing unless the isr is full

        // rxfifo-not-full isr-not-full -> should NOT push, presever isr
        pio.instructionMemory[0] = 0x8040;  // push iffull noblock 
        pio.regs.isr = 0xFACEFEED;
        pio.settings.push_threshold = 20;
        pio.regs.isr_shift_count = 10;
        pio.fifo.rx_fifo[0] = 0xdeadbeef;
        pio.fifo.rx_fifo[1] = 0xdeadbee1;
        pio.fifo.rx_fifo[2] = 0xdeadbee2;
        pio.fifo.rx_fifo[3] = 0xdeadbee3;
        pio.fifo.rx_fifo_count = 4;  // Fifo not empty

        pio.tick();  // should not push normally

        CHECK(pio.regs.isr == 0);  // but still clear isr (s3.4.6.2)
        CHECK(pio.regs.isr_shift_count == 0);
        CHECK(pio.fifo.push_is_stalling == false); // should not stall
        // not pushed (unchanged)
        CHECK(pio.fifo.rx_fifo_count == 4);
        CHECK(pio.fifo.rx_fifo[0] == 0xdeadbeef);
        CHECK(pio.fifo.rx_fifo[1] == 0xdeadbee1);
        CHECK(pio.fifo.rx_fifo[2] == 0xdeadbee2);
        CHECK(pio.fifo.rx_fifo[2] == 0xdeadbee3);
    }
#endif
}
