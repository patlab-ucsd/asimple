// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <systick.h>

#include <am_bsp.h>

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
	am_hal_systick_load(48000);
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
