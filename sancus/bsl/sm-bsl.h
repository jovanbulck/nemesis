#ifndef SM_BSL_H
#define SM_BSL_H

#include <sancus/sm_support.h>

extern struct SancusModule sm_bsl;

char SM_ENTRY(sm_bsl) BSL430_unlock_BSL_unbalanced(char* data);
char SM_ENTRY(sm_bsl) BSL430_unlock_BSL_balanced(char* data);

#define SUCCESSFUL_OPERATION 0x00
#define BSL_PASSWORD_ERROR 0x05

#define INTERRUPT_VECTOR_START 0xFFE0
#define INTERRUPT_VECTOR_END   0xFFFF

/* NOTE: speed up Travis-CI builds */
#ifdef TRAVIS_CI
    #define BSL_PASSWORD_LENGTH    3
#else
    #define BSL_PASSWORD_LENGTH    (INTERRUPT_VECTOR_END - INTERRUPT_VECTOR_START + 1)
#endif

//#define LEAK_BSL_TSC
#ifdef LEAK_BSL_TSC
    #warning leaking bsl tsc for testing purposes...
    extern int spy_tsc;
#endif

#endif
