// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>

struct uart
{
	void *handle;
	uint8_t tx_buffer[256];
	uint8_t rx_buffer[2];
};

enum uart_instance
{
	UART_INST0 = 0,
	UART_INST1 = 1
};

#define CHECK_ERRORS(x)               \
	if ((x) != AM_HAL_STATUS_SUCCESS) \
	{                                 \
		error_handler(x);             \
	}

static void error_handler(uint32_t error)
{
	(void)error;
	for(;;)
	{
		am_devices_led_on(am_bsp_psLEDs, 0);
		am_util_delay_ms(500);
		am_devices_led_off(am_bsp_psLEDs, 0);
		am_util_delay_ms(500);
	}
}

static void* isr_uart_handle;

static void uart_print(char *string)
{
	uint32_t len = strlen(string);
	uint32_t written = 0;
	const am_hal_uart_transfer_t config =
	{
		.ui32Direction = AM_HAL_UART_WRITE,
		.pui8Data = (uint8_t *) string,
		.ui32NumBytes = len,
		.ui32TimeoutMs = 0,
		.pui32BytesTransferred = &written,
	};

	CHECK_ERRORS(am_hal_uart_transfer(isr_uart_handle, &config));
	CHECK_ERRORS(written != len);
}

static void uart_init(struct uart *uart, enum uart_instance instance)
{
	const am_hal_uart_config_t config =
	{
		// Standard UART settings: 115200-8-N-1
		.ui32BaudRate = 115200,
		.ui32DataBits = AM_HAL_UART_DATA_BITS_8,
		.ui32Parity = AM_HAL_UART_PARITY_NONE,
		.ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
		.ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

		// Set TX and RX FIFOs to interrupt at half-full.
		.ui32FifoLevels = (AM_HAL_UART_TX_FIFO_1_2 |
						   AM_HAL_UART_RX_FIFO_1_2),

		// Buffers
		.pui8TxBuffer = uart->tx_buffer,
		.ui32TxBufferSize = sizeof(uart->tx_buffer),
		.pui8RxBuffer = uart->rx_buffer,
		.ui32RxBufferSize = sizeof(uart->rx_buffer),
	};
	static_assert(sizeof(uart->tx_buffer) == 256, "unexpected tx_buffer size");

	CHECK_ERRORS(am_hal_uart_initialize((int)instance, &uart->handle));
	CHECK_ERRORS(am_hal_uart_power_control(uart->handle, AM_HAL_SYSCTRL_WAKE, false));
	CHECK_ERRORS(am_hal_uart_configure(uart->handle, &config));

	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);


	isr_uart_handle = uart->handle;
	am_util_stdio_printf_init(uart_print);

	NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + (int)instance));
}

// This is a weak symbol, override
void am_uart_isr(void)
{
	uint32_t status;
	am_hal_uart_interrupt_status_get(isr_uart_handle, &status, true);
	am_hal_uart_interrupt_clear(isr_uart_handle, status);

	uint32_t idle;
	am_hal_uart_interrupt_service(isr_uart_handle, status, &idle);
}

static struct uart uart;

int main(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();

	// Init UART
	uart_init(&uart, UART_INST0);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();

	// Print the banner.
	am_util_stdio_terminal_clear();
	am_util_stdio_printf("Hello World!\n\n");

	// Print the device info.
	am_util_id_t device_id;
	am_util_id_device(&device_id);
	am_util_stdio_printf("Vendor Name: %s\n", device_id.pui8VendorName);
	am_util_stdio_printf("Device type: %s\n", device_id.pui8DeviceName);

	am_util_stdio_printf("Qualified: %s\n",
						 device_id.sMcuCtrlDevice.ui32Qualified ?
						 "Yes" : "No");

	am_util_stdio_printf("Device Info:\n"
						 "\tPart number: 0x%08X\n"
						 "\tChip ID0:	0x%08X\n"
						 "\tChip ID1:	0x%08X\n"
						 "\tRevision:	0x%08X (Rev%c%c)\n",
						 device_id.sMcuCtrlDevice.ui32ChipPN,
						 device_id.sMcuCtrlDevice.ui32ChipID0,
						 device_id.sMcuCtrlDevice.ui32ChipID1,
						 device_id.sMcuCtrlDevice.ui32ChipRev,
						 device_id.ui8ChipRevMaj, device_id.ui8ChipRevMin );

	// If not a multiple of 1024 bytes, append a plus sign to the KB.
	uint32_t mem_size = ( device_id.sMcuCtrlDevice.ui32FlashSize % 1024 ) ? '+' : 0;
	am_util_stdio_printf("\tFlash size:  %7d (%d KB%s)\n",
						 device_id.sMcuCtrlDevice.ui32FlashSize,
						 device_id.sMcuCtrlDevice.ui32FlashSize / 1024,
						 &mem_size);

	mem_size = ( device_id.sMcuCtrlDevice.ui32SRAMSize % 1024 ) ? '+' : 0;
	am_util_stdio_printf("\tSRAM size:   %7d (%d KB%s)\n\n",
						 device_id.sMcuCtrlDevice.ui32SRAMSize,
						 device_id.sMcuCtrlDevice.ui32SRAMSize / 1024,
						 &mem_size);

	// Print the compiler version.
	am_hal_uart_tx_flush(uart.handle);
	am_util_stdio_printf("App Compiler:	%s\n", COMPILER_VERSION);
	am_util_stdio_printf("HAL Compiler:	%s\n", g_ui8HALcompiler);
	am_util_stdio_printf("HAL SDK version: %d.%d.%d\n",
						 g_ui32HALversion.s.Major,
						 g_ui32HALversion.s.Minor,
						 g_ui32HALversion.s.Revision);
	am_util_stdio_printf("HAL compiled with %s-style registers\n",
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

		am_util_stdio_printf("SBL ver: 0x%x - 0x%x, %s\n",
			security_info.sblVersion, security_info.sblVersionAddInfo, string_buffer);
	}
	else
	{
		am_util_stdio_printf("am_hal_security_get_info failed 0x%X\n", status);
	}

	// Go to sleep...
	am_hal_uart_tx_flush(uart.handle);
	CHECK_ERRORS(am_hal_uart_power_control(uart.handle, AM_HAL_SYSCTRL_DEEPSLEEP, false));
	while (1)
	{
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
	}
}
