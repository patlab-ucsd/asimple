// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
/// @file

#ifndef POWER_CONTROL_H_
#define POWER_CONTROL_H_

#include <gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct power_control
{
	struct gpio shd;
	struct gpio active;
};

void power_control_init(struct power_control *control, uint8_t shd_pin, uint8_t active_pin);

void power_control_shutdown(struct power_control *control);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//POWER_CONTROL_H_
