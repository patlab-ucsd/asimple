// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef SPI_H_
#define SPI_H_

#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>

/** Structure holding SPI information and state. */
struct spi
{
	void *handle;
	int iom_module;
	int chip_select;
};

enum spi_chip_select
{
	SPI_CS_0,
//	SPI_CS_1, FIXME these are not used in IOM0 currently in the SDK
//	SPI_CS_2,
	SPI_CS_3 = 3,
};

/** Initializes the given MIO module for SPI use.
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
 * On initialization, the hardware is set to sleep-- call spi_enable to turn on
 * the hardware.
 *
 * @param[out] spi Pointer to the spi structure to initialize.
 * @param[in] iom_module Index of the IOM module to use.
 * @param[in] clock The clock speed in Hz. The actual hardware has limitations
 *  on the speeds and the actual clock will be rounded down to the closes
 *  supported one.
 */
void spi_init(struct spi *spi, uint32_t iom_module, uint32_t clock);

/** Selects which CS line to use.
 *
 * Which pin is used by the Apollo3 depends on the IOM in use.
 *
 * For IOM0:
 *   SPI_CS_0 - pin 11
 *   SPI_CS_3 - pin 15
 *
 * @param[in,out] spi Spi structure to update the currenctly selected chip
 *  select line.
 * @param[in] chip_select Chip select ID to use.
 */
void spi_chip_select(struct spi *spi, enum spi_chip_select chip_select);

/** Releases all resources of the given spi object.
 *
 * This de-initializes the MIO module, returning the GPIO pins to their prior
 * configuration. FIXME should we set them to a known state?
 */
void spi_destroy(struct spi *spi);

/** Enables/wakes up the SPI module.
 *
 * @param[in,out] spi Pointer to the spi structure to enable.
 *
 * @returns True on success, false if it cannot be enabled. This usually
 *  happens if the device is already awake.
 */
bool spi_enable(struct spi *spi);

/** Places the SPI module to sleep.
 *
 * @param[in,out] spi Pointer to the spi structure to set to sleep.
 *
 * @returns True on success, false if it cannot be put to sleep. This usually
 * happens if the SPI module is actively transfering data.
 */
bool spi_sleep(struct spi *spi);

/** Reads data (blocking) from the SPI peripheral.
 *
 * FIXME is there any way to time out?
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
void spi_cmd_read(
	struct spi *spi, uint32_t command, uint32_t *buffer, uint32_t size);

/** Write data (blocking) to the SPI peripheral.
 *
 * FIXME is there any way to time out?
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
void spi_cmd_write(
	struct spi *spi, uint32_t command, const uint32_t *buffer, uint32_t size);

void spi_read(struct spi *spi, uint32_t *buffer, uint32_t size);

void spi_write(struct spi *spi, const uint32_t *buffer, uint32_t size);

void spi_read_continue(struct spi *spi, uint32_t *buffer, uint32_t size);

void spi_write_continue(
	struct spi *spi, const uint32_t *buffer, uint32_t size);

/** Write and read data to/from the SPI peripheral simultaneously.
 *
 * This function will block until all of the wrte buffer is sent and an equal
 * number of bytes received in the read buffer.
 *
 * @param[in,out] spi Pointer to the spi structure to use.
 * @param[in] command Command byte to send first.
 * @param[in] rx_buffer Pointer to buffer with outgoing data.
 * @param[out] tx_buffer Pointer to buffer to hold incoming data.
 * @param[in] size Size of the buffers, should be the same for both.
 */
void spi_readwrite(
	struct spi *spi,
	uint32_t command,
	uint32_t *rx_buffer,
	const uint32_t *tx_buffer,
	uint32_t size);

#endif//SPI_H_
