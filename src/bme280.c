// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023

#include <bme280.h>
#include <spi.h>
#include <uart.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <sys/endian.h>

void bme280_init(struct bme280 *bme280, struct spi_device *device)
{
	bme280->spi = device;

	// Read all compensation parameter storage data at once
	uint8_t dig_part1[13*2];
	bme280_read_register(bme280, 0x88, dig_part1, sizeof(dig_part1));
	uint8_t dig_part2[6];
	bme280_read_register(bme280, 0xE1, dig_part2, sizeof(dig_part2));

	bme280->dig_T1 = le16dec(dig_part1);
	bme280->dig_T2 = (int16_t)le16dec(dig_part1 + 2);
	bme280->dig_T3 = (int16_t)le16dec(dig_part1 + 4);

	bme280->dig_P1 = le16dec(dig_part1 + 6);
	bme280->dig_P2 = (int16_t)le16dec(dig_part1 + 8);
	bme280->dig_P3 = (int16_t)le16dec(dig_part1 + 10);
	bme280->dig_P4 = (int16_t)le16dec(dig_part1 + 12);
	bme280->dig_P5 = (int16_t)le16dec(dig_part1 + 14);
	bme280->dig_P6 = (int16_t)le16dec(dig_part1 + 16);
	bme280->dig_P7 = (int16_t)le16dec(dig_part1 + 18);
	bme280->dig_P8 = (int16_t)le16dec(dig_part1 + 20);
	bme280->dig_P9 = (int16_t)le16dec(dig_part1 + 22);

	bme280->dig_H1 = dig_part1[25];
	bme280->dig_H2 = le16dec(dig_part2 + 0);
	bme280->dig_H3 = dig_part2[2];
	// I'm not sure why it's encoded this way, but the first byte has the top 8
	// bits, and the bottom 4 bits of the second byte have the bottom 4 bits of
	// H4...
	bme280->dig_H4 = (dig_part2[3] << 4) | (dig_part2[4] & 0xF);
	// I'm not sure why it's encoded this way, but the first byte has the lower
	// 4 bits in its upper nibble, and the top 8 bits are in the last byte.
	bme280->dig_H5 = (dig_part2[4] >> 4) | (dig_part2[5] << 4);

	uint8_t byte = 0b001;
	bme280_write_register(bme280, 0xF2, &byte, 1);
	byte = 0b00100100;
	bme280_write_register(bme280, 0xF4, &byte, 1);
}

uint8_t bme280_read_id(struct bme280 *bme280)
{
	uint8_t buffer[1];
	bme280_read_register(bme280, 0xD0, buffer, 1);
	return buffer[0];
}

void bme280_read_register(struct bme280 *bme280, uint8_t addr, uint8_t *buffer, uint32_t size)
{
	addr |= 0x80; // set top bit
	spi_device_write_continue(bme280->spi, &addr, 1);
	spi_device_read(bme280->spi, buffer, size);
}

void bme280_write_register(struct bme280 *bme280, uint8_t addr, const uint8_t *buffer, uint32_t size)
{
	addr &= 0x7F; // clear top bit
	spi_device_cmd_write(bme280->spi, addr, buffer, size);
}

inline static uint32_t be24dec(void* buff)
{
	unsigned char *data = buff;
	return data[0] << 16 | data[1] << 8 | data[2];
}

struct bme280_sample bme280_get_sample(struct bme280 *bme280)
{
	// take out of sleep mode
	// set temp oversampling
	uint8_t data = 0b00100101;

	// trigger sampling (also set T and P sampling to x1)
	bme280_write_register(bme280, 0xF4, &data, 1);
	do
	{
		bme280_read_register(bme280, 0xF3, &data, 1);
	}
	while (data != 0);

	uint8_t buffer[8];
	bme280_read_register(bme280, 0xF7, buffer, 8);
	struct bme280_sample result = {
		.raw_pressure = be24dec(buffer) >> 4,
		.raw_temperature = be24dec(buffer+3) >> 4,
		.raw_humidity = be16dec(buffer+6)
	};
	return result;
}

static uint32_t bme280_get_t_fine(struct bme280 *bme280, uint32_t raw_temp)
{
	double var1, var2;
	var1 = (raw_temp/16384.0 - bme280->dig_T1/1024.0) * bme280->dig_T2;
	var2 = ((raw_temp/131072.0 - bme280->dig_T1/8192.0) * (raw_temp/131072.0 - bme280->dig_T1/8192.0)) * bme280->dig_T3;
	return (uint32_t)(var1 + var2);
}

double bme280_compensate_T_double(struct bme280 *bme280, uint32_t raw_temp)
{
	uint32_t t_fine = bme280_get_t_fine(bme280, raw_temp);
	double T = t_fine / 5120.0;
	return T;
}

double bme280_compensate_P_double(struct bme280 *bme280, uint32_t raw_press, uint32_t raw_temp)
{
	uint32_t t_fine = bme280_get_t_fine(bme280, raw_temp);
	double var1, var2, p;
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)bme280->dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)bme280->dig_P5) * 2.0;
	var2 = (var2/4.0)+(((double)bme280->dig_P4) * 65536.0);
	var1 = (((double)bme280->dig_P3) * var1 * var1 / 524288.0 + ((double)bme280->dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0)*((double)bme280->dig_P1);
	if (var1 == 0.0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)raw_press;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double)bme280->dig_P9) * p * p / 2147483648.0;
	var2 = p * ((double)bme280->dig_P8) / 32768.0;
	p = p + (var1 + var2 + ((double)bme280->dig_P7)) / 16.0;
	return p;
}
