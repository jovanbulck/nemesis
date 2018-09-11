#ifndef SM_KEYPAD_H
#define SM_KEYPAD_H

#include <sancus/sm_support.h>
#include "keypad_mmio.h"

#define PIN_LEN     4
#define NB_KEYS     16

extern struct SancusModule secure;
extern struct SancusModule key_mmio;
extern struct SancusModule spy;
extern struct SancusModule spy_mmio;

int SM_ENTRY(secure) sm_secure_poll_keypad( void );

// explicit leak function for testing purposes
void SM_ENTRY(secure) dump_secure_pin( void );

int SM_ENTRY(spy) sm_spy_measure( void );
void SM_ENTRY(spy) do_spy_init( void );

/* SM_spy copies Timer_A TAR content to global var after conditional jump. */
extern int spy_tsc;

#endif
