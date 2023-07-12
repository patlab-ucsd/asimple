// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023

#include <flash.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <string.h>

#include <stdio.h>

#include <spi.h>
#include <uart.h>

struct flash
{
	struct spi *spi;
};

void flash_init(struct flash *flash, struct spi *spi)
{
	flash->spi = spi;
}

uint8_t flash_read_status_register(struct flash *flash)
{
	uint32_t writeBuffer = 0x05;
	spi_write_continue(flash->spi, &writeBuffer, 1);
	uint32_t readBuffer = 0;
	spi_read(flash->spi, &readBuffer, 1);
	return (uint8_t)readBuffer;
}

void flash_write_enable(struct flash *flash)
{
	uint32_t writeBuffer = 0x06;
	spi_write(flash->spi, &writeBuffer, 1);
}

void flash_read_data(struct flash *flash, uint32_t addr, uint32_t *buffer, uint32_t size)
{
	// Write command as least significant bit
	uint32_t toWrite = 0;
	uint32_t* tmpPtr = &toWrite;
	uint8_t* tmp = (uint8_t*) tmpPtr;
	tmp[0] = 0x03;
	tmp[1] = addr >> 16;
	tmp[2] = addr >> 8;
	tmp[3] = addr;

	spi_write_continue(flash->spi, &toWrite, 4);
	spi_read(flash->spi, buffer, size);
}

uint8_t flash_page_program(struct flash *flash, uint32_t addr, uint8_t *buffer, uint32_t size)
{
	// Enable writing and check that status register updated
	flash_write_enable(flash);
	uint8_t read = flash_read_status_register(flash);
	uint8_t mask = 0b00000010;
	read = read & mask;
	read = read >> 1;

	// Indicate failure if write enable didn't work
	if (read == 0) {
		return 0;
	}

	// Write command as least significant bit
	uint32_t toWrite = 0;
	uint32_t* tmpPtr = &toWrite;
	uint8_t* tmp = (uint8_t*) tmpPtr;
	tmp[0] = 0x02;
	tmp[1] = addr >> 16;
	tmp[2] = addr >> 8;
	tmp[3] = addr;
	spi_write_continue(flash->spi, &toWrite, 4);

	// Write the data
	uint32_t *data = malloc(((size+3)/4) * 4);
	memcpy(data, buffer, size);
	spi_write(flash->spi, data, size);
	free(data);
	data = NULL;
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
	if (read == 0) {
		return 0;
	}

	// Write command as least significant bit
	uint32_t toWrite = 0;
	uint32_t* tmpPtr = &toWrite;
	uint8_t* tmp = (uint8_t*) tmpPtr;
	tmp[0] = 0x20;
	tmp[1] = addr >> 16;
	tmp[2] = addr >> 8;
	tmp[3] = addr;
	spi_write(flash->spi, &toWrite, 4);
	return 1;
}

uint32_t flash_read_id(struct flash *flash)
{
	uint32_t writeBuffer = 0x9F;
	spi_write_continue(flash->spi, &writeBuffer, 1);
	uint32_t readBuffer = 0;
	spi_read(flash->spi, &readBuffer, 3);
	return readBuffer;
}