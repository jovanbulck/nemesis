#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>
#include <sancus_support/sancus_step.h>
#include "foo-signed-mod.h"

int main()
{
    msp430_io_init();
    sancus_enable(&foo);
    init_foo();
    
    sancus_step_start();
    volatile int rv = leak_foo();
    sancus_step_end();
    
    printf("All done!\n");
    EXIT();
}

/* ======== TIMER A ISR ======== */
SANCUS_STEP_ISR_ENTRY(sancus_step_print_latency)

