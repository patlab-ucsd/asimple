// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include "am_bsp.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

#include <uart.h>

#include <assert.h>
#include <stdatomic.h>
#include <string.h>

/** UART structure. */
struct uart
{
	void *handle;
	int instance;
	// FIXME do we want the buffer size to be fixed?
	uint8_t tx_buffer[1024];
	// FIXME does RX work?
	uint8_t rx_buffer[1024];
	atomic_uint refcount;
};

#define CHECK_ERRORS(x)                                                        \
	if ((x) != AM_HAL_STATUS_SUCCESS)                                          \
	{                                                                          \
		error_handler(x);                                                      \
	}

static void error_handler(uint32_t error)
{
	(void)error;
	for (;;)
	{
		am_devices_led_on(am_bsp_psLEDs, 0);
		am_util_delay_ms(500);
		am_devices_led_off(am_bsp_psLEDs, 0);
		am_util_delay_ms(500);
	}
}

static struct uart uarts[2];

struct uart *uart_get_instance(enum uart_instance instance)
{
	struct uart *uart = uarts + (int)instance;
	if (!uart->handle)
	{
		const am_hal_uart_config_t config = {
			// Standard UART settings: 115200-8-N-1
			.ui32BaudRate = 115200,
			.ui32DataBits = AM_HAL_UART_DATA_BITS_8,
			.ui32Parity = AM_HAL_UART_PARITY_NONE,
			.ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
			.ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

			// Set TX and RX FIFOs to interrupt at half-full.
			.ui32FifoLevels =
				(AM_HAL_UART_TX_FIFO_1_2 | AM_HAL_UART_RX_FIFO_1_2),

			// Buffers
			.pui8TxBuffer = uart->tx_buffer,
			.ui32TxBufferSize = sizeof(uart->tx_buffer),
			.pui8RxBuffer = uart->rx_buffer,
			.ui32RxBufferSize = sizeof(uart->rx_buffer),
		};
		static_assert(
			sizeof(uart->tx_buffer) == 1024, "unexpected tx_buffer size"
		);
		static_assert(
			sizeof(uart->rx_buffer) == 1024, "unexpected rx_buffer size"
		);

		uart->instance = (int)instance;
		CHECK_ERRORS(am_hal_uart_initialize((int)instance, &uart->handle));
		CHECK_ERRORS(
			am_hal_uart_power_control(uart->handle, AM_HAL_SYSCTRL_WAKE, false)
		);
		CHECK_ERRORS(am_hal_uart_configure(uart->handle, &config));

		uart_sleep(uart);
	}
	uart->refcount++;
	return uart;
}

bool uart_sleep(struct uart *uart)
{
	// Note that turning off the hardware resets registers, which is why we
	// request saving the state
	// Also, spinloop while the device is busy
	// Implementation based off spi.c
	int status;
	do
	{
		status = am_hal_uart_power_control(
			uart->handle, AM_HAL_SYSCTRL_DEEPSLEEP, true
		);
	}
	while (status == AM_HAL_STATUS_IN_USE);

	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_HAL_GPIO_DISABLE);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_HAL_GPIO_DISABLE);
	NVIC_DisableIRQ((IRQn_Type)(UART0_IRQn + uart->instance));
	return true;
}

bool uart_enable(struct uart *uart)
{
	// This can fail if there is no saved state, which indicates we've never
	// gone asleep
	int status =
		am_hal_uart_power_control(uart->handle, AM_HAL_SYSCTRL_WAKE, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}

	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);
	NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + uart->instance));
	return true;
}

void uart_deinitialize(struct uart *uart)
{
	if (uart->refcount)
	{
		if (!--(uart->refcount))
		{
			NVIC_DisableIRQ((IRQn_Type)(UART0_IRQn + uart->instance));
			am_hal_gpio_pinconfig(
				AM_BSP_GPIO_COM_UART_RX, g_AM_HAL_GPIO_DISABLE
			);
			am_hal_gpio_pinconfig(
				AM_BSP_GPIO_COM_UART_TX, g_AM_HAL_GPIO_DISABLE
			);
			am_hal_uart_power_control(
				uart->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false
			);
			am_hal_uart_deinitialize(uart->handle);
			memset(uart, 0, sizeof(*uart));
		}
	}
}

size_t uart_write(struct uart *uart, const unsigned char *data, size_t size)
{
	uint32_t written = 0;
	const am_hal_uart_transfer_t config = {
		.ui32Direction = AM_HAL_UART_WRITE,
		.pui8Data = (uint8_t *)data,
		.ui32NumBytes = size,
		.ui32TimeoutMs = 0,
		.pui32BytesTransferred = &written,
	};

	CHECK_ERRORS(am_hal_uart_transfer(uart->handle, &config));
	return written;
}

size_t uart_read(struct uart *uart, unsigned char *data, size_t size)
{
	uint32_t read = 0;
	const am_hal_uart_transfer_t config = {
		.ui32Direction = AM_HAL_UART_READ,
		.pui8Data = (uint8_t *)data,
		.ui32NumBytes = size,
		.ui32TimeoutMs = 0,
		.pui32BytesTransferred = &read,
	};

	CHECK_ERRORS(am_hal_uart_transfer(uart->handle, &config));
	return read;
}

void uart_set_baud_rate(struct uart *uart, unsigned int baud_rate)
{
	const am_hal_uart_config_t config = {
		// Standard UART settings: 115200-8-N-1
		.ui32BaudRate = baud_rate,
		.ui32DataBits = AM_HAL_UART_DATA_BITS_8,
		.ui32Parity = AM_HAL_UART_PARITY_NONE,
		.ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
		.ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

		// Set TX and RX FIFOs to interrupt at half-full.
		.ui32FifoLevels = (AM_HAL_UART_TX_FIFO_1_2 | AM_HAL_UART_RX_FIFO_1_2),

		// Buffers
		.pui8TxBuffer = uart->tx_buffer,
		.ui32TxBufferSize = sizeof(uart->tx_buffer),
		.pui8RxBuffer = uart->rx_buffer,
		.ui32RxBufferSize = sizeof(uart->rx_buffer),
	};
	CHECK_ERRORS(am_hal_uart_configure(uart->handle, &config));
}

void uart_sync(struct uart *uart)
{
	am_hal_uart_tx_flush(uart->handle);
}

static void uart_isr_handler(struct uart *uart)
{
	uint32_t status;
	am_hal_uart_interrupt_status_get(uart->handle, &status, true);
	am_hal_uart_interrupt_clear(uart->handle, status);

	uint32_t idle;
	am_hal_uart_interrupt_service(uart->handle, status, &idle);
}

// This is a weak symbol, override
void am_uart_isr(void)
{
	uart_isr_handler(uarts + 0);
}

void am_uart1_isr(void)
{
	uart_isr_handler(uarts + 1);
}
