// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>

/** Returns whether or not the systick subsystem is running.
 *
 * @returns True if systick has been initialized and is started, false
 *  otherwise.
 */
bool systick_started(void);

/** Returns whether or not the systick subsystem has been initialized.
 *
 * @returns True if systic has been initialized.
 */
bool systick_initialized(void);

/** (Re)Initializes the system counter.
 *
 * This uses the M4's systick function, and it is affected by the use of
 * Turbospot. This implementation attempts to correct for the use of turbo
 * mode, but there may still be some small error in timing introduced by the
 * switching of operating modes.
 *
 * This function initializes the tick timer, and enables the interrupt. It does
 * not start the counter, however.
 */
void systick_reset(void);

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

/** Returns the time since the timer was initialized and started.
 *
 * @returns The time since the timer was initialized and started.
 */
struct timespec systick_time(void);

#endif//SYSTICK_H_
