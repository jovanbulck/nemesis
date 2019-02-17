#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>
#include <sancus_support/sancus_step.h>
#include "foo-signed-div.h"

int main()
{
    msp430_io_init();
    sancus_enable(&foo);
    init_foo();
    
    SANCUS_STEP_INIT
    volatile int rv = leak_foo();
    SANCUS_STEP_END
    printf("All done!\n");
    EXIT();
}

/* ======== TIMER A ISR ======== */

/*
 * NOTE: we use a naked asm function here to be able to store IRQ latency.
 * (Timer_A continues counting from zero after IRQ generation)
 */
__attribute__((naked)) __attribute__((interrupt(TIMER_IRQ_VECTOR)))
void timerA_isr_entry(void)
{
    SANCUS_STEP_ISR(print_latency);
}

