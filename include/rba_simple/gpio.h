// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include <stdbool.h>

/** GPIO struct.
 */
struct gpio
{
	uint8_t pin;
};

/** Initializes the GPIO structure and given pin hardware.
 *
 * @param[out] gpio Pointer to the GPIO structure to initialize as a push-pull
 *  GPIO.
 * @param[in] Pin number of the GPIO to initialize.
 * @param[in] Initial state of the GPIO pin.
 */
void gpio_init(struct gpio *gpio, uint8_t pin, bool init_state);

/** Sets the GPIO pin state.
 *
 * @param[in,out] gpio Pointer to GPIO struct of pin to set.
 * @param[in] state True to set to high, false to set to low.
 */
void gpio_set(struct gpio *gpio, bool state);

/** Gets the GPIO pin state.
 *
 * @param[in,out] gpio Pointer to GPIO struct of pin to get.
 *
 * @Returns True if the pin reads high, false otherwise.
 */
bool gpio_read(struct gpio *gpio);

#endif//GPIO_H_
