#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>
#include <sancus_support/sancus_step.h>
#include "foo-unsigned-mod.h"

int main()
{
    msp430_io_init();
    sancus_enable(&foo);
    init_foo();
    
    __ss_start();
    volatile int rv = leak_foo();
    
    printf("All done!\n");
    EXIT();
}

/* ======== TIMER A ISR ======== */
SANCUS_STEP_ISR_ENTRY(__ss_print_latency, __ss_end)
