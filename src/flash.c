// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023

#include <flash.h>
#include <spi.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void flash_init(struct flash *flash, struct spi_device *device)
{
	flash->spi = device;
}

uint8_t flash_read_status_register(struct flash *flash)
{
	uint8_t writeBuffer = 0x05;
	spi_device_write_continue(flash->spi, &writeBuffer, 1);
	uint8_t readBuffer = 0;
	spi_device_read(flash->spi, &readBuffer, 1);
	return (uint8_t)readBuffer;
}

void flash_wait_busy(struct flash *flash)
{
	uint8_t writeBuffer = 0x05;
	spi_device_write_continue(flash->spi, &writeBuffer, 1);
	uint8_t readBuffer = 0;
	do
	{
		spi_device_read_continue(flash->spi, &readBuffer, 1);
	}
	while (readBuffer & 0x01);
	// This has the potential to waste sime cycles as we need to bring down the
	// CS line, and only way I know how to do that is to complete a transaction
	// with the continue flag unset.
	spi_device_read(flash->spi, &readBuffer, 1);
}

void flash_write_enable(struct flash *flash)
{
	uint8_t writeBuffer = 0x06;
	spi_device_write(flash->spi, &writeBuffer, 1);
}

void flash_read_data(
	struct flash *flash, uint32_t addr, uint8_t *buffer, uint32_t size
)
{
	// Write command as least significant bit
	uint8_t toWrite[] = {
		0x03,
		addr >> 16,
		addr >> 8,
		addr,
	};
	static_assert(sizeof(toWrite) == 4, "guessed array size wrong");

	spi_device_write_continue(flash->spi, toWrite, 4);
	spi_device_read(flash->spi, buffer, size);
}

uint8_t flash_page_program(
	struct flash *flash, uint32_t addr, const uint8_t *buffer, uint32_t size
)
{
	// Enable writing and check that status register updated
	flash_write_enable(flash);
	uint8_t read = flash_read_status_register(flash);
	uint8_t mask = 0b00000010;
	read = read & mask;
	read = read >> 1;

	// Indicate failure if write enable didn't work
	if (read == 0)
	{
		return 0;
	}

	// Write command as least significant bit
	uint8_t toWrite[4] = {
		0x02,
		addr >> 16,
		addr >> 8,
		addr,
	};
	spi_device_write_continue(flash->spi, toWrite, 4);

	// Write the data
	spi_device_write(flash->spi, buffer, size);
	return 1;
}

uint8_t flash_sector_erase(struct flash *flash, uint32_t addr)
{
	// Enable writing and check that status register updated
	flash_write_enable(flash);
	uint8_t read = flash_read_status_register(flash);
	uint8_t mask = 0b00000010;
	read = read & mask;
	read = read >> 1;

	// Indicate failure if write enable didn't work
	if (read == 0)
	{
		return 0;
	}

	// Write command as least significant bit
	uint8_t toWrite[4] = {
		0x20,
		addr >> 16,
		addr >> 8,
		addr,
	};
	spi_device_write(flash->spi, toWrite, 4);
	return 1;
}

uint32_t flash_read_id(struct flash *flash)
{
	uint8_t writeBuffer = 0x9F;
	spi_device_write_continue(flash->spi, &writeBuffer, 1);
	uint8_t readBuffer[3] = {0};
	spi_device_read(flash->spi, readBuffer, 3);
	uint32_t result = readBuffer[0] << 16 | readBuffer[1] << 8 | readBuffer[2];
	return result;
}
