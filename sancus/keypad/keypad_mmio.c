#include "keypad_mmio.h"
#include <msp430.h>

/* ======================== IMPLEMENTATION MACROS =========================== */
// helper macros facilitate exact implementation copy in spy SM

#define READ_KEY_STATE_STUB                                                   \
    "clr r15\n\t"           /* return value key_state bitmask */              \
    "clr r14\n\t"           /* loop counter */                                \
    "mov.b #0xfe,r13\n\t"   /* column pin selector */                         \
                                                                              \
    "1: cmp #4, r14                                                     \n\t" \
    "jge 4f                                                             \n\t" \
                                                                              \
    /* pull the column pin low */                                             \
    "mov.b r13, &%0                                                     \n\t" \
    /* if we don't wait a couple of cycles, we sometimes seem to read old     \
       values from the row pins */                                            \
    "nop                                                                \n\t" \
    "nop                                                                \n\t" \
    "nop                                                                \n\t" \
    "nop                                                                \n\t" \
    "nop                                                                \n\t" \
                                                                              \
    /* read state of current column (row pins) and shift right into 4 LSBs */ \
    "mov.b &%1, r12                                                     \n\t" \
    "clrc                                                               \n\t" \
    "rrc.b r12                                                          \n\t" \
    "rra.b r12                                                          \n\t" \
    "rra.b r12                                                          \n\t" \
    "rra.b r12                                                          \n\t" \
                                                                              \
    /* shift current column state left into key_state return bitmask */       \
    "clr r11                                                            \n\t" \
    "2: cmp r14, r11                                                    \n\t" \
    "jge 3f                                                             \n\t" \
    "rla r12                                                            \n\t" \
    "rla r12                                                            \n\t" \
    "rla r12                                                            \n\t" \
    "rla r12                                                            \n\t" \
    "inc r11                                                            \n\t" \
    "jmp 2b                                                             \n\t" \
    "3: bis r12, r15                                                    \n\t" \
                                                                              \
    "inc r14                                                            \n\t" \
    "setc                                                               \n\t" \
    "rlc.b r13                                                          \n\t" \
    "jmp 1b                                                             \n\t" \
                                                                              \
    /* since the row pins are active low, we invert the state to get a more   \
       natural representation*/                                               \
    "4: inv r15                                                         \n\t"

#define INIT_STUB                                                             \
    /* column pins are mapped to P2OUT[0:3] and the row pins to P2IN[4:7] */  \
    "mov.b #0x00, &%0                                                   \n\t" \
    "mov.b #0x0f, &%1                                                   \n\t"

/* ======================== SECURE MMIO DRIVER =========================== */

DECLARE_EXCLUSIVE_MMIO_SM(key_mmio,
                          /*[secret_start, end[=*/ P2IN_, P3IN_,
                          /*caller_id=*/ 1,
                          /*vendor_id=*/ 0x1234);

key_state_t SM_MMIO_ENTRY(key_mmio) keypad_mmio_read_key_state(void)
{
    asm(
        READ_KEY_STATE_STUB
        ::"m"(P2OUT), "m"(P2IN):
       );
}

void SM_MMIO_ENTRY(key_mmio) keypad_mmio_init(void)
{
    asm(
        INIT_STUB
        ::"m"(P2SEL), "m"(P2DIR):
       );
}

#if 0
/* ======================== SPY MMIO DRIVER =========================== */
// near exact copy for reference measurements without memory violations

DECLARE_EXCLUSIVE_MMIO_SM(spy_mmio,
                          /*[secret_start, end[=*/ &dummy, &dummy+1,
                          /*caller_id=*/ 1,
                          /*vendor_id=*/ 0x1234);

char dummy, dummyIn, dummyOut, dummySel, dummyDir;
uint16_t spy_mmio_ret_val;

key_state_t SM_MMIO_ENTRY(key_mmio) spy_mmio_read_key_state(void)
{
    asm(
        READ_KEY_STATE_STUB
        /* ensure only first 2 key comparisons hold (+3 overhead cycles)*/
        "mov &spy_mmio_ret_val, r15\n\t"
        ::"m"(dummyOut), "m"(dummyIn):
       );
}

void SM_MMIO_ENTRY(key_mmio) spy_mmio_init(void)
{
    asm(
        INIT_STUB
        ::"m"(dummySel), "m"(dummyDir):
       );
}

void SM_MMIO_ENTRY(key_mmio) spy_empty(void)
{
}
#endif
