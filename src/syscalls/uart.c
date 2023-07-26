// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <uart.h>
#include <syscalls_internal.h>

#include <fcntl.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

struct syscalls_uart
{
	struct syscalls_base base;
	struct uart *uart;
};

int uart_write_(void *context, int file, char *ptr, int len)
{
	(void)file;
	struct syscalls_uart *uart = (struct syscalls_uart*)context;
	if (!uart && !uart->uart)
	{
		errno = ENXIO;
		return -1;
	}
	int result = uart_write(uart->uart, (unsigned char *)ptr, len);
	uart_sync(uart->uart);
	return result;
}

static struct syscalls_uart uart = {
	.base = {
		.open = NULL,
		.close = NULL,
		.read = NULL, // FIXME
		.write = uart_write_,
		.lseek = NULL,
	},
};

void syscalls_uart_init(struct uart *uart_)
{
	uart.uart = uart_;
	syscalls_register_stdin(&uart);
	syscalls_register_stdout(&uart);
	syscalls_register_stderr(&uart);
}
