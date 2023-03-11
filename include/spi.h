// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

/** Structure holding SPI information and state. */
struct spi
{
	void *handle;
};

/** Initialize the given MIO module for SPI use.
 *
 * The SPI peripheral is being initialized to run at 4MHz and using SPI mode 0.
 *
 * The hardware pins used depend on the Apollo version being used. Refer to the
 * BSP header file for the AM_BSP_GPIO_IOM* defines, which describe which pins
 * are being used based on the module selected.
 *
 * For example, on the Redboard Artemis ATP, the pins used are as follows for
 * module 0:
 *   CS   - 11
 *   MISO - 6
 *   MOSI - 7
 *   SCK  - 5
 *
 * @param[out] spi Pointer to the spi structure to initialize.
 * @param[in] iom_module Index of the IOM module to use.
 */
void spi_init(struct spi *spi, uint32_t iom_module);

/** Read data (blocking) from the SPI peripheral.
 *
 * This function will block until the read buffer is filled.
 *
 * The buffer is a pointer to a uint32_t for two reasons: 1. the Ambiq SDK uses
 * this, and 2. the MIO FIFO can only be read 4 bytes at a time, and the
 * internal SDK implementation uses this as an optimization, to read 4 bytes at
 * once if there are that many bytes in the FIFO. By passing in a uint32_t
 * pointer, this should make it so that the buffer is aligned to uint32_t
 * standards, and that it's safe to write 4 bytes at once without running afoul
 * of C anti-aliasing rules.
 *
 * @param[in,out] spi Pointer to the spi structure to use.
 * @param[in] command Command byte to send first.
 * @param[out] buffer Pointer to buffer to hold incoming data.
 * @param[in] size Size of the buffer.
 */
void spi_read(
	struct spi *spi, uint32_t command, uint32_t *buffer, uint32_t size);

/** Write data (blocking) to the SPI peripheral.
 *
 * This function will block until the write buffer is sent.
 *
 * See spi_read for an explanation as to why buffer is a uint32_t pointer.
 *
 * @param[in,out] spi Pointer to the spi structure to use.
 * @param[in] command Command byte to send first.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_write(
	struct spi *spi, uint32_t command, const uint32_t *buffer, uint32_t size);

#endif//SPI_H_
