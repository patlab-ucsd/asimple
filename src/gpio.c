// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include "am_bsp.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

#include <gpio.h>

#include <am_hal_gpio.h>
#include <am_hal_status.h>

void gpio_init(
	struct gpio *gpio, uint8_t pin, enum gpio_mode mode, bool init_state
)
{
	gpio->pin = pin;
	// Make sure to set the hardware registers indicating the pin value before
	// changing the pin configuration, so that the state is activated as soon
	// as we configure the hardware
	gpio_set(gpio, init_state);
	if (mode == GPIO_MODE_OUTPUT)
	{
		gpio->config = g_AM_HAL_GPIO_OUTPUT_WITH_READ;
	}
	else if (mode == GPIO_MODE_INPUT)
	{
		const am_hal_gpio_pincfg_t input_gpio = {
			.uFuncSel = 3,
			.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
			.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
			.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN,
			.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI
		};
		gpio->config = input_gpio;
	}
	else if (mode == GPIO_MODE_INPUT_PULLUP)
	{
		const am_hal_gpio_pincfg_t input_gpio = {
			.uFuncSel = 3,
			.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
			.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
			.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN,
			.ePullup = AM_HAL_GPIO_PIN_PULLUP_WEAK,
			.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI
		};
		gpio->config = input_gpio;
	}
	else if (mode == GPIO_MODE_INPUT_PULLDOWN)
	{
		const am_hal_gpio_pincfg_t input_gpio = {
			.uFuncSel = 3,
			.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
			.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
			.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN,
			.ePullup = AM_HAL_GPIO_PIN_PULLDOWN,
			.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI
		};
		gpio->config = input_gpio;
	}
	else if (mode == GPIO_MODE_OPENDRAIN)
	{
		const am_hal_gpio_pincfg_t opendrain_gpio = {
			.uFuncSel = 3,
			.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
			.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
			.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN,
			.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE,
			.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
		};
		gpio->config = opendrain_gpio;
	}

	switch (mode)
	{
	case GPIO_MODE_INPUT:
	case GPIO_MODE_INPUT_PULLDOWN:
	case GPIO_MODE_INPUT_PULLUP:
	{
		AM_HAL_GPIO_MASKCREATE(interrupt_mask);
		AM_HAL_GPIO_MASKBIT(pinterrupt_mask, pin);
		am_hal_gpio_interrupt_enable(pinterrupt_mask);
		NVIC_EnableIRQ(GPIO_IRQn);
	}

	case GPIO_MODE_OUTPUT:
	case GPIO_MODE_OPENDRAIN:
	default:
		// Do nothing
	}
	am_hal_gpio_pinconfig(pin, gpio->config);
}

void gpio_set(struct gpio *gpio, bool state)
{
	am_hal_gpio_state_write(
		gpio->pin, state ? AM_HAL_GPIO_OUTPUT_SET : AM_HAL_GPIO_OUTPUT_CLEAR
	);
}

bool gpio_read(struct gpio *gpio)
{
	uint32_t result = 0;
	am_hal_gpio_state_read(gpio->pin, AM_HAL_GPIO_INPUT_READ, &result);
	return result;
}

void gpio_set_interrupt_direction(
	struct gpio *gpio, enum gpio_interrupt_direction direction
)
{
	switch (direction)
	{
	case GPIO_INTERRUPT_DIRECTION_HI2LO:
		gpio->config.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
	case GPIO_INTERRUPT_DIRECTION_LO2HI:
	default:
		gpio->config.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI;
	}
	am_hal_gpio_pinconfig(gpio->pin, gpio->config);
}

void am_gpio_isr(void)
{
	uint64_t ui64Status;
	am_hal_gpio_interrupt_status_get(false, &ui64Status);
	am_hal_gpio_interrupt_clear(ui64Status);
	am_hal_gpio_interrupt_service(ui64Status);
}
