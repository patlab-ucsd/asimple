// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef SPI_H_
#define SPI_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Opaque structure holding SPI bus information and state. */
struct spi_bus;

/** Opaque structure holding SPI information and state. */
struct spi_device;

// FIXME how many instances are there?
enum spi_bus_instance
{
	SPI_BUS_0,
	SPI_BUS_1,
	SPI_BUS_2
};

enum spi_chip_select
{
	SPI_CS_0,
	SPI_CS_1,
	SPI_CS_2,
	SPI_CS_3 = 3,
};

/** Gets an instance of the SPI bus.
 *
 * The SPI bus is configured to set all of its devices to SPI mode 0.
 *
 * The hardware pins used depend on the Apollo version being used. Refer to the
 * BSP header file for the AM_BSP_GPIO_IOM* defines, which describe which pins
 * are being used based on the module selected.
 *
 * On initialization, the hardware is set to sleep-- call spi_bus_enable to
 * turn on the hardware.
 *
 * @param[in] instance SPI bus instance to get.
 * @returns A pointer to the requested instance.
 */
struct spi_bus *spi_bus_get_instance(enum spi_bus_instance instance);

/** Releases all resources of the given spi bus object.
 *
 * This de-initializes the MIO module, returning the GPIO pins to their prior
 * configuration. FIXME should we set them to a known state?
 *
 * Note, spi devices associated with this bus should be deinitialized first!
 */
void spi_bus_deinitialize(struct spi_bus *bus);

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

/** Gets a SPI device under the given bus.
 *
 * This tracks how many times the instance has been borrowed.
 *
 * Which pin is used by the Apollo3 depends on the IOM in use.
 *
 * For IOM0:
 *   SPI_CS_0 - pin 11
 *   SPI_CS_1 - pin 17
 *   SPI_CS_2 - pin 14
 *   SPI_CS_3 - pin 15
 *
 * For IOM1:
 *   SPI_CS_0 - pin 23
 *   SPI_CS_2 - pin 18
 *
 * @param[in,out] spi_bus SPI bus to get a device from.
 * @param[in] chip_select Chip select ID to use.
 * @param[in] clock The clock speed in Hz. The actual hardware has limitations
 *  on the speeds and the actual clock will be rounded down to the closes
 *  supported one.
 *
 * @returns A pointer to the SPI device.
 */
struct spi_device *spi_device_get_instance(
	struct spi_bus *bus, enum spi_chip_select, uint32_t clock
);

/** Releases all resources of the given spi device object.
 *
 * This de-initializes the MIO module, returning the GPIO pins to their prior
 * configuration, once all borrowed instances are returned.
 * FIXME should we set them to a known state?
 *
 * @param[in,out] device SPI device to deinitialize.
 */
void spi_device_deinitialize(struct spi_device *device);

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
 * @param[in,out] device Pointer to the spi device structure to use.
 * @param[in] command Command byte to send first.
 * @param[out] buffer Pointer to buffer to hold incoming data.
 * @param[in] size Size of the buffer.
 */
void spi_device_cmd_read(
	struct spi_device *device, uint8_t command, uint8_t *buffer, uint32_t size
);

/** Writes data (blocking) to the SPI device, sending a command byte
 *  beforehand.
 *
 * FIXME is there any way to time out?
 *
 * This function will block until the write buffer is sent.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] command Command byte to send first.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_cmd_write(
	struct spi_device *device, uint8_t command, const uint8_t *buffer,
	uint32_t size
);

/** Reads data (blocking) from the SPI device.
 *
 * This sets the CS line to logical false (high) on completion.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_read(struct spi_device *device, uint8_t *buffer, uint32_t size);

/** Writes data (blocking) to the SPI device.
 *
 * This sets the CS line to logical false (high) on completion.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_write(
	struct spi_device *device, const uint8_t *buffer, uint32_t size
);

/** Reads data (blocking) to the SPI device, and leave CS active (low).
 *
 * This leaves the CS line as logical true (low) on completion.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_read_continue(
	struct spi_device *device, uint8_t *buffer, uint32_t size
);

/** Writes data (blocking) to the SPI device, and leaves CS active (low).
 *
 * This sets the CS line to logical true (low) on completion.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] buffer Pointer to buffer with outgoing data.
 * @param[in] size Size of the buffer.
 */
void spi_device_write_continue(
	struct spi_device *device, const uint8_t *buffer, uint32_t size
);

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
void spi_device_cmd_readwrite(
	struct spi_device *device, uint32_t command, uint8_t *rx_buffer,
	const uint8_t *tx_buffer, uint32_t size
);

/** Write and read data to/from the SPI device simultaneously.
 *
 * This function will block until all of the wrte buffer is sent and an equal
 * number of bytes received in the read buffer. It does not set CS to inactive
 * on completion.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @param[in] rx_buffer Pointer to buffer with outgoing data.
 * @param[out] tx_buffer Pointer to buffer to hold incoming data.
 * @param[in] size Size of the buffers, should be the same for both.
 */
void spi_device_readwrite_continue(
	struct spi_device *device, uint8_t *rx_buffer, const uint8_t *tx_buffer,
	uint32_t size
);

/** Forces MOSI to the given logic level.
 *
 * This is mostly used for SD card functionality, to force the MOSI level high
 * while reading as apparently the cards malfunction if MOSI isn't high-- the
 * cards likely interpret something as a command.
 *
 * @param[in,out] device Pointer to the spi device to use.
 * @[aram[in] level Voltage level to set MOSI to (not logic level).
 */
void spi_device_hold_mosi(struct spi_device *device, bool level);

/** Returns MOSI to its normal SPI operation.
 *
 * @param[in,out] device Pointer to the spi device to use.
 */
void spi_device_release_mosi(struct spi_device *device);

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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SPI_H_
