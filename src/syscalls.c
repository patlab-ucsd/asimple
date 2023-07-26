// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <syscalls.h>
#include <syscalls_internal.h>

#include <sys/time.h>
#include <fcntl.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

static int syscalls_read(void *sys, int file, char *ptr, int len)
{
	struct syscalls_base *base = sys;
	return base->read(base, file, ptr, len);
}

static int syscalls_write(void *sys, int file, char *ptr, int len)
{
	struct syscalls_base *base = sys;
	return base->write(base, file, ptr, len);
}

static int syscalls_open(void *sys, const char *name, int flags, int mode)
{
	struct syscalls_base *base = sys;
	return base->open(base, name, flags, mode);
}

static int syscalls_lseek(void *sys, int file, int ptr, int dir)
{
	struct syscalls_base *base = sys;
	return base->lseek(base, file, ptr, dir);
}

static int syscalls_close(void *sys, int file)
{
	struct syscalls_base *base = sys;
	return base->close(base, file);
}

static int syscalls_gettimeofday(void *sys, struct timeval *ptimeval, void *ptimezone)
{
	struct syscalls_base *base = sys;
	return base->gettimeofday(base, ptimeval, ptimezone);
}

struct syscalls_devices
{
	struct syscalls_base *stdio[3];
	struct syscalls_base *rtc;
	struct syscalls_base *fs;
};

static struct syscalls_devices devices;
void syscalls_register_rtc(void *device)
{
	devices.rtc = (struct syscalls_base*)device;
}

void syscalls_register_stdin(void *device)
{
	devices.stdio[0] = (struct syscalls_base*)device;
}

void syscalls_register_stdout(void *device)
{
	devices.stdio[1] = (struct syscalls_base*)device;
}

void syscalls_register_stderr(void *device)
{
	devices.stdio[2] = (struct syscalls_base*)device;
}

void syscalls_register_fs(void *device)
{
	devices.fs = (struct syscalls_base*)device;
}

// Syscalls

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
	(void)ptimezone;

	// Return an error if rtc hasn't been initialized
	if (!devices.rtc)
	{
		errno = ENXIO;
		return -1;
	}
	int result = syscalls_gettimeofday(devices.rtc, ptimeval, ptimezone);
	if (result < 0)
	{
		// FIXME translate error
		errno = result;
		return -1;
	}

	return result;
}

int _open(const char *name, int flags, int mode)
{
	// FIXME check if mounted?
	if (devices.fs && strncmp(name, "fs:/", 4) == 0)
	{
		int result = syscalls_open(devices.fs, name + 3, flags, mode);
		if (result >= 0)
		{
			return result + 3;
		}
		return result;
	}
	errno = ENXIO;
	return -1;
}

__attribute__ ((used))
int _read(int file, char *ptr, int len)
{
	if (file < 3)
	{
		return syscalls_read(devices.stdio[file], file, ptr, len);
	}
	if (file >= 3)
	{
		return syscalls_read(devices.fs, file - 3, ptr, len);
	}
	errno = ENOSYS;
	return -1;
}

__attribute__ ((used))
int _write(int file, char *ptr, int len)
{
	// stdin, stdout, stderr
	if (file < 3)
	{
		return syscalls_write(devices.stdio[file], file, ptr, len);
	}
	else if (file >= 3)
	{
		return syscalls_write(devices.fs, file - 3, ptr, len);
	}
	errno = ENOSYS;
	return -1;
}

__attribute__ ((used))
int _lseek (int file, int ptr, int dir)
{
	if (file < 3)
	{
		return syscalls_lseek(devices.stdio[file], file, ptr, dir);;
	}
	else if (file >= 3)
	{
		return syscalls_lseek(devices.fs, file - 3, ptr, dir);
	}
	errno = ENOSYS;
	return -1;
}

__attribute__ ((used))
int _close (int file)
{
	if (file < 3)
	{
		errno = EINVAL;
		return -1;
	}
	else if (file >= 3)
	{
		return syscalls_close(devices.fs, file - 3);
	}
	errno = ENOSYS;
	return -1;
}
