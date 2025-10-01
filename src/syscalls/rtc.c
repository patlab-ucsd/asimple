// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <am1815.h>
#include <syscalls_internal.h>

#include <fcntl.h>
#include <sys/time.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

struct syscalls_rtc
{
	struct syscalls_base base;
	struct am1815 *rtc;
};

int syscalls_rtc_gettimeofday(
	void *context, struct timeval *ptimeval, void *ptimezone
)
{
	(void)ptimezone;
	struct syscalls_rtc *rtc = (struct syscalls_rtc *)context;

	// Return an error if rtc hasn't been initialized
	if (!rtc->rtc)
	{
		errno = ENXIO;
		return -1;
	}

	*ptimeval = am1815_read_time(rtc->rtc);
	return 0;
}

static struct syscalls_rtc rtc = {
	.base = {
		.open = NULL,
		.close = NULL,
		.read = NULL,
		.write = NULL, // FIXME I think I can come up with something here...
		.lseek = NULL,
		.gettimeofday = syscalls_rtc_gettimeofday,
	}
};

void syscalls_rtc_init(struct am1815 *rtc_)
{
	rtc.rtc = rtc_;
	syscalls_register_rtc(&rtc);
}
