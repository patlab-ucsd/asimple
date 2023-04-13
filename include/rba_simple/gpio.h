// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include <stdbool.h>

struct gpio
{
	uint8_t pin;
};

void gpio_init(struct gpio *gpio, uint8_t pin, bool init_state);
void gpio_set(struct gpio *gpio, bool state);
bool gpio_read(struct gpio *gpio);

#endif//GPIO_H_
