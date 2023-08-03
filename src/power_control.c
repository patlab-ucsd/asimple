// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <gpio.h>
#include <power_control.h>

void power_control_init(struct power_control *control, uint8_t shd_pin, uint8_t active_pin)
{
	gpio_init(&control->shd, shd_pin, GPIO_MODE_OUTPUT, 1);
	gpio_init(&control->active, active_pin, GPIO_MODE_OUTPUT, 0);
}

void power_control_shutdown(struct power_control *control)
{
	gpio_set(&control->active, true);
	gpio_set(&control->shd, false);
}
