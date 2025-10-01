// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <syscalls_internal.h>
#include <uart.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

struct syscalls_uart
{
	struct syscalls_base base;
	struct uart *uart;
};

static int uart_write_(void *context, int file, char *ptr, int len)
{
	(void)file;
	struct syscalls_uart *uart = (struct syscalls_uart *)context;
	if (!uart && !uart->uart)
	{
		errno = ENXIO;
		return -1;
	}
	int result = uart_write(uart->uart, (unsigned char *)ptr, len);
	uart_sync(uart->uart);
	return result;
}

static int uart_read_(void *context, int file, char *ptr, int len)
{
	if (len < 0)
	{
		errno = EINVAL;
		return -1;
	}

	(void)file;
	struct syscalls_uart *uart = (struct syscalls_uart *)context;
	if (!uart && !uart->uart)
	{
		errno = ENXIO;
		return -1;
	}
	int read = 0;
	// while(read < len)
	while (!read)
	{
		read += uart_read(uart->uart, (unsigned char *)ptr, len - read);
		// FIXME sleep if interrupts are enabled?
	}
	return read;
}

static int uart_fstat_(void *context, int file, struct stat *stat)
{
	(void)file;
	struct syscalls_uart *uart = (struct syscalls_uart *)context;
	if (!uart && !uart->uart)
	{
		errno = EBADF;
		return -1;
	}
	struct stat result = {
		.st_dev = 1, // FIXME some enum somewhere?
		.st_ino = 1, // FIXME I don't even know what this means for a special
					 // file in no particular filesystem
		.st_mode = S_IFCHR | S_IRWXU | S_IRWXG | S_IRWXO,
		.st_nlink = 1,
		.st_uid = 0,
		.st_gid = 0,
		.st_rdev = 1, // FIXME
		.st_size = 0,
		.st_blksize = 32, // FIXME confirm with datasheet what the FIFO depth is
		.st_blocks = 1024 / 32, // FIXME base this on the actual TX/RX size
		.st_atim = {0},         // FIXME
		.st_mtim = {0},         // FIXME
		.st_ctim = {0},         // FIXME
	};
	*stat = result;
	return 0;
}

static struct syscalls_uart uart = {
	.base = {
		.open = NULL,
		.close = NULL,
		.read = uart_read_,
		.write = uart_write_,
		.lseek = NULL,
		.fstat = uart_fstat_,
	},
};

void syscalls_uart_init(struct uart *uart_)
{
	uart.uart = uart_;
	syscalls_register_stdin(&uart);
	syscalls_register_stdout(&uart);
	syscalls_register_stderr(&uart);
}
