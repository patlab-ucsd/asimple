// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef GPIO_H_
#define GPIO_H_

#include <am_mcu_apollo.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** GPIO struct. */
struct gpio
{
	am_hal_gpio_pincfg_t config;
	uint8_t pin;
};

enum gpio_mode
{
	GPIO_MODE_OUTPUT,
	GPIO_MODE_INPUT,
	GPIO_MODE_INPUT_PULLUP,
	GPIO_MODE_INPUT_PULLDOWN,
	GPIO_MODE_OPENDRAIN
};

enum gpio_interrupt_direction
{
	GPIO_INTERRUPT_DIRECTION_LO2HI,
	GPIO_INTERRUPT_DIRECTION_HI2LO,
};

// FIXME we should let the type of GPIO be configurable, or document how to do
// so

/** Initializes the GPIO structure and given pin hardware.
 *
 * @param[out] gpio Pointer to the GPIO structure to initialize as a push-pull
 *  GPIO or as input.
 * @param[in] Pin number of the GPIO to initialize.
 * @param[in] mode The mode of the GPIO, see enum gpio_mode for options.
 * @param[in] init_state Initial state of the GPIO pin.
 */
void gpio_init(
	struct gpio *gpio, uint8_t pin, enum gpio_mode mode, bool init_state
);

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

/** Set the GPIO interrupt direction.
 *
 * @param[in] direction The direction of the interrupt.
 */
void gpio_set_interrupt_direction(
	struct gpio *gpio, enum gpio_interrupt_direction direction
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPIO_H_
