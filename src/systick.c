// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <systick.h>

#include <am_bsp.h>

#include <sys/time.h>

#include <stdint.h>
#include <stdbool.h>

static bool initialized = false;
static volatile uint64_t jiffies;

static uint32_t const SYSTEM_CLOCK = 48000000;

bool systick_initialized(void)
{
	return initialized;
}

bool systick_started(void)
{
	return systick_initialized() && (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk);
}

void systick_reset(void)
{
	// This clears the entire control register. The effect is:
	// - COUNTFLAG is cleared
	// - CLKSOURCE is set to IMPLEMENTATION DEFINED
	// - TICKINT is cleared
	// - ENABLE is cleared
	am_hal_systick_reset();
	jiffies = 0;
	NVIC_EnableIRQ(SysTick_IRQn);
	am_hal_systick_load((SYSTEM_CLOCK / 1000) - 1);
	am_hal_systick_int_enable();
	initialized = true;
}

void systick_stop(void)
{
	// This clears the TICKINT bit in CTRL
	am_hal_systick_stop();
}

void systick_start(void)
{
	// This sets the TICKINT bit in CTRL
	am_hal_systick_start();
}

void SysTick_Handler(void)
{
	static bool burst_counter = false;
	bool burst = am_hal_burst_mode_status() == AM_HAL_BURST_MODE;
	if (!burst || burst_counter)
	{
		jiffies++;
	}

	if (burst)
	{
		burst_counter = !burst_counter;
	}
}

uint64_t systick_jiffies(void)
{
	am_hal_interrupt_master_disable();
	uint64_t result = jiffies;
	am_hal_interrupt_master_enable();
	return result;
}

struct timespec systick_time(void)
{
	uint64_t prev, current;
	uint32_t counter;

	do
	{
		prev = systick_jiffies();
		counter = ((SYSTEM_CLOCK / 1000) - 1) -  SysTick->VAL;
		current = systick_jiffies();
	} while (prev != current);

	struct timespec result = {
		.tv_sec = current / 1000,
		.tv_nsec = (current % 1000u) * 1000000ull + ((counter + 1) * 1000ull / 48),
	};
	return result;
}
