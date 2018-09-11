#include <stdint.h>
#include <msp430.h>
#include "sm_io_wrap.h"
#include "sm-pin.h"
#include "keypad_mmio.h"

#define SECURE_SM_EXPECTED_ID   1
#define SPY_SM_EXPECTED_ID      2

int spy_tsc;

/* ======================== IMPLEMENTATION MACROS =========================== */
// helper macros facilitate exact implementation copy in spy SM

#define SECURE_INIT( me, mmio_init_fct, my_init, my_pin_idx, my_keymap,       \
                     my_key_state )                                           \
{                                                                             \
    /* keypad_mmio SM has the ID of this SM hardcoded */                      \
    if (sancus_get_self_id() != me)                                           \
    {                                                                         \
        return 0;                                                             \
    }                                                                         \
    /* call and verify keypad_mmio SM */                                      \
    mmio_init_fct();                                                          \
    my_init = 1;                                                              \
    my_pin_idx = 0;                                                           \
    my_key_state = 0x0;                                                       \
    /* avoid an unprotected global constant */                                \
    /* TODO consider an SM_CONST compiler attribute (?) */                    \
    my_keymap[0]  = '1';                                                      \
    my_keymap[1]  = '4';                                                      \
    my_keymap[2]  = '7';                                                      \
    my_keymap[3]  = '0';                                                      \
    my_keymap[4]  = '2';                                                      \
    my_keymap[5]  = '5';                                                      \
    my_keymap[6]  = '8';                                                      \
    my_keymap[7]  = 'F';                                                      \
    my_keymap[8]  = '3';                                                      \
    my_keymap[9]  = '6';                                                      \
    my_keymap[10] = '9';                                                      \
    my_keymap[11] = 'E';                                                      \
    my_keymap[12] = 'A';                                                      \
    my_keymap[13] = 'B';                                                      \
    my_keymap[14] = 'C';                                                      \
    my_keymap[15] = 'D';                                                      \
    return 1;                                                                 \
}

/* ========================== SECURE KEYPAD SM ============================= */

DECLARE_SM(secure, 0x1234);

key_state_t SM_DATA(secure) secret_key_state;
char        SM_DATA(secure) secret_pin[PIN_LEN];
int         SM_DATA(secure) secret_pin_idx;
int         SM_DATA(secure) secret_init;
int         SM_DATA(secure) secret_keymap[NB_KEYS];

int SM_FUNC(secure) sm_secure_init( void )
{
    SECURE_INIT( SECURE_SM_EXPECTED_ID, keypad_mmio_init, secret_init,
        secret_pin_idx, secret_keymap, secret_key_state)
}

/*
 * The start-to-end timing of this function only reveals the number of times the
 * if statement was executed (i.e. the number of keys that were down), which is
 * also explicitly returned. By carefully interrupting the function each for
 * loop iteration, an untrusted ISR can learn the value of the secret PIN code.
 */
int SM_ENTRY(secure) sm_secure_poll_keypad( void )
{
    int key_mask = 0x1;

    if (!secret_init)
    {
        return sm_secure_init();
    }

    /* Fetch key state from MMIO driver SM. */
    key_state_t new_key_state = keypad_mmio_read_key_state();

    /* Store down keys in private PIN array. */
    for (int key = 0; key < NB_KEYS; key++)
    {
        int is_pressed  = (new_key_state & key_mask);
        int was_pressed = (secret_key_state & key_mask);
        if ( is_pressed /* INTERRUPT SHOULD ARRIVE HERE */
             && !was_pressed && (secret_pin_idx < PIN_LEN))
        {
            secret_pin[secret_pin_idx++] = secret_keymap[key];
        }
        /* .. OR HERE. */

        key_mask = key_mask << 1;
    }

    secret_key_state = new_key_state;
    return (PIN_LEN - secret_pin_idx);
}

void SM_ENTRY(secure) dump_secure_pin( void )
{
    printf2("[sm] secret PIN is %c%c", secret_pin[0], secret_pin[1]);
    printf2("%c%c\n", secret_pin[2], secret_pin[3]);
}

#if 0
/* =========================== SPY SM ============================== */
// near exact copy of secure SM, modified to leak timings

DECLARE_SM(spy, 0x1234);

key_state_t SM_DATA(spy) spy_key_state;
char        SM_DATA(spy) spy_pin[PIN_LEN];
int         SM_DATA(spy) spy_pin_idx;
int         SM_DATA(spy) spy_init;
int         SM_DATA(spy) spy_keymap[NB_KEYS];
int                        spy_tsc;

int SM_FUNC(spy) sm_spy_init( void )
{
    SECURE_INIT( SPY_SM_EXPECTED_ID, spy_mmio_init, spy_init,
        spy_pin_idx, spy_keymap, spy_key_state )
}

int SM_ENTRY(spy) __attribute__((naked)) sm_spy_measure( void )
{
    asm ("mov &%0, &spy_tsc \n\t"
    ::"m"(TAR):);

#if 0
/*
    SECURE_POLL_KEYPAD( spy_init, sm_spy_init, spy_pin_idx, spy_pin,
        spy_mmio_read_key_state, spy_key_state, spy_keymap,
        SPY_IF_BODY)
*/
    asm(
        "push    r4                                     \n\t"
        "mov     r1,     r4                             \n\t"
        "push    r11                                    \n\t"
        "push    r10                                    \n\t"
        "push    r9                                     \n\t"
        "tst     &spy_init                              \n\t"
        "jz      1f                                     \n\t"
        "call    #spy_mmio_read_key_state               \n\t"
        "mov     #1,     r12                            \n\t"
        "clr     r13                                    \n\t"
        "mov     &spy_key_state,r14                     \n\t"
     "3: mov     &spy_pin_idx,r11                       \n\t"
        "cmp     #4,     r11                            \n\t"
        "jge     2f                                     \n\t"
        "mov     r12,    r10                            \n\t"
        "and     r15,    r10                            \n\t"
        "tst     r10                                    \n\t"
        "jz      2f                                     \n\t"
        "mov     r14,    r10                            \n\t"
        "and     r12,    r10                            \n\t"
        "tst     r10                                    \n\t"
        "jnz     2f                                     \n\t"

        "mov     &%0, &spy_tsc                          \n\t"

        "mov.b   702(r13),r10                           \n\t"
        "mov     r11,    r9                             \n\t"
        "inc     r9                                     \n\t"
        "mov     r9,     &spy_pin_idx                   \n\t"
        "mov.b   r10,    734(r11)                       \n\t"
     "2: rla     r12                                    \n\t"
        "incd    r13                                    \n\t"
        "cmp     #32,    r13                            \n\t"
        "jnz     3b                                     \n\t"
        "mov     r15,    &spy_key_state                 \n\t"
        "mov     #4,     r15                            \n\t"
        "sub     &spy_pin_idx,r15                       \n\t"
        "jmp     4f                                     \n\t"
     "1: call    #sm_spy_init                           \n\t" 
     "4: pop     r9                                     \n\t"
        "pop     r10                                    \n\t"
        "pop     r11                                    \n\t"
        "pop     r4                                     \n\t"
        "ret                                            \n\t"
    ::"m"(TAR):);
#endif
}

// allow re-initialisation for deterministic test behavior
void SM_ENTRY(spy) do_spy_init( void )
{
    //sm_spy_init();
}
#endif
