// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023

#include <bmp280.h>
#include <spi.h>
#include <uart.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <sys/endian.h>

static uint32_t bmp280_get_t_fine(struct bmp280 *bmp280, uint32_t raw_temp)
{
	uint32_t t_fine;
	double var1, var2;
	var1 = (((double)raw_temp)/16384.0 - ((double)bmp280->dig_T1)/1024.0) * ((double)bmp280->dig_T2);
	var2 = ((((double)raw_temp)/131072.0 - ((double)bmp280->dig_T1)/8192.0) *
	(((double)raw_temp)/131072.0 - ((double)bmp280->dig_T1)/8192.0)) * ((double)bmp280->dig_T3);
	t_fine = (uint32_t)(var1 + var2);
	return t_fine;
}

void bmp280_init(struct bmp280 *bmp280, struct spi_device *device)
{
	bmp280->spi = device;

	// Read all compensation parameter storage data at once
	uint8_t dig_T_dig_P[12*2];
	bmp280_read_register(bmp280, 0x88, dig_T_dig_P, sizeof(dig_T_dig_P));

	bmp280->dig_T1 = le16dec(dig_T_dig_P);
	bmp280->dig_T2 = (int16_t)le16dec(dig_T_dig_P + 2);
	bmp280->dig_T3 = (int16_t)le16dec(dig_T_dig_P + 4);
	bmp280->dig_P1 = le16dec(dig_T_dig_P + 6);
	bmp280->dig_P2 = (int16_t)le16dec(dig_T_dig_P + 8);
	bmp280->dig_P3 = (int16_t)le16dec(dig_T_dig_P + 10);
	bmp280->dig_P4 = (int16_t)le16dec(dig_T_dig_P + 12);
	bmp280->dig_P5 = (int16_t)le16dec(dig_T_dig_P + 14);
	bmp280->dig_P6 = (int16_t)le16dec(dig_T_dig_P + 16);
	bmp280->dig_P7 = (int16_t)le16dec(dig_T_dig_P + 18);
	bmp280->dig_P8 = (int16_t)le16dec(dig_T_dig_P + 20);
	bmp280->dig_P9 = (int16_t)le16dec(dig_T_dig_P + 22);
}

uint8_t bmp280_read_id(struct bmp280 *bmp280)
{
	uint8_t buffer[1];
	bmp280_read_register(bmp280, 0xD0, buffer, 1);
	return buffer[0];
}

void bmp280_read_register(struct bmp280 *bmp280, uint8_t addr, uint8_t *buffer, uint32_t size)
{
	addr |= 0x80; // set top bit
	spi_device_write_continue(bmp280->spi, &addr, 1);
	spi_device_read(bmp280->spi, buffer, size);
}

void bmp280_write_register(struct bmp280 *bmp280, uint8_t addr, const uint8_t *buffer, uint32_t size)
{
	addr &= 0x7F; // clear top bit
	spi_device_cmd_write(bmp280->spi, addr, buffer, size);
}

uint32_t bmp280_get_adc_temp(struct bmp280 *bmp280)
{
	// take out of sleep mode
	// set temp oversampling
	uint8_t activate = 0b00100001;

	// write temp sampling to the register
	bmp280_write_register(bmp280, 0xF4, &activate, 1);

	uint8_t buffer[3] = {0};
	bmp280_read_register(bmp280, 0xFA, buffer, 3);
	uint32_t temp = buffer[2] + (buffer[1] << 8) + (buffer[0] << 16);
	return temp >> 4;
}

uint32_t bmp280_get_adc_pressure(struct bmp280 *bmp280)
{
	// take out of sleep mode
	// set temp oversampling
	uint8_t activate = 0b00000101;

	// write temp sampling to the register
	bmp280_write_register(bmp280, 0xF4, &activate, 1);

	uint8_t buffer[3] = {0};
	bmp280_read_register(bmp280, 0xF7, buffer, 3);
	uint32_t temp = buffer[2] + (buffer[1] << 8) + (buffer[0] << 16);
	return temp >> 4;
}

double bmp280_compensate_T_double(struct bmp280 *bmp280, uint32_t raw_temp)
{
	uint32_t t_fine = bmp280_get_t_fine(bmp280, raw_temp);
	double T = t_fine / 5120.0;
	return T;
}

double bmp280_compensate_P_double(struct bmp280 *bmp280, uint32_t raw_press, uint32_t raw_temp)
{
	uint32_t t_fine = bmp280_get_t_fine(bmp280, raw_temp);
	double var1, var2, p;
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)bmp280->dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)bmp280->dig_P5) * 2.0;
	var2 = (var2/4.0)+(((double)bmp280->dig_P4) * 65536.0);
	var1 = (((double)bmp280->dig_P3) * var1 * var1 / 524288.0 + ((double)bmp280->dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0)*((double)bmp280->dig_P1);
	if (var1 == 0.0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)raw_press;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double)bmp280->dig_P9) * p * p / 2147483648.0;
	var2 = p * ((double)bmp280->dig_P8) / 32768.0;
	p = p + (var1 + var2 + ((double)bmp280->dig_P7)) / 16.0;
	return p;
}
