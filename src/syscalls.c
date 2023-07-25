// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <am1815.h>
#include <uart.h>
#include <asimple_littlefs.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <lfs.h>

#include <sys/time.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

struct syscalls_base
{
	int (*open)(void *context, const char *name, int flags, int mode);
	int (*read)(void *context, int file, char *ptr, int len);
	int (*write)(void *context, int file, char *ptr, int len);
};

int syscalls_read(void *sys, int file, char *ptr, int len)
{
	struct syscalls_base *base = sys;
	return base->read(base, file, ptr, len);
}

int syscalls_write(void *sys, int file, char *ptr, int len)
{
	struct syscalls_base *base = sys;
	return base->write(base, file, ptr, len);
}

int syscalls_open(void *sys, const char *name, int flags, int mode)
{
	struct syscalls_base *base = sys;
	return base->open(base, name, flags, mode);
}

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
	return uart_write(uart->uart, (unsigned char *)ptr, len);
}

static struct syscalls_uart uart = {
	.base = {
		.open = NULL,
		.read = NULL, // FIXME
		.write = uart_write_,
	},
};

void syscalls_uart_init(struct uart *uart_)
{
	uart.uart = uart_;
}

struct syscalls_rtc
{
	struct syscalls_base base;
	struct am1815 *rtc;
};

static struct syscalls_rtc rtc = {
	.base = {
		.open = NULL,
		.read = NULL,
		.write = NULL, // FIXME I think I can come up with something here...
	}
};

void syscalls_rtc_init(struct am1815 *rtc_)
{
	rtc.rtc = rtc_;
}

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

struct syscalls_littlefs
{
	struct syscalls_base base;
	struct asimple_littlefs *fs;
	struct lfs_file files[10];
	struct lfs_file *active_files[10];
};

int littlefs_open_(void *context, const char *name, int flags, int mode)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	size_t max = ARRAY_SIZE(fs->files);
	size_t i;
	for (i = 0; i < max; ++i)
	{
		if (fs->active_files[i] == NULL)
			break;
	}

	if (i >= max)
	{
		errno = ENFILE;
		return -1;
	}

	// FIXME convert flags and mode

	int result = lfs_file_open(&fs->fs->lfs, &fs->files[i], name, flags);
	if (result < 0)
	{
		//FIXME convert into errno
		errno = result;
		return -1;
	}

	fs->active_files[i] = &fs->files[i];
	return i;
}

int littlefs_read_(void *context, int file, char *ptr, int len)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	int result = lfs_file_read(&fs->fs->lfs, fs->active_files[file], ptr, len);
	if (result < 0)
	{
		//FIXME convert into errno
		errno = result;
		return -1;
	}
	return result;
}

int littlefs_write_(void *context, int file, char *ptr, int len)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	int result = lfs_file_write(&fs->fs->lfs, fs->active_files[file], ptr, len);
	if (result < 0)
	{
		//FIXME convert into errno
		errno = result;
		return -1;
	}
	return result;
}

static struct syscalls_littlefs fs = {
	.base = {
		.open = littlefs_open_,
		.read = littlefs_read_,
		.write = littlefs_write_,
	},
};

void syscalls_littlefs_init(struct asimple_littlefs *fs_)
{
	fs.fs = fs_;
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
	(void)ptimezone;

	// Return an error if rtc hasn't been initialized
	if (!rtc.rtc)
	{
		errno = ENXIO;
		return -1;
	}

	*ptimeval = am1815_read_time(rtc.rtc);
	return 0;
}

int _open(const char *name, int flags, int mode)
{
	// FIXME check if mounted?
	if (fs.fs && strcmp(name, "fs:") == 0)
	{
		int result = syscalls_open(&fs, name + 3, flags, mode);
		if (result >= 0)
		{
			return result + 3;
		}
		return result;
	}
	errno = ENXIO;
	return -1;
}

int _read(int file, char *ptr, int len)
{
	if (file >= 3)
	{
		return syscalls_read(&fs, file - 3, ptr, len);
	}
	errno = ENOSYS;
	return -1;
}

int _write(int file, char *ptr, int len)
{
	// stdin, stdout, stderr
	if (file < 3)
	{
		return syscalls_write(&uart, file, ptr, len);
	}
	else if (file > 3)
	{
		return syscalls_write(&fs, file - 3, ptr, len);
	}
	errno = ENOSYS;
	return -1;
}
