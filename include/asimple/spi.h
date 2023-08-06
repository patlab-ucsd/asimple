// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef SPI_H_
#define SPI_H_

#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>

/** Structure holding SPI bus information and state. */
struct spi_bus
{
	void *handle;
	int iom_module;
	unsigned current_clock;
};

/** Structure holding SPI information and state. */
struct spi_device
{
	struct spi_bus *parent;
	int chip_select;
	unsigned clock;
};

enum spi_chip_select
{
	SPI_CS_0,
	SPI_CS_1,
	SPI_CS_2,
	SPI_CS_3 = 3,
};

/** Initializes the given MIO module for SPI use.
 *
 * The SPI bus is configured to set all of its devices to SPI mode 0.
 *
 * The hardware pins used depend on the Apollo version being used. Refer to the
 * BSP header file for the AM_BSP_GPIO_IOM* defines, which describe which pins
 * are being used based on the module selected.
 *
 * For example, on the Redboard Artemis ATP, the pins used are as follows for
 * module 0:
 *   CS0  - 11
 *   CS1  - 17
 *   CS2  - 14
 *   CS3  - 15
 *   MISO - 6
 *   MOSI - 7
 *   SCK  - 5
 *
 * On initialization, the hardware is set to sleep-- call spi_bus_enable to
 * turn on the hardware.
 *
 * @param[out] spi_bus Pointer to the spi bus structure to initialize.
 * @param[in] iom_module Index of the IOM module to use.
 */
void spi_bus_init(struct spi_bus *bus, uint32_t iom_module);

/** Releases all resources of the given spi bus object.
 *
 * This de-initializes the MIO module, returning the GPIO pins to their prior
 * configuration. FIXME should we set them to a known state?
 *
 * Note, spi devices associated with this bus should be destroyed first!
 */
void spi_bus_destroy(struct spi_bus *bus);

/** Enables/wakes up the SPI bus module.
 *
 * @param[in,out] spi_bus Pointer to the spi bus structure to enable.
 *
 * @returns True on success, false if it cannot be enabled. This usually
 *  happens if the device is already awake.
 */
bool spi_bus_enable(struct spi_bus *bus);

/** Places the SPI bus module to sleep.
 *
 * @param[in,out] spi_bus Pointer to the spi bus structure to set to sleep.
 *
 * @returns True on success, false if it cannot be put to sleep. This usually
 * happens if the SPI module is actively transfering data.
 */
bool spi_bus_sleep(struct spi_bus *bus);

/** Initializes a SPI device under the given bus.
 *
 * Which pin is used by the Apollo3 depends on the IOM in use.
 *
 * For IOM0:
 *   SPI_CS_0 - pin 11
 *   SPI_CS_1 - pin 17
 *   SPI_CS_2 - pin 14
 *   SPI_CS_3 - pin 15
 *
 * @param[in,out] spi_bus SPI bus to get a device from.
 * @param[out] device SPI device to initialize.
 * @param[in] chip_select Chip select ID to use.
 * @param[in] clock The clock speed in Hz. The actual hardware has limitations
 *  on the speeds and the actual clock will be rounded down to the closes
 *  supported one.
 */
void spi_bus_init_device(
	struct spi_bus *bus,
	struct spi_device *device,
	enum spi_chip_select chip_select,
	uint32_t clock);

/** Releases all resources of the given spi device object.
 *
 * This de-initializes the MIO module, returning the GPIO pins to their prior
 * configuration. FIXME should we set them to a known state?
 *
 * @param[in,out] device SPI device to destroy.
 */
void spi_device_destroy(struct spi_device *device);

/** Sets the SPI clock to the nearest supported value, rounding down.
 *
 * @param[in,out] device SPI structure to modify.
 * @param[in] clock Desired clock rate in Hertz.
 */
void spi_device_set_clock(struct spi_device *device, uint32_t clock);

/** Reads data (blocking) from a SPI device, sending a command byte beforehand.
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
 * @param[in,out] device Pointer to the spi device structure to use.
 * @param[in] command Command byte to send first.
 * @param[out] buffer Pointer to buffer to hold incoming data.
 * @param[in] size Size of the buffer.
 */
void spi_device_cmd_read(
	struct spi_device *device,
	uint8_t command,
	uint32_t *buffer,
	uint32_t size);

/** Writes data (blocking) to the SPI device, sending a command byte
 *  beforehand.
 *
 * FIXME is there any way to time out?
 *
 * This function will block until the write buffer is sent.
 *
 * See spi_device_read for an explanation as to why buffer is a uint32_t
 *  pointer.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] command Command byte to send first.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_cmd_write(
	struct spi_device *device,
	uint8_t command,
	const uint32_t *buffer,
	uint32_t size);

/** Reads data (blocking) from the SPI device.
 *
 * This sets the CS line to logical false (high) on completion.
 *
 * See spi_read for an explanation as to why buffer is a uint32_t pointer.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_read(
	struct spi_device *device, uint32_t *buffer, uint32_t size);

/** Writes data (blocking) to the SPI device.
 *
 * This sets the CS line to logical false (high) on completion.
 *
 * See spi_read for an explanation as to why buffer is a uint32_t pointer.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_write(
	struct spi_device *device, const uint32_t *buffer, uint32_t size);

/** Reads data (blocking) to the SPI device, and leave CS active (low).
 *
 * This leaves the CS line as logical true (low) on completion.
 *
 * See spi_read for an explanation as to why buffer is a uint32_t pointer.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_read_continue(
	struct spi_device *device, uint32_t *buffer, uint32_t size);

/** Writes data (blocking) to the SPI device, and leaves CS active (low).
 *
 * This sets the CS line to logical true (low) on completion.
 *
 * See spi_read for an explanation as to why buffer is a uint32_t pointer.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_write_continue(
	struct spi_device *device, const uint32_t *buffer, uint32_t size);

/** Write and read data to/from the SPI device simultaneously.
 *
 * This function will block until all of the wrte buffer is sent and an equal
 * number of bytes received in the read buffer.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] command Command byte to send first.
 * @param[in] rx_buffer Pointer to buffer with outgoing data.
 * @param[out] tx_buffer Pointer to buffer to hold incoming data.
 * @param[in] size Size of the buffers, should be the same for both.
 */
void spi_device_readwrite(
	struct spi_device *device,
	uint32_t command,
	uint32_t *rx_buffer,
	const uint32_t *tx_buffer,
	uint32_t size);

/** Toggles the SPI clock while sending 0xFF and keeping CS logical false
 *  (high).
 *
 * SD cards require some clocking after the CS line is deasserted-- this
 * function is meant to do this.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] size Number of bytes to clock.
 */
void spi_device_toggle(struct spi_device *device, uint32_t size);

#endif//SPI_H_
