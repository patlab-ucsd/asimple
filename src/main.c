// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include <uart.h>
#include <adc.h>
#include <spi.h>
#include <lora.h>
#include <gpio.h>
#include <syscalls.h>

static struct uart uart;
static struct adc adc;

int main(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();
	am_hal_sysctrl_fpu_enable();
	am_hal_sysctrl_fpu_stacking_enable(true);

	// Init UART
	uart_init(&uart, UART_INST0);

	// Initialize the ADC.
	uint8_t pins[1];
	pins[0] = 16;
	size_t size = 1;
	adc_init(&adc, pins, size);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();

	// Print the banner.
	am_util_stdio_terminal_clear();
	am_util_stdio_printf("Hello World!\r\n\r\n");
	syscalls_uart_init(&uart);
	printf("Hello World2!\r\n\r\n");

	// Print the device info.
	am_util_id_t device_id;
	am_util_id_device(&device_id);
	am_util_stdio_printf("Vendor Name: %s\r\n", device_id.pui8VendorName);
	am_util_stdio_printf("Device type: %s\r\n", device_id.pui8DeviceName);

	am_util_stdio_printf("Qualified: %s\r\n",
						 device_id.sMcuCtrlDevice.ui32Qualified ?
						 "Yes" : "No");

	am_util_stdio_printf("Device Info:\r\n"
						 "\tPart number: 0x%08X\r\n"
						 "\tChip ID0:	0x%08X\r\n"
						 "\tChip ID1:	0x%08X\r\n"
						 "\tRevision:	0x%08X (Rev%c%c)\r\n",
						 device_id.sMcuCtrlDevice.ui32ChipPN,
						 device_id.sMcuCtrlDevice.ui32ChipID0,
						 device_id.sMcuCtrlDevice.ui32ChipID1,
						 device_id.sMcuCtrlDevice.ui32ChipRev,
						 device_id.ui8ChipRevMaj, device_id.ui8ChipRevMin );

	// If not a multiple of 1024 bytes, append a plus sign to the KB.
	uint32_t mem_size = ( device_id.sMcuCtrlDevice.ui32FlashSize % 1024 ) ? '+' : 0;
	am_util_stdio_printf("\tFlash size:  %7d (%d KB%s)\r\n",
						 device_id.sMcuCtrlDevice.ui32FlashSize,
						 device_id.sMcuCtrlDevice.ui32FlashSize / 1024,
						 &mem_size);

	mem_size = ( device_id.sMcuCtrlDevice.ui32SRAMSize % 1024 ) ? '+' : 0;
	am_util_stdio_printf("\tSRAM size:   %7d (%d KB%s)\r\n\r\n",
						 device_id.sMcuCtrlDevice.ui32SRAMSize,
						 device_id.sMcuCtrlDevice.ui32SRAMSize / 1024,
						 &mem_size);

	// Print the compiler version.
	am_hal_uart_tx_flush(uart.handle);
	am_util_stdio_printf("App Compiler:	%s\r\n", COMPILER_VERSION);
	am_util_stdio_printf("HAL Compiler:	%s\r\n", g_ui8HALcompiler);
	am_util_stdio_printf("HAL SDK version: %d.%d.%d\r\n",
						 g_ui32HALversion.s.Major,
						 g_ui32HALversion.s.Minor,
						 g_ui32HALversion.s.Revision);
	am_util_stdio_printf("HAL compiled with %s-style registers\r\n",
						 g_ui32HALversion.s.bAMREGS ? "AM_REG" : "CMSIS");

	am_hal_security_info_t security_info;
	uint32_t status = am_hal_security_get_info(&security_info);
	if (status == AM_HAL_STATUS_SUCCESS)
	{
		char string_buffer[32];
		if (security_info.bInfo0Valid)
		{
			am_util_stdio_sprintf(string_buffer, "INFO0 valid, ver 0x%X", security_info.info0Version);
		}
		else
		{
			am_util_stdio_sprintf(string_buffer, "INFO0 invalid");
		}

		am_util_stdio_printf("SBL ver: 0x%x - 0x%x, %s\r\n",
			security_info.sblVersion, security_info.sblVersionAddInfo, string_buffer);
	}
	else
	{
		am_util_stdio_printf("am_hal_security_get_info failed 0x%X\r\n", status);
	}

	struct gpio lora_power;
	gpio_init(&lora_power, 10, GPIO_MODE_OUTPUT, false);
	struct spi_bus spi_bus;
	spi_bus_init(&spi_bus, 0);
	struct spi_device device;
	spi_bus_init_device(&spi_bus, &device, SPI_CS_0, 4000000u);

	struct lora lora;
	//lora_receive_mode(&lora);

	// Wait here for the ISR to grab a buffer of samples.
	while (1)
	{
		// Trigger the ADC to start collecting data
		adc_trigger(&adc);

		// Print the battery voltage and temperature for each interrupt
		uint32_t data[3] = {0};
		if (adc_get_sample(&adc, data, pins, size))
		{
			// The math here is straight forward: we've asked the ADC to give
			// us data in 14 bits (max value of 2^14 -1). We also specified the
			// reference voltage to be 1.5V. A reading of 1.5V would be
			// translated to the maximum value of 2^14-1. So we divide the
			// value from the ADC by this maximum, and multiply it by the
			// reference, which then gives us the actual voltage measured.
			const double reference = 1.5;
			double voltage = data[0] * reference / ((1 << 14) - 1);

			double temperature = 5.506 - sqrt((-5.506)*(-5.506) + 4 * 0.00176 * (870.6 - voltage*1000));
			temperature /= (2 * -.00176);
			temperature += 30;

			gpio_set(&lora_power, true);
			// The documentation for the SX1276 states that it takes 10 ms for
			// the radio to come online from coldboot.
			am_util_delay_ms(10);
			// Only continue if we initialize
			// FIXME what if we never initialize?
			while(!lora_init(&lora, &device, 915000000, 42));
			lora_standby(&lora);
			lora_set_spreading_factor(&lora, 7);
			lora_set_coding_rate(&lora, 1);
			lora_set_bandwidth(&lora, 0x7);

			unsigned char buffer[64];
			int magnitude = 10000;
			snprintf((char*)buffer, sizeof(buffer),
				"{ \"temperature\": %i, \"magnitude\": %i }",
				(int)(temperature * magnitude),
				magnitude);
			lora_send_packet(&lora, buffer, strlen((char*)buffer));
			if (lora_rx_amount(&lora))
			{
				am_util_stdio_printf("length %i\r\n", lora_rx_amount(&lora));
				lora_receive_packet(&lora, buffer, 32);
				am_util_stdio_printf("Data: %s\r\n", buffer);
			}
			lora_destroy(&lora);
			gpio_set(&lora_power, false);
		}

		// Sleep here until the next ADC interrupt comes along.
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
	}
}
