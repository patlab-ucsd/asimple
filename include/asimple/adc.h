// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef ADC_H_
#define ADC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ADC structure. */
struct adc
{
	void *handle;
	uint8_t slots_configured;
};

/** Initializes the ADC.
 *
 * Initialization involves setting up timer 3 to trigger an interrupt every 1/8
 * of a second, and configuring slot 0 to get 14 bits of data from pin 16.
 *
 * This function allows initializing the following pins only:
 *  16, 29, 11
 *
 * @param[in,out] adc Pointer to the ADC object to initialize.
 * @param[in] pins Array of pin numbers to initialize.
 * @param[in] size Size of pin array.
 */
void adc_init(struct adc *adc, uint8_t pins[], size_t size);

/** Get a sample from the ADC. The sample and pins arrays have to be the same size.
 *
 * @param[in] adc Pointer to the ADC object to use.
 * @param[out] sample Data extracted from the ADC, if there is data available.
 * @param[in] pins The pins we want to get data from.
 * @param[in] size The number of pins we want to get data from.
 *
 * @returns True if there was data in the queue to extract, false otherwise.
 */
bool adc_get_sample(struct adc *adc, uint32_t sample[], uint8_t pins[], size_t size);

/** Trigger the ADC to collect a sample.
 *
 * This triggers the ADC to collect a single sample.
 */
void adc_trigger(struct adc *adc);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//ADC_H_
