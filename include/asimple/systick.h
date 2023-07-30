// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>

/** Initializes the system counter.
 *
 * This uses the M4's systick function, and it is affected by the use of
 * Turbospot. This implementation attempts to correct for the use of turbo
 * mode, but there may still be some small error in timing introduced by the
 * switching of operating modes.
 *
 * This function initializes the tick timer, and enables the interrupt. It odes
 * not start the counter, however.
 */
void systick_init(void);

/** Frees the system counter.
 *
 * This stops the timer, resets the M4's systick control register, and disables
 * the systick interrupt.
 *
 */
void systick_destroy(void);

/** Starts the system counter.
 *
 * The timer is configured to tick every millisecond.
 */
void systick_start(void);

/** Returns the number of ticks since the timer was initialized and started.
 *
 * @returns The number of ticks since the timer was initialized and started.
 */
uint64_t systick_jiffies(void);

#endif//SYSTICK_H_
