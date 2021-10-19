#include <msp430.h>
#include <stdio.h>
#include <sancus/sm_support.h>
#include <sancus_support/sm_io.h>
#include <sancus_support/timer.h>
#include <sancus_support/sancus_step.h>
#include "foo-mul.h"

int main()
{
    msp430_io_init();
    sancus_enable(&foo);
    init_foo();

    __ss_start();
    volatile int rv = leak_foo();
    
    FINISH();
}

/* ======== TIMER A ISR ======== */
SANCUS_STEP_ISR_ENTRY2(__ss_print_latency, __ss_end)

