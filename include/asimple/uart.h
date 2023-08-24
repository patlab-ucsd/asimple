// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Declaration of opaque struct for UART handle. */
struct uart;

/** Represents the UART instance index.
 */
enum uart_instance
{
	UART_INST0 = 0,
	UART_INST1 = 1
};

/** Gets the requested UART structure.
 *
 * The first time this is called (from boot or after a disable) this
 * initializes the hardware and allocates resources (in other words, lazy
 * initialization).
 *
 * @param[in] instance UART instance to assign to UART structure.
 *
 * @returns Opaque pointer to UART instance.
 */
struct uart* uart_get_instance(enum uart_instance instance);

/** Deinitializes the given UART structure, freeing resources held, including
 * the associated UART instance.
 *
 * @param[in] uart Pointer to uart structure to deinitialize.
 */
void uart_disable(struct uart *uart);

/** Sends the given buffer over UART.
 *
 * @param[in,out] uart
 * @param[in] data Buffer to write.
 * @param[in] size Size of buffer to write.
 *
 * @returns The number of bytes written.
 */
size_t uart_write(struct uart *uart, const unsigned char *data, size_t size);

/** Receives data over UART.
 *
 * @param[in,out] uart
 * @param[in] data Buffer to read into.
 * @param[in] size Size of buffer to read into.
 *
 * @returns The number of bytes read, which may be less than the size of the
 *  buffer.
 */
size_t uart_read(struct uart *uart, unsigned char *data, size_t size);

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

/** Blocks until UART is done transmitting.
 *
 * @param[in] uart UART instance to block on.
 */
void uart_sync(struct uart *uart);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//UART_H_
