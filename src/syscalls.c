// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <am1815.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <sys/time.h>

#include <stdint.h>

static struct am1815 *rtc;

void initialize_time(struct am1815 *rtc_)
{
	rtc = rtc_;
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
	(void)ptimezone;

	// Return an error if rtc hasn't been initialized
	// FIXME is this the right error? Should we set errno?
	if (!rtc)
		return -1;

	*ptimeval = am1815_read_time(rtc);
	return 0;
}
