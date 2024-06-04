// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <adc.h>

#include <stdatomic.h>

/** Configure ADC to:
 *
 * - Use the high-freq clock/2 as a clock
 * - Trigger on a rising edge of whatever enables it
 * - Trigger by software only
 * - Internal 2.0V reference
 * - Low power CLKMODE (turns clock off between samples)
 * - Low power mode (disables ADC between samples, has 50uS latency)
 * - Use Timer3 to keep sampling automatically after initial trigger
 */
static const am_hal_adc_config_t adc_config = {
	.eClock = AM_HAL_ADC_CLKSEL_HFRC_DIV2,
	.ePolarity = AM_HAL_ADC_TRIGPOL_RISING,
	.eTrigger = AM_HAL_ADC_TRIGSEL_SOFTWARE,
	.eReference = AM_HAL_ADC_REFSEL_INT_2P0,
	.eClockMode = AM_HAL_ADC_CLKMODE_LOW_POWER,
	.ePowerMode = AM_HAL_ADC_LPMODE1,
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

// Configure the pins on the redboard
static const am_hal_gpio_pincfg_t g_AM_PIN_16_ADCSE0 =
{
	.uFuncSel = AM_HAL_PIN_16_ADCSE0,
};

static const am_hal_gpio_pincfg_t g_AM_PIN_29_ADCSE1 =
{
	.uFuncSel = AM_HAL_PIN_29_ADCSE1,
};

static const am_hal_gpio_pincfg_t g_AM_PIN_11_ADCSE2 =
{
	.uFuncSel = AM_HAL_PIN_11_ADCSE2,
};

void adc_init(struct adc *adc, const uint8_t *pins, size_t size)
{
	adc->slots_configured = size;

	// Configure the pins given
	for(size_t i = 0; i < size; i++){
		if(pins[i] == 16){
			am_hal_gpio_pinconfig(16, g_AM_PIN_16_ADCSE0);
		}
		else if(pins[i] == 29){
			am_hal_gpio_pinconfig(29, g_AM_PIN_29_ADCSE1);
		}
		else if(pins[i] == 11){
			am_hal_gpio_pinconfig(11, g_AM_PIN_11_ADCSE2);
		}
		else{
			am_util_stdio_printf("Error - Pin cannot be configured with ADC\r\n");
			return;
		}
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

	// Configure the slots for the pins given
	for(size_t i = 0; i < size; i++){
		if(pins[i] == 16){
			slot_config.eChannel = AM_HAL_ADC_SLOT_CHSEL_SE0;
			am_hal_adc_configure_slot(adc->handle, 0, &slot_config);
		}
		else if(pins[i] == 29){
			slot_config.eChannel = AM_HAL_ADC_SLOT_CHSEL_SE1;
			am_hal_adc_configure_slot(adc->handle, 1, &slot_config);
		}
		else if(pins[i] == 11){
			slot_config.eChannel = AM_HAL_ADC_SLOT_CHSEL_SE2;
			am_hal_adc_configure_slot(adc->handle, 2, &slot_config);
		}
	}

	// Enable the ADC.
	am_hal_adc_enable(adc->handle);
	adc_timer_init();
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

bool adc_get_sample(struct adc *adc, uint32_t sample[], const uint8_t pins[], size_t size)
{
	if (AM_HAL_ADC_FIFO_COUNT(ADC->FIFO) < adc->slots_configured)
		return false;

	for(size_t i = 0; i < size; i++){
		uint32_t samples = 1;
		am_hal_adc_sample_t slot = {0};
		am_hal_adc_samples_read(adc->handle, true, NULL, &samples, &slot);

		// we got a slot-- is it for a pin we care about?
		for (size_t j = 0; j < size; ++j)
		{
			// Determine which slot it came from
			if ((slot.ui32Slot == 0 && pins[j] == 16) ||
				(slot.ui32Slot == 1 && pins[j] == 29) ||
				(slot.ui32Slot == 2 && pins[j] == 11))
			{
				// The returned ADC sample is from pin 16, 29, or 11
				// and the user requested that pin
				sample[j] = AM_HAL_ADC_FIFO_SAMPLE(slot.ui32Sample);
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
