// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2024
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023
/// @file

#ifndef BME280_H_
#define BME280_H_

#include <spi.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure representing the BME280 sensor */
struct bme280
{
	struct spi_device *spi;
	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;
	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;
	uint8_t dig_H1;
	int16_t dig_H2;
	uint8_t dig_H3;
	int16_t dig_H4;
	int16_t dig_H5;
};

struct bme280_sample
{
	uint32_t raw_temperature;
	uint16_t raw_humidity;
	uint32_t raw_pressure;
};

/** Initializes the BME280 structure.
 *
 * @param[out] bme280 BME280 sensor object to initialize
 * @param[in,out] device The SPI object to use for communication with the
 *  BME280 sensor. It should already be initialized.
 */
void bme280_init(struct bme280 *bme280, struct spi_device *device);

/**
 * Reads the device and manufacturer ID of the BME280 sensor.
 *
 * @param[in] bme280 BME280 sensor to read ID of.
 *
 * @returns A byte integer, which contains the ID information.
*/
uint8_t bme280_read_id(struct bme280 *bme280);

/**
 * Reads registers from the BME280 sensor.
 *
 * @param[in] bme280 BME280 sensor to read the registers from.
 * @param[in] addr BME280 sensor address to read from.
 * @param[out] buffer Buffer that registers from the BME280 sensor will be written into.
 * @param[in] size Number of bytes to read, starting at addr.
*/
void bme280_read_register(struct bme280 *bme280, uint8_t addr, uint8_t *buffer, uint32_t size);

/**
 * Writes to registers on the BME280 sensor.
 *
 * @param[in] bme280 BME280 sensor to write to.
 * @param[in] addr BME280 sensor address to start writing to.
 * @param[in] buffer Buffer with data to write.
 * @param[in] size Number of bytes to write, starting at addr.
*/
void bme280_write_register(struct bme280 *bme280, uint8_t addr, const uint8_t *buffer, uint32_t size);

/** Trigger and collect a sample from all sensors.
 *
 */
struct bme280_sample bme280_get_sample(struct bme280 *bme280);

/**
 * Converts the raw temperature from bme280_get_adc_temp value into celsius.
 *
 * @param[in] bme280 BME280 sensor to read the registers from.
 * @param[in] raw_temp Raw temperature value.
 *
 * @returns The temperature in celsius as a double.
*/
double bme280_compensate_T_double(struct bme280 *bme280, uint32_t raw_temp);

/**
 * Converts the raw pressure value from bme280_get_adc_pressure into pascals.
 *
 * @param[in] bme280 BME280 sensor to read the registers from.
 * @param[in] raw_press Raw pressure value.
 * @param[in] raw_temp Raw temperature value.
 *
 * @returns The pressure in pascals as a double.
*/
double bme280_compensate_P_double(struct bme280 *bme280, uint32_t raw_press, uint32_t raw_temp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//BME280_H_
