//
// Created by Samuel Jones on 11/9/21.
//

#ifndef badge2022_c_INIT_H
#define badge2022_c_INIT_H

#include <stdint.h>

/// Do hardware-specific initialization.
void hal_init(void);

/// Do hardware-specific deinit before a reset or exit.
void hal_deinit(void);

/// Exit application or go back to reset vector.
void hal_reboot(void);

/// disable / restore interrupt state;
uint32_t hal_disable_interrupts(void);
void hal_restore_interrupts(uint32_t state);

#endif //badge2022_c_INIT_H
