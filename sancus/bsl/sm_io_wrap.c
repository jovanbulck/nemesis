#include "sm_io_wrap.h"
#include <msp430.h>

#ifndef NODEBUG

#include <stdio.h>
#ifndef __SANCUS_SIM
    #include <sancus_support/uart.h>
#endif

int putchar(int c)
{
#if __SANCUS_SIM
    // simulator
    P1OUT = c;
    P1OUT |= 0x80;
#else
    // uart
    if (c == '\n')
        putchar('\r');

    uart_write_byte(c);
#endif

    return c;
}

void __attribute__((noinline)) printf0(const char* str)
{
    printf("%s", str);
}

void __attribute__((noinline)) printf1(const char* fmt, int arg1)
{
    printf(fmt, arg1);
}

void __attribute__((noinline)) printf2(const char* fmt, int arg1, int arg2)
{
    printf(fmt, arg1, arg2);
}

void __attribute__((noinline)) printf3(const char* fmt, int arg1, int arg2, int arg3)
{
    printf(fmt, arg1, arg2, arg3);
}

#ifndef __SANCUS_SIM
    void sm_io_init(void)
    {
        uart_init();
    }

    void dump_sm_layout(struct SancusModule *sm)
    {
        printf("\nSM %s with ID %d enabled\t: 0x%.4x 0x%.4x 0x%.4x 0x%.4x\n",
            sm->name, sm->id,
            (uintptr_t) sm->public_start, (uintptr_t) sm->public_end,
            (uintptr_t) sm->secret_start, (uintptr_t) sm->secret_end);
    }
#endif

#endif /* NODEBUG */
