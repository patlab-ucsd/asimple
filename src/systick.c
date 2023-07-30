// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <systick.h>

#include <am_bsp.h>

#include <sys/time.h>

#include <stdint.h>
#include <stdbool.h>

void systick_init(void)
{
	am_hal_systick_reset();
	NVIC_EnableIRQ(SysTick_IRQn);
}

void systick_destroy(void)
{
	am_hal_systick_reset();
	NVIC_DisableIRQ(SysTick_IRQn);
}

void systick_start(void)
{
	am_hal_systick_load(48000 - 1);
	am_hal_systick_start();
	am_hal_systick_int_enable();
}

static volatile uint64_t jiffies;

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
	uint64_t result;
	am_hal_interrupt_master_disable();
	result = jiffies;
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
		counter = 47999 -  SysTick->VAL;
		current = systick_jiffies();
	} while (prev != current);

	struct timespec result = {
		.tv_sec = current / 1000,
		.tv_nsec = (current % 1000u) * 1000000ull + ((counter + 1) * 1000ull / 48),
	};
	return result;
}
