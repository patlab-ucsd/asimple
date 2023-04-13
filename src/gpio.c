// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <gpio.h>

#include <am_hal_status.h>
#include <am_hal_gpio.h>

void gpio_init(struct gpio *gpio, uint8_t pin, bool init_state)
{
	gpio->pin = pin;
	gpio_set(gpio, init_state);
	am_hal_gpio_pinconfig(pin, g_AM_HAL_GPIO_OUTPUT_WITH_READ);
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
