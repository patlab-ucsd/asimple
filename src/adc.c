// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <adc.h>

#include <stdatomic.h>
#include <string.h>

struct adc
{
	void *handle;
	uint8_t slots_configured;
	am_hal_adc_slot_chan_e slot_channels[8]; // actually an am_hal_adc_slot_chan_e
	atomic_uint refcount;
};

static struct adc adc;

/** Configure ADC to:
 *
 * - Use the high-freq clock as a clock
 * - Trigger on a rising edge of whatever enables it
 * - Trigger by software only
 * - Internal 2.0V reference
 * - High power CLKMODE (remains active between samples)
 * - High power mode (low latency when triggering new sample)
 */
static const am_hal_adc_config_t adc_config = {
	.eClock = AM_HAL_ADC_CLKSEL_HFRC,
	.ePolarity = AM_HAL_ADC_TRIGPOL_RISING,
	.eTrigger = AM_HAL_ADC_TRIGSEL_SOFTWARE,
	.eReference = AM_HAL_ADC_REFSEL_INT_2P0,
	.eClockMode = AM_HAL_ADC_CLKMODE_LOW_LATENCY,
	.ePowerMode = AM_HAL_ADC_LPMODE0,
	.eRepeat = AM_HAL_ADC_SINGLE_SCAN
};

// ADC handle used by the interrupt, registered by adc_init
// This is a volatile pointer, as it is set by a function outside the ISR (and
// while extremely not recommended, can be changed by said function between ISR
// calls)
static void *volatile interrupt_adc_handle;

// This is a weak symbol for the ADC ISR
void am_adc_isr(void)
{
	uint32_t status;
	// Cache the pointer, as it can't change while we're inside an ISR
	void *handle = interrupt_adc_handle;

	// Clear timer 3 interrupt.
	am_hal_adc_interrupt_status(handle, &status, true);
	am_hal_adc_interrupt_clear(handle, status);
}

/** Timer3 is the only one that can work with the ADC. Configure it:
 *
 * - Don't link both counters from timer A and B to form a 32-bit counter
 * - Setup to do PWM output, and use the low-freq clock / 32 as the clock source
 * - Don't setup timerA
 */
static const am_hal_ctimer_config_t timer3_config = {
	.ui32Link = 0,
	.ui32TimerAConfig = (AM_HAL_CTIMER_FN_PWM_REPEAT | AM_HAL_CTIMER_LFRC_32HZ),
	.ui32TimerBConfig = 0,
};

// FIXME this has a bunch of hard-coded parameters
static void adc_timer_init(void)
{
	// Only CTIMER 3 supports the ADC.
	static const unsigned TIMERNUM = 3;

	// Turn on the low-freq RC clock (1024Hz)
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_LFRC_START, 0);

	// Set up timer 3A so start by clearing it.
	am_hal_ctimer_clear(TIMERNUM, AM_HAL_CTIMER_TIMERA);

	// Configure the timer to count 32Hz LFRC clocks but don't start it yet.
	am_hal_ctimer_config(TIMERNUM, (am_hal_ctimer_config_t*)&timer3_config);

	// Compute CMPR value needed for desired period based on a 32HZ clock.
	// 32 ticks per sec * 1/8 second = 4 ticks period
	// PWM 50% duty cycle
	uint32_t period = 32 / 8;
	am_hal_ctimer_period_set(
		TIMERNUM, AM_HAL_CTIMER_TIMERA, period, (period >> 1));

	// Set up timer 3A as the trigger source for the ADC.
	am_hal_ctimer_adc_trigger_enable();

	// Start timer 3A.
	am_hal_ctimer_start(TIMERNUM, AM_HAL_CTIMER_TIMERA);
}


// * *****************************************************************************
// *
// *                      Settings for each ADC Channel
// *
// * *****************************************************************************

typedef struct channel_settings {
	am_hal_adc_slot_chan_e channel;
	uint32_t pin_p; // primary/positive pin (-1 for channel with no pins)
	uint32_t pin_n; // negative pin for diff channels (else -1)
	uint32_t gpio_funcsel_p; // funcsel for the pin (-1 if no pin)
	uint32_t gpio_funcsel_n; // funcsel for the negative pin (-1 if not differential)
} channel_settings_t;

static const uint32_t NO_PIN = 0xFFFFFFFF;

static const channel_settings_t g_channel_settings[] = {
	// Channel                   pin_p    pin_n  funcsel_p                funcsel_n
	// =====
	// Single-ended channels
	{AM_HAL_ADC_SLOT_CHSEL_SE0,     16,  NO_PIN, AM_HAL_PIN_16_ADCSE0,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE1,     29,  NO_PIN, AM_HAL_PIN_29_ADCSE1,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE2,     11,  NO_PIN, AM_HAL_PIN_11_ADCSE2,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE3,     31,  NO_PIN, AM_HAL_PIN_31_ADCSE3,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE4,     32,  NO_PIN, AM_HAL_PIN_32_ADCSE4,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE5,     33,  NO_PIN, AM_HAL_PIN_33_ADCSE5,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE6,     34,  NO_PIN, AM_HAL_PIN_34_ADCSE6,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE7,     35,  NO_PIN, AM_HAL_PIN_35_ADCSE7,    NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE8,     13,  NO_PIN, AM_HAL_PIN_13_ADCD0PSE8, NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_SE9,     12,  NO_PIN, AM_HAL_PIN_12_ADCD0NSE9, NO_PIN },
	// Differential channels.
	{AM_HAL_ADC_SLOT_CHSEL_DF0,     13,      12, AM_HAL_PIN_13_ADCD0PSE8, AM_HAL_PIN_12_ADCD0NSE9 },
	{AM_HAL_ADC_SLOT_CHSEL_DF1,     14,      15, AM_HAL_PIN_14_ADCD1P,    AM_HAL_PIN_15_ADCD1N },
	// Miscellaneous other signals.
	{AM_HAL_ADC_SLOT_CHSEL_TEMP, NO_PIN, NO_PIN, NO_PIN,                  NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_BATT, NO_PIN, NO_PIN, NO_PIN,                  NO_PIN },
	{AM_HAL_ADC_SLOT_CHSEL_VSS,  NO_PIN, NO_PIN, NO_PIN,                  NO_PIN },

};
//NOTE: ADC channels (type am_hal_adc_slot_chan_e) come from am_hal_adc.h
const int ADC_CHANNEL_MAX = sizeof(g_channel_settings) / sizeof(g_channel_settings[1]);


// * *****************************************************************************
// *
// *                           Main Functions
// *
// * *****************************************************************************

static void adc_init_channels(struct adc *adc, const am_hal_adc_slot_chan_e* channels, size_t size)
{
	if (size > 8) {
		am_util_stdio_printf("Error - ADC can't take more than 8 slots\r\n");
		while(1);
	}

	adc->slots_configured = size;

	// Configure the pins given
	for (size_t i = 0; i < size; i++){
		channel_settings_t settings = g_channel_settings[channels[i]];

		// If 2 pins specified, it's a differential channel
		if (settings.pin_p != NO_PIN && settings.pin_n != NO_PIN) {
			am_util_stdio_printf("Error - ADC Differential channels not yet implemented, panicking\r\n");
			while(1);

		// If 1 pin specified, it's a normal single-ended channel, set it's funcsel appropriately
		} else if (settings.pin_p != NO_PIN) {
			const am_hal_gpio_pincfg_t cfg = { .uFuncSel = settings.gpio_funcsel_p };

			int status = am_hal_gpio_pinconfig(settings.pin_p, cfg);
			if (status != AM_HAL_STATUS_SUCCESS) {
				am_util_stdio_printf("Error - Couldnt configure pin %d, status=0x%x\r\n",
						settings.pin_p, status);
				while(1);
			}
		}

		// Else, misc ADC channels take no pins (bat VSS temp)
	}

	// Initialize the ADC and get the handle.
	if (am_hal_adc_initialize(0, &adc->handle) != AM_HAL_STATUS_SUCCESS)
	{
		adc->handle = NULL;
		interrupt_adc_handle = NULL;
		return;
	}
	interrupt_adc_handle = adc->handle;

	// Power on the ADC.
	// Don't save power state (actually can't-- state is saved when switching
	// to SLEEP, restored on WAKE)
	int result = am_hal_adc_power_control(
		adc->handle, AM_HAL_SYSCTRL_WAKE, false);

	if (result != AM_HAL_STATUS_SUCCESS)
	{
		am_util_stdio_printf("Error - ADC power on failed.\r\n");
		return;
	}

	// Configure the ADC.
	result = am_hal_adc_configure(adc->handle, (am_hal_adc_config_t*)&adc_config);
	if (result != AM_HAL_STATUS_SUCCESS)
	{
		am_util_stdio_printf("Error - configuring ADC failed.\r\n");
		return;
	}

	// Setup blank/empty slot config first
	am_hal_adc_slot_config_t slot_config = {
		.bEnabled       = false,
		.bWindowCompare = false,
		.eChannel       = AM_HAL_ADC_SLOT_CHSEL_SE0,
		.eMeasToAvg     = AM_HAL_ADC_SLOT_AVG_1,
		.ePrecisionMode = AM_HAL_ADC_SLOT_14BIT,
	};

	// Set all slots as unused by default...
	am_hal_adc_configure_slot(adc->handle, 0, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 1, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 2, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 3, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 4, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 5, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 6, &slot_config);
	am_hal_adc_configure_slot(adc->handle, 7, &slot_config);

	// Configure enabled slots shared settings
	slot_config.bEnabled       = true;
	slot_config.bWindowCompare = true;
	// Already configured eMeasToAvg = AM_HAL_ADC_SLOT_AVG_1

	// Configure the input slot
	slot_config.ePrecisionMode = AM_HAL_ADC_SLOT_14BIT;

	// Configure the slots with the given channels
	for (size_t i = 0; i < size; i++){
		slot_config.eChannel = channels[i];
		am_hal_adc_configure_slot(adc->handle, i, &slot_config);

		//also save which channels went to which slots
		adc->slot_channels[i] = channels[i];

		am_util_stdio_printf("Configure slot %d to channel %d\n", i, channels[i]); //DEBUG
	}

	// Enable the ADC.
	am_hal_adc_enable(adc->handle);
	NVIC_EnableIRQ(ADC_IRQn);

	// Enable the ADC interrupts in the ADC.
	am_hal_adc_interrupt_enable(
		adc->handle,
		AM_HAL_ADC_INT_WCINC |
		AM_HAL_ADC_INT_WCEXC |
		AM_HAL_ADC_INT_FIFOOVR2 |
		AM_HAL_ADC_INT_FIFOOVR1 |
		AM_HAL_ADC_INT_SCNCMP |
		AM_HAL_ADC_INT_CNVCMP);
}

// Converts a pin number to a single-ended ADC channel, if possible
am_hal_adc_slot_chan_e adc_channel_for_pin(uint8_t pin) {

	// Loop over single-ended channels, see if any match
	for (int i = AM_HAL_ADC_SLOT_CHSEL_SE0; i <= AM_HAL_ADC_SLOT_CHSEL_SE9; i++) {

		//sanity check assert: all channels should be in their own index
		if (g_channel_settings[i].channel != i) {
			am_util_stdio_printf("Error - asimple ADC channel settings table broken, panicking\r\n");
			while(1);
		}

		if (g_channel_settings[i].pin_p == pin) { return i; }
	}

	// This pin matches no channel
	am_util_stdio_printf("Error - Pin %d cannot be configured with ADC\r\n", pin);
	while(1);
}

struct adc *adc_get_instance(const uint8_t *pins, size_t size)
{
	if (!adc.handle)
	{
		if (size > 8) {
			am_util_stdio_printf("Error - ADC can't take more than 8 slots\r\n");
			while(1);
		}

		// Convert pins to channels, then call the main function
		am_hal_adc_slot_chan_e channels[8] = {};
		for (size_t i = 0; i < size; i++)
			{ channels[i] = adc_channel_for_pin(pins[i]); }

		adc_init_channels(&adc, channels, size);
		adc_sleep(&adc);
	}
	adc.refcount++;
	return &adc;
}

void adc_deinitialize(struct adc *adc)
{
	if (adc->refcount)
	{
		if (!--(adc->refcount))
		{
			NVIC_DisableIRQ(ADC_IRQn);
			// We need to find what pins are configured and disable them.
			for (uint8_t i = 0; i < adc->slots_configured; ++i)
			{
				for (uint8_t j = 0; j < ADC_CHANNEL_MAX; ++j)
				{
					if (adc->slot_channels[i] == g_channel_settings[j].channel)
					{
						if (g_channel_settings[j].pin_p != NO_PIN)
						{
							am_hal_gpio_pinconfig(g_channel_settings[j].pin_p, g_AM_HAL_GPIO_DISABLE);
						}
						if (g_channel_settings[j].pin_n != NO_PIN)
						{
							am_hal_gpio_pinconfig(g_channel_settings[j].pin_n, g_AM_HAL_GPIO_DISABLE);
						}
						break;
					}
				}
			}

			am_hal_adc_power_control(adc->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
			am_hal_adc_deinitialize(adc->handle);
			memset(adc, 0, sizeof(*adc));
		}
	}
}

bool adc_sleep(struct adc *adc)
{
	NVIC_DisableIRQ(ADC_IRQn);

	// Note that turning off the hardware resets registers, which is why we
	// request saving the state
	// Also, spinloop while the device is busy
	// Implementation based off spi.c
	int status;
	do
	{
		status = am_hal_adc_power_control(adc->handle, AM_HAL_SYSCTRL_DEEPSLEEP, true);
	} while (status == AM_HAL_STATUS_IN_USE);

	// We need to find what pins are configured and disable them.
	for (uint8_t i = 0; i < adc->slots_configured; ++i)
	{
		for (uint8_t j = 0; j < ADC_CHANNEL_MAX; ++j)
		{
			if (adc->slot_channels[i] == g_channel_settings[j].channel)
			{
				if (g_channel_settings[j].pin_p != NO_PIN)
				{
					am_hal_gpio_pinconfig(g_channel_settings[j].pin_p, g_AM_HAL_GPIO_DISABLE);
				}
				if (g_channel_settings[j].pin_n != NO_PIN)
				{
					am_hal_gpio_pinconfig(g_channel_settings[j].pin_n, g_AM_HAL_GPIO_DISABLE);
				}
				break;
			}
		}
	}
	return true;
}

bool adc_enable(struct adc *adc)
{
	// This can fail if there is no saved state, which indicates we've never
	// gone asleep
	int status = am_hal_adc_power_control(adc->handle, AM_HAL_SYSCTRL_WAKE, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}

	// We need to find what pins are configured and turn them on.
	for (uint8_t i = 0; i < adc->slots_configured; ++i)
	{
		for (uint8_t j = 0; j < ADC_CHANNEL_MAX; ++j)
		{
			if (adc->slot_channels[i] == g_channel_settings[j].channel)
			{
				if (g_channel_settings[j].pin_p != NO_PIN)
				{
					const am_hal_gpio_pincfg_t cfg = { .uFuncSel = g_channel_settings[j].gpio_funcsel_p};
					am_hal_gpio_pinconfig(g_channel_settings[j].pin_p, cfg);
				}
				if (g_channel_settings[j].pin_n != NO_PIN)
				{
					const am_hal_gpio_pincfg_t cfg = { .uFuncSel = g_channel_settings[j].gpio_funcsel_n};
					am_hal_gpio_pinconfig(g_channel_settings[j].pin_n, cfg);
				}
				break;
			}
		}
	}

	NVIC_EnableIRQ(ADC_IRQn);
	return true;
}

// Helper function: converts pins to channels and calls adc_get_sample_channels
bool adc_get_sample(struct adc *adc, uint32_t out_samples[], const uint8_t pins[], size_t size) {
	if (size > 8) {
		am_util_stdio_printf("Error - ADC can't take more than 8 slots\r\n");
		while(1);
	}

	// Convert pins to channels, then call the main function
	am_hal_adc_slot_chan_e channels[8] = {};
	for (size_t i = 0; i < size; i++)
		{ channels[i] = adc_channel_for_pin(pins[i]); }

	return adc_get_sample_channels(adc, out_samples, channels, size);
}

bool adc_get_sample_channels(struct adc *adc, uint32_t out_samples[], const am_hal_adc_slot_chan_e req_channels[], size_t size) {
	if (AM_HAL_ADC_FIFO_COUNT(ADC->FIFO) < adc->slots_configured)
		return false;

	for (size_t i = 0; i < size; i++) {
		uint32_t num_samples = 1; // in/out, is # samples requested and # received
		am_hal_adc_sample_t data = {0};
		am_hal_adc_samples_read(adc->handle, true, NULL, &num_samples, &data);

		// Some asserts for sanity checking
		if (num_samples != 1)
			{ am_util_stdio_printf("Error: adc returned no samples??\n"); while(1); }
		if (data.ui32Slot >= adc->slots_configured)
			{ am_util_stdio_printf("Error: adc returned sample for invalid slot?\n"); while(1); }

		// Lookup what channel we had configured for the slot the sample is for
		uint32_t data_channel = adc->slot_channels[data.ui32Slot];

		// check which index user requested this channel at (if any)
		for (size_t j = 0; j < size; ++j) {
			if (req_channels[j] == data_channel) {
				out_samples[j] = AM_HAL_ADC_FIFO_SAMPLE(data.ui32Sample);
				break;
			}
		}
	}
	return true;
}

void adc_trigger(struct adc *adc)
{
	// Kick Start Timer 3 with an ADC software trigger in REPEAT used.
	am_hal_adc_sw_trigger(adc->handle);
}
