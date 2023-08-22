// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023
/// @file

#ifndef BMP280_H_
#define BMP280_H_

#include <spi.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure representing the BMP280 sensor */
struct bmp280
{
	struct spi_device *spi;
	uint16_t dig_T1;
	uint16_t dig_T2;
	uint16_t dig_T3;
	uint16_t dig_P1;
	uint16_t dig_P2;
	uint16_t dig_P3;
	uint16_t dig_P4;
	uint16_t dig_P5;
	uint16_t dig_P6;
	uint16_t dig_P7;
	uint16_t dig_P8;
	uint16_t dig_P9;
};

/** Initializes the BMP280 structure.
 *
 * @param[out] bmp280 BMP280 sensor object to initialize
 * @param[in,out] device The SPI object to use for communication with the
 *  BMP280 sensor. It should already be initialized.
 */
void bmp280_init(struct bmp280 *bmp280, struct spi_device *device);

/**
 * Reads the device and manufacturer ID of the BMP280 sensor.
 *
 * @param[in] bmp280 BMP280 sensor to read ID of.
 *
 * @returns A byte integer, which contains the ID information.
*/
uint8_t bmp280_read_id(struct bmp280 *bmp280);

/**
 * Reads registers from the BMP280 sensor.
 *
 * @param[in] bmp280 BMP280 sensor to read the registers from.
 * @param[in] addr BMP280 sensor address to read from.
 * @param[out] buffer Buffer that registers from the BMP280 sensor will be written into.
 * @param[in] size Number of bytes to read, starting at addr.
*/
void bmp280_read_register(struct bmp280 *bmp280, uint8_t addr, uint8_t *buffer, uint32_t size);

/**
 * Gets the raw temperature value from the temperature registers of the BMP280 sensor.
 *
 * @param[in] bmp280 BMP280 sensor to read the registers from.
 *
 * @returns The raw temperature value.
*/
uint32_t bmp280_get_adc_temp(struct bmp280 *bmp280);

/**
 * Gets the raw pressure value from the pressure registers of the BMP280 sensor.
 *
 * @param[in] bmp280 BMP280 sensor to read the registers from.
 *
 * @returns The raw pressure value.
*/
uint32_t bmp280_get_adc_pressure(struct bmp280 *bmp280);

/**
 * Converts the raw temperature from bmp280_get_adc_temp value into celsius.
 *
 * @param[in] bmp280 BMP280 sensor to read the registers from.
 * @param[in] raw_temp Raw temperature value.
 *
 * @returns The temperature in celsius as a double.
*/
double bmp280_compensate_T_double(struct bmp280 *bmp280, uint32_t raw_temp);

/**
 * Converts the raw pressure value from bmp280_get_adc_pressure into pascals.
 *
 * @param[in] bmp280 BMP280 sensor to read the registers from.
 * @param[in] raw_press Raw pressure value.
 * @param[in] raw_temp Raw temperature value.
 *
 * @returns The pressure in pascals as a double.
*/
double bmp280_compensate_P_double(struct bmp280 *bmp280, uint32_t raw_press, uint32_t raw_temp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//BMP280_H_
