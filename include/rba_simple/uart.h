// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

struct uart
{
	void *handle;
	uint8_t tx_buffer[256*10];
	uint8_t rx_buffer[2];
};

enum uart_instance
{
	UART_INST0 = 0,
	UART_INST1 = 1
};

void uart_init(struct uart *uart, enum uart_instance instance);

#endif//UART_H_
