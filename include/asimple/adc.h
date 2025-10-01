// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
/// @file

#ifndef ADC_H_
#define ADC_H_

#include "am_hal_adc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** ADC structure forward declaration. */
struct adc;

/** Returns the ADC instance.
 *
 * If it hasn't been initialized, it initializes the ADC instance. Before use
 * it must be enabled. This tracks the number of times it's been borrowed.
 *
 * (JV: OUTDATED?)
 * Initialization involves setting up timer 3 to trigger an interrupt every 1/8
 * of a second, and configuring slot 0 to get 14 bits of data from pin 16.
 *
 * Configures slots of the ADC to read from the pins specified
 * Can specify up to 8 pins (8 ADC slots)
 *
 * @param[in,out] adc Pointer to the ADC object to initialize.
 * @param[in] pins Array of pin numbers to initialize.
 * @param[in] size Size of pin array.
 */
struct adc *adc_get_instance(const uint8_t pins[], size_t size);
// struct adc *adc_get_instance_channels(struct adc *adc, const
// am_hal_adc_slot_chan_e channels[], size_t size);

/** Deinitializes the given adc structure, freeing resources held, including
 * the associated UART instance, once all borrowed instances are
 * de-initialized.
 *
 * @param[in] adc Pointer to adc structure to deinitialize.
 */
void adc_deinitialize(struct adc *adc);

/** Enables/wakes up the adc module.
 *
 * @param[in,out] adc Pointer to the adc structure to enable.
 *
 * @returns True on success, false if it cannot be enabled. This usually
 *  happens if the device is already awake.
 */
bool adc_enable(struct adc *adc);

/** Places the adc module to sleep.
 *
 * @param[in,out] adc Pointer to the adc structure to set to sleep.
 *
 * @returns True on success, false if it cannot be put to sleep.
 */
bool adc_sleep(struct adc *adc);

/** Get a batch of samples from the ADC.
 *
 * The sample and pins arrays must be the same size.
 * Samples will be placed at locations corresponding to pins[]
 *
 * @param[in] adc Pointer to the ADC object to use.
 * @param[out] out_samples Data extracted from the ADC, if there is data
 * available.
 * @param[in] pins The pins we want to get data from.
 * @param[in] size The number of pins we want to get data from.
 *
 * @returns True if there was data in the queue to extract, false otherwise.
 */
bool adc_get_sample(
	struct adc *adc, uint32_t out_samples[], const uint8_t pins[], size_t size
);

// As above, but specified in terms of ADC channels
bool adc_get_sample_channels(
	struct adc *adc, uint32_t out_samples[],
	const am_hal_adc_slot_chan_e channels[], size_t size
);

/** Trigger the ADC to collect a sample.
 *
 * This triggers the ADC to collect a single sample.
 */
void adc_trigger(struct adc *adc);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ADC_H_
