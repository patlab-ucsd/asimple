// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023
/// @file

#ifndef FLASH_H_
#define FLASH_H_

#include <spi.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure representing the flash chip */
struct flash
{
	struct spi_device *spi;
};

/** Initializes the flash structure.
 *
 * @param[out] flash Flash object to initialize
 * @param[in,out] device The SPI object to use for communication with the
 *  physical flash chip. It should already be initialized.
 */
void flash_init(struct flash *flash, struct spi_device *device);


/** Reads the flash chip's status register.
 *
 * @param[in] rtc Flash to read the status register from.
 *
 * @returns The value of the register.
 */
uint8_t flash_read_status_register(struct flash *flash);

/**
 * Writes to flash status register to enable writing.
 *
 * @param[in,out] flash Flash to enable writing on.
*/
void flash_write_enable(struct flash *flash);

/**
 * Reads data from the flash chip.
 *
 * @param[in] flash Flash to read the data from.
 * @param[in] addr Flash address to read from.
 * @param[out] buffer Buffer that data from flash chip will be written into.
 * @param[in] size Number of bytes to read, starting at addr.
*/
void flash_read_data(struct flash *flash, uint32_t addr, uint8_t *buffer, uint32_t size);

/**
 * Writes data to the flash chip.
 *
 * @param[in,out] flash Flash chip to write data to.
 * @param[in] addr Address to begin writing at.
 * @param[in] buffer Buffer to hold data to be written.
 * @param[in] size Number of bytes in the buffer.
 *
 * @returns 0 if chip was busy and write failed, or 1 if write command was accepted.
*/
uint8_t flash_page_program(struct flash *flash, uint32_t addr, const uint8_t *buffer, uint32_t size);

/**
 * Erases a 4K sector of the flash chip.
 *
 * @param[in,out] flash Flash chip that contains a sector to be erased.
 * @param[in] addr An address within the sector that will be erased.
 *
 * @returns 0 if chip was busy and erase failed, or 1 if erase command was accepted.
*/
uint8_t flash_sector_erase(struct flash *flash, uint32_t addr);

/**
 * Reads the device and manufacturer ID of the flash chip.
 *
 * @param[in] flash Flash chip to read ID of.
 *
 * @returns A 4-byte integer, which contains 3 bytes of ID information.
*/
uint32_t flash_read_id(struct flash *flash);

/** Spinloops until the status register returns a cleared busy bit.
 *
 * @param[in] flash Flash to query status register from.
 */
void flash_wait_busy(struct flash *flash);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//FLASH_H_
