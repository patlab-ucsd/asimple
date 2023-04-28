// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

/** UART structure. */
struct uart
{
	void *handle;
	int instance;
	// FIXME do we want the buffer size to be fixed?
	uint8_t tx_buffer[256*10];
	// FIXME does RX work?
	uint8_t rx_buffer[2];
};

/** Represents the UART instance index.
 */
enum uart_instance
{
	UART_INST0 = 0,
	UART_INST1 = 1
};

/** Initializes the given UART structure and gives ownership of the specified
 *  instance to the structure.
 *
 * @param[out] uart Pointer to uart structure to initialize.
 * @param[in] instance UART instance to assign to UART structure.
 */
void uart_init(struct uart *uart, enum uart_instance instance);

/** Deinitializes the given UART structure, freeing resources held, including
 * the associated UART instance.
 *
 * @param[in] uart Pointer to uart structure to deinitialize.
 */
void uart_destroy(struct uart *uart);

//FIXME what about RX and TX functions?

#endif//UART_H_
