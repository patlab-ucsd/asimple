// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
// Copyright: Kristin Ebuengan, 2023
// Copyright: Melody Gill, 2023

#include <bmp280.h>
#include <spi.h>
#include <uart.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>

void bmp280_init(struct bmp280 *sensor, struct spi *spi)
{
	sensor->spi = spi;
}

uint8_t bmp280_read_id(struct bmp280 *bmp280)
{
	uint32_t sensorID = 0xD0;
	spi_write_continue(bmp280->spi, &sensorID, 1);
	uint32_t readBuffer = 0;
	spi_read(bmp280->spi, &readBuffer, 1);
	return (uint8_t)readBuffer;
}

void bmp280_read_register(struct bmp280 *bmp280, uint32_t addr, uint32_t *buffer, uint32_t size)
{
	addr |= 0x80;
	spi_write_continue(bmp280->spi, &addr, 1);
	spi_read(bmp280->spi, buffer, size);
}

uint32_t bmp280_get_adc_temp(struct bmp280 *bmp280)
{
	// take out of sleep mode
	// set temp oversampling
	uint32_t activate = 0b00100001;

	// write temp sampling to the register
	uint32_t addr = 0xF4 & 0x7F;
	uint32_t buffer = 0;
	uint8_t* ptr = &buffer;
	ptr[0] = addr;
	ptr[1] = activate;
	spi_write(bmp280->spi, &buffer, 2);

	uint32_t tempRegister = 0xFA;
	spi_write_continue(bmp280->spi, &tempRegister, 1);
	uint32_t readBuffer = 0;
	spi_read(bmp280->spi, &readBuffer, 3);
	uint8_t* ptr2 = &readBuffer;
	uint32_t temp = ptr2[2] + (ptr2[1] << 8) + (ptr2[0] << 16);
	temp = temp >> 4;
	return temp;
}

uint32_t bmp280_get_adc_pressure(struct bmp280 *bmp280)
{
	// take out of sleep mode
	// set temp oversampling
	uint32_t activate = 0b00000101;

	// write temp sampling to the register
	uint32_t addr = 0xF4 & 0x7F;
	uint32_t buffer = 0;
	uint8_t* ptr = &buffer;
	ptr[0] = addr;
	ptr[1] = activate;
	spi_write(bmp280->spi, &buffer, 2);

	uint32_t tempRegister = 0xF7;
	spi_write_continue(bmp280->spi, &tempRegister, 1);
	uint32_t readBuffer = 0;
	spi_read(bmp280->spi, &readBuffer, 3);
	uint8_t* ptr2 = &readBuffer;
	uint32_t temp = ptr2[2] + (ptr2[1] << 8) + (ptr2[0] << 16);
	temp = temp >> 4;
	return temp;
}

static uint16_t bmp280_unsigned_short_from_buffer(uint32_t* buffer)
{
	uint8_t* ptr = buffer;
	uint8_t msb = ptr[1];
	uint8_t lsb = ptr[0];

	uint16_t toReturn = (msb << 8) | (lsb & 0xff);
	return toReturn;
}

static int16_t bmp280_signed_short_from_buffer(uint32_t* buffer)
{
	uint8_t* ptr = buffer;
	uint8_t msb = ptr[1];
	uint8_t lsb = ptr[0];

	int16_t toReturn = (msb << 8) | (lsb & 0xff);
	return toReturn;
}

static uint32_t bmp280_get_t_fine(struct bmp280 *bmp280, uint32_t raw_temp)
{
	uint32_t dig_T1_buf;
	bmp280_read_register(bmp280, 0x88, &dig_T1_buf, 2);		//0x88 is lsb and 0x89 is msb
	uint16_t dig_T1 = bmp280_unsigned_short_from_buffer(&dig_T1_buf);

	uint32_t dig_T2_buf;
	bmp280_read_register(bmp280, 0x8A, &dig_T2_buf, 2);		//0x8A is lsb and 0x8B is msb
	int16_t dig_T2 = bmp280_signed_short_from_buffer(&dig_T2_buf);

	uint32_t dig_T3_buf;
	bmp280_read_register(bmp280, 0x8C, &dig_T3_buf, 2);		//0x8C is lsb and 0x8D is msb
	int16_t dig_T3 = bmp280_signed_short_from_buffer(&dig_T3_buf);

	uint32_t t_fine;
	double var1, var2;
	var1 = (((double)raw_temp)/16384.0 - ((double)dig_T1)/1024.0) * ((double)dig_T2);
	var2 = ((((double)raw_temp)/131072.0 - ((double)dig_T1)/8192.0) *
	(((double)raw_temp)/131072.0 - ((double) dig_T1)/8192.0)) * ((double)dig_T3);
	t_fine = (uint32_t)(var1 + var2);
	return t_fine;
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

	uint32_t dig_P1_buf;
	bmp280_read_register(bmp280, 0x8E, &dig_P1_buf, 2);		//0x8E is lsb and 0x8F is msb
	uint16_t dig_P1 = bmp280_unsigned_short_from_buffer(&dig_P1_buf);

	uint32_t dig_P2_buf;
	bmp280_read_register(bmp280, 0x90, &dig_P2_buf, 2);		//0x90 is lsb and 0x91 is msb
	int16_t dig_P2 = bmp280_signed_short_from_buffer(&dig_P2_buf);

	uint32_t dig_P3_buf;
	bmp280_read_register(bmp280, 0x92, &dig_P3_buf, 2);		//0x92 is lsb and 0x93 is msb
	int16_t dig_P3 = bmp280_signed_short_from_buffer(&dig_P3_buf);

	uint32_t dig_P4_buf;
	bmp280_read_register(bmp280, 0x94, &dig_P4_buf, 2);		//0x94 is lsb and 0x95 is msb
	int16_t dig_P4 = bmp280_signed_short_from_buffer(&dig_P4_buf);

	uint32_t dig_P5_buf;
	bmp280_read_register(bmp280, 0x96, &dig_P5_buf, 2);		//0x96 is lsb and 0x97 is msb
	int16_t dig_P5 = bmp280_signed_short_from_buffer(&dig_P5_buf);

	uint32_t dig_P6_buf;
	bmp280_read_register(bmp280, 0x98, &dig_P6_buf, 2);		//0x98 is lsb and 0x99 is msb
	int16_t dig_P6 = bmp280_signed_short_from_buffer(&dig_P6_buf);

	uint32_t dig_P7_buf;
	bmp280_read_register(bmp280, 0x9A, &dig_P7_buf, 2);		//0x9A is lsb and 0x9B is msb
	int16_t dig_P7 = bmp280_signed_short_from_buffer(&dig_P7_buf);

	uint32_t dig_P8_buf;
	bmp280_read_register(bmp280, 0x9C, &dig_P8_buf, 2);		//0x9C is lsb and 0x9D is msb
	int16_t dig_P8 = bmp280_signed_short_from_buffer(&dig_P8_buf);

	uint32_t dig_P9_buf;
	bmp280_read_register(bmp280, 0x9E, &dig_P9_buf, 2);		//0x9E is lsb and 0x9F is msb
	int16_t dig_P9 = bmp280_signed_short_from_buffer(&dig_P9_buf);

	double var1, var2, p;
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)dig_P5) * 2.0;
	var2 = (var2/4.0)+(((double)dig_P4) * 65536.0);
	var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0)*((double)dig_P1);
	if (var1 == 0.0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)raw_press;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double)dig_P9) * p * p / 2147483648.0;
	var2 = p * ((double)dig_P8) / 32768.0;
	p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
	return p;
}