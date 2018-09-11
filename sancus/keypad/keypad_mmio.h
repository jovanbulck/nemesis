#ifndef MMIO_DRIVER_H
#define MMIO_DRIVER_H

#include <sancus/sm_support.h>
#include <stdint.h>

// bitmap indicating which keys are down
typedef uint16_t key_state_t;

key_state_t SM_MMIO_ENTRY(key_mmio) keypad_mmio_read_key_state(void);
void SM_MMIO_ENTRY(key_mmio) keypad_mmio_init(void);

key_state_t SM_MMIO_ENTRY(spy_mmio) spy_mmio_read_key_state( void );
void SM_MMIO_ENTRY(spy_mmio) spy_mmio_init(void);

extern uint16_t spy_mmio_ret_val;

#endif
