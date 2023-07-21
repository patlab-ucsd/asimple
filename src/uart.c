// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>

#include <uart.h>

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

static struct uart* isr_uart_handle;

size_t uart_write(struct uart *uart, const unsigned char *data, size_t size)
{
	uint32_t written = 0;
	const am_hal_uart_transfer_t config =
	{
		.ui32Direction = AM_HAL_UART_WRITE,
		.pui8Data = (uint8_t *)data,
		.ui32NumBytes = size,
		.ui32TimeoutMs = 0,
		.pui32BytesTransferred = &written,
	};

	CHECK_ERRORS(am_hal_uart_transfer(uart->handle, &config));
	return written;
}

static void am_uart_write(char *string)
{
	size_t left = strlen(string);
	do
	{
		size_t written = uart_write(isr_uart_handle, ((uint8_t*)string), left);
		string += written;
		left -= written;
	} while (left);
}

void uart_init(struct uart *uart, enum uart_instance instance)
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
	static_assert(sizeof(uart->tx_buffer) == 2560, "unexpected tx_buffer size");

	uart->instance = (int)instance;
	CHECK_ERRORS(am_hal_uart_initialize((int)instance, &uart->handle));
	CHECK_ERRORS(am_hal_uart_power_control(uart->handle, AM_HAL_SYSCTRL_WAKE, false));
	CHECK_ERRORS(am_hal_uart_configure(uart->handle, &config));

	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);


	isr_uart_handle = uart;
	am_util_stdio_printf_init(am_uart_write);

	NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + (int)instance));
}

void uart_set_baud_rate(struct uart *uart, unsigned int baud_rate)
{
	const am_hal_uart_config_t config =
	{
		// Standard UART settings: 115200-8-N-1
		.ui32BaudRate = baud_rate,
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
	CHECK_ERRORS(am_hal_uart_configure(uart->handle, &config));
}

void uart_destroy(struct uart *uart)
{
	NVIC_DisableIRQ((IRQn_Type)(UART0_IRQn + uart->instance));
	am_util_stdio_printf_init(NULL);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_HAL_GPIO_DISABLE);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_HAL_GPIO_DISABLE);
	am_hal_uart_power_control(uart->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
	am_hal_uart_deinitialize(uart->handle);
	memset(uart, 0, sizeof(*uart));
}

// This is a weak symbol, override
void am_uart_isr(void)
{
	uint32_t status;
	am_hal_uart_interrupt_status_get(isr_uart_handle->handle, &status, true);
	am_hal_uart_interrupt_clear(isr_uart_handle->handle, status);

	uint32_t idle;
	am_hal_uart_interrupt_service(isr_uart_handle->handle, status, &idle);
}
