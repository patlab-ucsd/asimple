// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <syscalls.h>
#include <syscalls_internal.h>

#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

static int syscalls_read(void *sys, int file, char *ptr, int len)
{
	struct syscalls_base *base = sys;
	if (!base || !base->read)
	{
		errno = EBADF;
		return -1;
	}
	return base->read(base, file, ptr, len);
}

static int syscalls_write(void *sys, int file, char *ptr, int len)
{
	struct syscalls_base *base = sys;
	if (!base || !base->write)
	{
		errno = EBADF;
		return -1;
	}
	return base->write(base, file, ptr, len);
}

static int syscalls_open(void *sys, const char *name, int flags, int mode)
{
	struct syscalls_base *base = sys;
	if (!base || !base->open)
	{
		errno = ENXIO;
		return -1;
	}
	return base->open(base, name, flags, mode);
}

static int syscalls_lseek(void *sys, int file, int ptr, int dir)
{
	struct syscalls_base *base = sys;
	if (!base || !base->lseek)
	{
		errno = EBADF;
		return -1;
	}
	return base->lseek(base, file, ptr, dir);
}

static int syscalls_close(void *sys, int file)
{
	struct syscalls_base *base = sys;
	if (!base || !base->close)
	{
		errno = EBADF;
		return -1;
	}
	return base->close(base, file);
}

static int syscalls_gettimeofday(void *sys, struct timeval *ptimeval, void *ptimezone)
{
	struct syscalls_base *base = sys;
	if (!base || !base->gettimeofday)
	{
		errno = EFAULT;
		return -1;
	}
	return base->gettimeofday(base, ptimeval, ptimezone);
}

static int syscalls_fstat(void *sys, int fd, struct stat *st)
{
	struct syscalls_base *base = sys;
	if (!base || !base->fstat)
	{
		errno = EBADF;
		return -1;
	}
	return base->fstat(base, fd, st);
}

static int syscalls_stat(void *sys, char *path, struct stat *st)
{
	struct syscalls_base *base = sys;
	if (!base || !base->stat)
	{
		errno = ENOENT;
		return -1;
	}
	return base->stat(base, path, st);
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

__attribute__((used))
int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	errno = ENOSYS;
	return -1;
}

__attribute((used))
int _getpid(void)
{
	return 1;
}

__attribute__((used))
int _isatty(int file)
{
	(void)file;
	if (file < 3)
		return 1;
	// FIXME what constitutes a tty???
	errno = ENOTTY;
	return 0;
}

__attribute__((used))
int _fstat(int file, struct stat *st)
{
	if (file < 3)
	{
		return syscalls_fstat(devices.stdio[file], file, st);
	}
	else
	{
		return syscalls_fstat(devices.fs, file - 3, st);
	}
}

__attribute__((used))
int _stat(char *filename, struct stat *st)
{
	if (devices.fs && strncmp(filename, "fs:/", 4) == 0)
	{
		return syscalls_stat(devices.fs, filename + 3, st);
	}
	errno = ENOENT;
	return -1;
}
