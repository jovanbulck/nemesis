#include "sm-bsl.h"
#include <msp430.h>
#include <sancus_support/sm_io.h>

DECLARE_SM(sm_bsl, 0x1234);

#define MASS_ERASE_DELAY 0x8000

#define LOCKED 0x00
#define UNLOCKED 0xA5A5
unsigned int SM_DATA(sm_bsl) LockedStatus;

char SM_FUNC(sm_bsl) BSL430_massErase(void)
{
    return SUCCESSFUL_OPERATION;
}

/*
 * Vulnerable password comparison routine based on TI MSP430 BSL v9 and v2.01.
 *
 * Texas Instruments (http://www.ti.com/tool/mspbsl) does not anymore provide
 * source code or binaries for BSL versions prior to version 8. We therefore
 * based the _vulnerable_ password comparison loop on the assembly code snippet
 * published in: T. Goodspeed, "Practical attacks against the MSP430 BSL",
 * Twenty-Fifth Chaos Communications Congress, 2008.
 *
 * The C code skeleton below is taken verbatim from the _invulnerable_ password
 * comparison routine of the latest TI BSL v9.
 */

/*******************************************************************************
* *Function:    BSL430_unlock_BSL
* *Description: Causes the BSL to compare the data buffer against the BSL password
*             BSL state will be UNLOCKED if successful
* *Parameters:
*             char* data            A pointer to an array containing the password
* *Returns:
*             SUCCESSFUL_OPERATION  All data placed into data array successfully
*             BSL_PASSWORD_ERROR    Correct Password was not given
*******************************************************************************/
char SM_ENTRY(sm_bsl) BSL430_unlock_BSL_unbalanced(char* data)
{
    int i;
    int retValue = 0;
    char *interrupts = (char*)INTERRUPT_VECTOR_START;
    /* BSL version from v2.2.09 on use a password comparison loop based on XOR,
     * as recommended in Goodspeed2008 to prevent timing side-channels. Our
     * _vulnerable_ implementation is based on published asm code from v2.12.
     */

    /*for (i = 0; i <= (INTERRUPT_VECTOR_END - INTERRUPT_VECTOR_START); i++, interrupts++)
    {
        retValue |=  *interrupts ^ data[i];
    }*/

    asm("mov %0, r11                                \n\t" /* retValue */
        "mov %1, r6                                 \n\t" /* data ptr */
        "mov %2, r13                                \n\t" /* ivt ptr */
        "mov %3, r7                                 \n\t" /* i cntr */
        "4: tst r7                                  \n\t"
        "jz 2f                                      \n\t"
        "mov.b @r13+, r12                           \n\t"
        /* --- START asm code BSLv2.12 --- */
        "cmp.b @r6+, r12                            \n\t"
        "jz 1f                                      \n\t"
        "bis #0x40, r11                             \n\t"
        "1: dec r7                                  \n\t"
        /* --- END asm code BSLv2.12 --- */
        "jmp 4b                                     \n\t"
        "2: mov r11, %0                             \n\t"
        :"=m"(retValue)
        :"m"(data),"m"(interrupts),
         "i"(BSL_PASSWORD_LENGTH)
        :"r6","r7","r11","r12","r13");

    if (retValue == 0)
    {
#ifndef RAM_WRITE_ONLY_BSL
        volatile int i;
        for (i = MASS_ERASE_DELAY - 1; i > 0; i--) ;
#endif
        LockedStatus = UNLOCKED;
        return SUCCESSFUL_OPERATION;
    }
    else
    {
        BSL430_massErase();
        return BSL_PASSWORD_ERROR;
    }
}

#ifdef LEAK_BSL_TSC
    int spy_tsc;
#endif

char SM_ENTRY(sm_bsl) BSL430_unlock_BSL_balanced(char* data)
{
    int i;
    int retValue = 0;
    char *interrupts = (char*)INTERRUPT_VECTOR_START;

    /*
     * We close the timing channel above by carefully balancing the else branch
     * with no-op compensation code.
     */
    asm(
        /* r11 = retValue */
        "mov %0, r11                                \n\t"
        /* r6 = data */
        "mov %1, r6                                 \n\t"
        /* r13 = interrupts */
        "mov %2, r13                                \n\t"
        /* r7 = length-1 */
        "mov %3, r7                                 \n\t"
        /* if r7 == 0 */
        "4: tst r7                                  \n\t"
        /* jump to label "3" if all (-1?) bytes have been checked */
        "jz 3f                                      \n\t"
        /* copy byte from interrupts array and increment r13 (move a character) */
        "mov.b @r13+, r12                           \n\t"
        /* --- START _modified_ asm code BSLv2.12 --- */
        /* compare byte from data array to interrupts array and increment (move) */
        "cmp.b @r6+, r12                            \n\t"
        /* if the characters match, jump to label "1" */
        "jz 1f                                      \n\t"
#ifdef LEAK_BSL_TSC
        "mov &%4, &spy_tsc                          \n\t"
#endif
        /* retValue register set to error value */
        "bis #0x40, r11                             \n\t"
        /* jump to label "2" */
        "jmp 2f                                     \n\t"
        /* label "1": nop block for when characters match */
        "1: nop                                     \n\t"
        "nop                                        \n\t"
        "nop                                        \n\t"
        "nop                                        \n\t"
        /* label "2": decrement length counter */
        "2: dec r7                                  \n\t"
        /* --- END _modified_ asm code BSLv2.12 --- */
        /* jump to label "4" (loop start) */
        "jmp 4b                                     \n\t"
        /* label "3": all bytes have been checked, return retValue */
        "3: mov r11, %0                             \n\t"
        :
        "=m"(retValue)                                      // %0
        :
        "m"(data),                                          // %1
        "m"(interrupts),                                    // %2
         "i"(BSL_PASSWORD_LENGTH) // %3
#ifdef LEAK_BSL_TSC
        , "m"(TAR)
#endif
        :"r6","r7","r11","r12","r13"
    );

    if (retValue == 0)
    {
#ifndef RAM_WRITE_ONLY_BSL
        volatile int i;
        for (i = MASS_ERASE_DELAY - 1; i > 0; i--) ;
#endif
        LockedStatus = UNLOCKED;
        return SUCCESSFUL_OPERATION;
    }
    else
    {
        BSL430_massErase();
        return BSL_PASSWORD_ERROR;
    }
}
