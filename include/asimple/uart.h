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

/** Sends the given buffer over UART.
 *
 * @param[in,out] uart
 * @param[in] data Buffer to write.
 * @param[in] size Size of buffer to write.
 *
 * @returns The number of bytes written.
 */
size_t uart_write(struct uart *uart, const unsigned char *data, size_t size);

/** Sets the UART baud rate to the requested amount.
 *
 * Note that in reality, there is an upper limit to the baud rate. For Apollo3
 * A1, 921600, for Apollo3 B0, 1500000. FIXME currently this function hangs if
 * a bad baud rate is given.
 *
 * @param[in,out] uart Uart instance to update.
 * @param[in] baud_rate Desired baud rate.
 */
void uart_set_baud_rate(struct uart *uart, unsigned int baud_rate);

//FIXME what about RX and TX functions?

#endif//UART_H_
