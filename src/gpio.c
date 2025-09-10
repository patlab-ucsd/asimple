// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <gpio.h>

#include <am_hal_status.h>
#include <am_hal_gpio.h>

void gpio_init(
	struct gpio *gpio, uint8_t pin, enum gpio_mode mode, bool init_state)
{
	gpio->pin = pin;
	// Make sure to set the hardware registers indicating the pin value before
	// changing the pin configuration, so that the state is activated as soon
	// as we configure the hardware
	gpio_set(gpio, init_state);
	if (mode == GPIO_MODE_OUTPUT)
		am_hal_gpio_pinconfig(pin, g_AM_HAL_GPIO_OUTPUT_WITH_READ);
	else if (mode == GPIO_MODE_INPUT)
	{
		const am_hal_gpio_pincfg_t input_gpio =
		{
			.uFuncSel  = 3,
			.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
			.eGPInput  = AM_HAL_GPIO_PIN_INPUT_ENABLE,
			.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN,
			.eIntDir   = AM_HAL_GPIO_PIN_INTDIR_LO2HI
		};
		am_hal_gpio_pinconfig(pin, input_gpio);

		AM_HAL_GPIO_MASKCREATE(interrupt_mask);
		AM_HAL_GPIO_MASKBIT(pinterrupt_mask, pin);
		am_hal_gpio_interrupt_enable(pinterrupt_mask);
		NVIC_EnableIRQ(GPIO_IRQn);
	}
	else if (mode == GPIO_MODE_OPENDRAIN)
	{
		const am_hal_gpio_pincfg_t opendrain_gpio =
		{
			.uFuncSel       = 3,
			.eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
			.eGPInput       = AM_HAL_GPIO_PIN_INPUT_ENABLE,
			.eGPRdZero      = AM_HAL_GPIO_PIN_RDZERO_READPIN,
			.ePullup        = AM_HAL_GPIO_PIN_PULLUP_NONE,
			.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
		};

		am_hal_gpio_pinconfig(pin, opendrain_gpio);
	}
}

void gpio_set(struct gpio *gpio, bool state)
{
	am_hal_gpio_state_write(gpio->pin, state ? AM_HAL_GPIO_OUTPUT_SET : AM_HAL_GPIO_OUTPUT_CLEAR);
}

bool gpio_read(struct gpio *gpio)
{
	uint32_t result = 0;
	am_hal_gpio_state_read(gpio->pin, AM_HAL_GPIO_INPUT_READ, &result);
	return result;
}

void am_gpio_isr(void)
{
	uint64_t ui64Status;
	am_hal_gpio_interrupt_status_get(false, &ui64Status);
	am_hal_gpio_interrupt_clear(ui64Status);
	am_hal_gpio_interrupt_service(ui64Status);
}
