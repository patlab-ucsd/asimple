// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <am1815.h>
#include <uart.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <sys/time.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

static struct am1815 *rtc;
static struct uart *uart;

void initialize_time(struct am1815 *rtc_)
{
	rtc = rtc_;
}

void initialize_sys_uart(struct uart *uart_)
{
	uart = uart_;
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
	(void)ptimezone;

	// Return an error if rtc hasn't been initialized
	if (!rtc)
	{
		errno = ENXIO;
		return -1;
	}

	*ptimeval = am1815_read_time(rtc);
	return 0;
}

struct file_drivers
{
	int (*open)(const char *name, int flags, int mode);
	int (*read)(int file, char *ptr, int len);
	int (*write)(int file, char *ptr, int len);
};

int uart_write_(int file, char *ptr, int len)
{
	(void)file;
	if (!uart)
	{
		errno = ENXIO;
		return -1;
	}
	return uart_write(uart, (unsigned char *)ptr, len);
}

struct file_drivers uart_ =
{
	NULL, NULL, uart_write_
};

// 0 - stdin
// 1 - stdout
// 2 - stderr
static struct file_drivers *files[10] = {
	&uart_, &uart_, &uart_,
};

int _open(const char *name, int flags, int mode)
{
	errno = ENOSYS;
	return -1;
}

int _read(int file, char *ptr, int len)
{
	if(!files[file])
	{
		errno = ENOSYS;
		return -1;
	}
	if ((unsigned)file > sizeof(files)/sizeof(files[0]))
	{
		errno = ENOENT;
		return -1;
	}
	if (!files[file]->read)
	{
		errno = ENOSYS;
		return -1;
	}
	return files[file]->read(file, ptr, len);
}

int _write(int file, char *ptr, int len)
{
	if(!files[file])
	{
		errno = ENOSYS;
		return -1;
	}
	if ((unsigned)file > sizeof(files)/sizeof(files[0]))
	{
		errno = ENOENT;
		return -1;
	}
	if (!files[file]->write)
	{
		errno = ENOSYS;
		return -1;
	}
	return files[file]->write(file, ptr, len);
}
