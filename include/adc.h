// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef ADC_H_
#define ADC_H_

struct adc
{
	void *handle;
};

/** Initializes the ADC.
 *
 * Initialization involves setting up timer 3 to trigger an interrupt every 1/8
 * of a second, and configuring slot 0 to get 14 bits of data from pin 16.
 *
 * @param[in,out] adc Pointer to the ADC object to initialize.
 */
void adc_init(struct adc *adc);

/** Get a sample from the ADC.
 *
 * @param[in] adc Pointer to the ADC object to use.
 * @param[out] sample Data extracted from the ADC, if there is data available.
 *
 * @returns True if there was data in the queue to extract, false otherwise.
 */
bool adc_get_sample(struct adc *adc, uint32_t *sample);

/** Trigger the ADC to start collecting data.
 *
 * Once this is called, the ADC will continue to run based on the configuration
 * of timer 3.
 */
void adc_trigger(struct adc *adc);

#endif//ADC_H_
