// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
/// @file

#ifndef POWER_CONTROL_H_
#define POWER_CONTROL_H_

#include <gpio.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct power_control_functions
{
	void (*sleep)(void *data);
	void (*wakeup)(void *data);
	void *data;
};

struct power_control
{};

void power_control_init(struct power_control *control);

void power_control_register(
	struct power_control *control, struct power_control_functions *functions
);

void power_control_shutdown(struct power_control *control);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // POWER_CONTROL_H_
