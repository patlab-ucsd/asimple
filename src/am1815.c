// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <am1815.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

uint8_t am1815_read_register(struct am1815 *rtc, uint8_t addr)
{
	uint8_t buffer;
	spi_device_cmd_read(rtc->spi, addr, &buffer, 1);
	return buffer;
}

void am1815_write_register(struct am1815 *rtc, uint8_t addr, uint8_t data)
{
	spi_device_cmd_write(rtc->spi, 0x80 | addr, &data, 1);
}

static uint8_t from_bcd(uint8_t bcd)
{
	return (bcd & 0xF) + ((bcd >> 4) * 10);
}

static uint8_t to_bcd(uint8_t value)
{
	uint8_t result = 0;
	uint8_t decade = 0;
	while (value != 0)
	{
		uint8_t digit = value % 10;
		value = value / 10;
		result |= digit << (4 * decade);
		decade += 1;
	}
	return result;
}

struct timeval am1815_read_time(struct am1815 *rtc)
{
	uint8_t data[8];
	spi_device_cmd_read(rtc->spi, 0x0, data, 8);

	struct tm date = {
		.tm_year = from_bcd(data[6]) + 100,
		.tm_mon = from_bcd(data[5] & 0x1F) - 1,
		.tm_mday = from_bcd(data[4] & 0x3F),
		.tm_hour = from_bcd(data[3] & 0x3F),
		.tm_min = from_bcd(data[2] & 0x7F),
		.tm_sec = from_bcd(data[1] & 0x7F),
		.tm_wday = from_bcd(data[7] & 0x07),
	};

	time_t time = mktime(&date);

	struct timeval result = {
		.tv_sec = time,
		.tv_usec = from_bcd(data[0]) * 10000,
	};
	return result;
}

void am1815_read_bulk(
	struct am1815 *rtc, uint8_t addr, uint8_t *data, size_t size
)
{
	spi_device_cmd_read(rtc->spi, addr, data, size);
}

void am1815_write_bulk(
	struct am1815 *rtc, uint8_t addr, const uint8_t *data, size_t size
)
{
	spi_device_cmd_write(rtc->spi, 0x80 | addr, data, size);
}

struct timeval am1815_read_alarm(struct am1815 *rtc)
{
	uint8_t data[7];
	spi_device_cmd_read(rtc->spi, 0x8, data, 7);

	struct tm date = {
		.tm_year = 0,
		.tm_mon = from_bcd(data[5] & 0x1F) - 1,
		.tm_mday = from_bcd(data[4] & 0x3F),
		.tm_hour = from_bcd(data[3] & 0x3F),
		.tm_min = from_bcd(data[2] & 0x7F),
		.tm_sec = from_bcd(data[1] & 0x7F),
		.tm_wday = from_bcd(data[7] & 0x07),
	};

	time_t time = mktime(&date);

	struct timeval result = {
		.tv_sec = time,
		.tv_usec = from_bcd(data[0]) * 10000,
	};
	return result;
}

void am1815_write_alarm(struct am1815 *rtc, struct timeval *atime)
{
	struct tm date;
	gmtime_r(&(atime->tv_sec), &date);
	uint8_t hundredths = atime->tv_usec / 10000;

	uint8_t regs[7];
	am1815_read_bulk(rtc, 0x08, regs, 7);
	regs[0] = to_bcd(hundredths);
	regs[1] &= 0x80;
	regs[2] &= 0x80;
	regs[3] &= 0xC0;
	regs[4] &= 0xC0;
	regs[5] &= 0xE0;
	regs[6] &= 0xF8;

	regs[1] |= to_bcd((uint8_t)date.tm_sec);
	regs[2] |= to_bcd((uint8_t)date.tm_min);
	regs[3] |= to_bcd((uint8_t)date.tm_hour);
	regs[4] |= to_bcd((uint8_t)date.tm_mday);
	regs[5] |= to_bcd((uint8_t)date.tm_mon + 1);
	regs[6] |= to_bcd((uint8_t)date.tm_wday);

	am1815_write_bulk(rtc, 0x08, regs, 7);
}

// Set RPT bits in Countdown Timer Control register to control how often the
// alarm interrupt repeats.
bool am1815_repeat_alarm(struct am1815 *rtc, int repeat)
{
	if (repeat < 0 || repeat > 7)
	{
		return false;
	}
	uint8_t timerControl = am1815_read_register(rtc, 0x18);
	timerControl = timerControl & ~(0b00011100);
	uint8_t timerMask = repeat << 2;
	uint8_t timerResult = timerControl | timerMask;
	am1815_write_register(rtc, 0x18, timerResult);
	return true;
}

void am1815_enable_trickle(struct am1815 *rtc)
{
	am1815_write_register(rtc, 0x1F, 0x9D);
	am1815_write_register(rtc, 0x20, 0xA5);
}

void am1815_disable_trickle(struct am1815 *rtc)
{
	am1815_write_register(rtc, 0x1F, 0x9D);
	am1815_write_register(rtc, 0x20, 0x00);
}

void am1815_init(struct am1815 *rtc, struct spi_device *device)
{
	rtc->spi = device;
	// am1815_enable_trickle(rtc);
}

uint8_t am1815_read_timer(struct am1815 *rtc)
{
	uint8_t buffer;
	spi_device_cmd_read(rtc->spi, 0x19, &buffer, 1);
	return (uint8_t)buffer;
	;
}

static double find_timer(double timer)
{
	if (timer <= 0.0625)
	{
		timer = ((int)(timer * 4096)) / 4096.0;
	}
	else if (timer <= 4)
	{
		timer = ((int)(timer * 64)) / 64.0;
	}
	else if (timer <= 256)
	{
		timer = (int)timer;
	}
	else if (timer <= 15360)
	{
		timer = ((int)(timer / 60)) * 60;
	}
	else
	{
		timer = 15360;
	}
	return timer;
}

double am1815_write_timer(struct am1815 *rtc, double timer)
{
	double finalTimer = find_timer(timer);

	if (finalTimer == 0)
	{
		return 0;
	}

	// TE (enables countdown timer)
	// Sets the Countdown Timer Frequency and the Timer Initial Value
	uint8_t countdowntimer = am1815_read_register(rtc, 0x18);
	// clear TE first
	am1815_write_register(rtc, 0x18, countdowntimer & ~0b10000000);
	uint8_t RPT = countdowntimer & 0b00011100;
	uint8_t timerResult = 0b10100000 + RPT;
	uint32_t timerinitial = 0;
	if (finalTimer <= 0.0625)
	{
		timerResult += 0b00;
		timerinitial = ((int)(finalTimer * 4096)) - 1;
	}
	else if (finalTimer <= 4)
	{
		timerResult += 0b01;
		timerinitial = ((int)(finalTimer * 64)) - 1;
	}
	else if (finalTimer <= 256)
	{
		timerResult += 0b10;
		timerinitial = ((int)timer) - 1;
	}
	else
	{
		timerResult += 0b11;
		timerinitial = ((int)(finalTimer * (1 / 60))) - 1;
	}

	am1815_write_register(rtc, 0x19, timerinitial);
	am1815_write_register(rtc, 0x1A, timerinitial);
	am1815_write_register(rtc, 0x18, timerResult);

	return finalTimer;
}

void am1815_enable_alarm_interrupt(
	struct am1815 *rtc, enum am1815_pulse_width pulse
)
{
	// Configure AIRQ (alarm) interrupt
	// IM (level/pulse) AIE (enables interrupt) 0x12 intmask
	uint8_t alarm = am1815_read_register(rtc, 0x12);
	alarm = alarm & ~(0b01100100);

	uint8_t alarmMask = ((uint8_t)pulse) << 5;

	// enables the alarm
	alarmMask |= 0b00000100;

	uint8_t alarmResult = alarm | alarmMask;
	am1815_write_register(rtc, 0x12, alarmResult);

	// Set Control2 register bits so that FOUT/nIRQ pin outputs nAIRQ
	uint8_t out = am1815_read_register(rtc, 0x11);
	uint8_t outMask = 0b00000011;
	uint8_t outResult = out | outMask;
	am1815_write_register(rtc, 0x11, outResult);
}

void am1815_disable_alarm_interrupt(struct am1815 *rtc)
{
	// Configure AIRQ (alarm) interrupt
	// IM (level/pulse) AIE (enables interrupt) 0x12 intmask
	uint8_t alarm = am1815_read_register(rtc, 0x12);
	alarm = alarm & ~(0b00000100);
	am1815_write_register(rtc, 0x12, alarm);
}

void am1815_write_time(struct am1815 *rtc, const struct timeval *time)
{
	struct tm date;
	gmtime_r(&(time->tv_sec), &date);
	uint8_t hundredths = time->tv_usec / 10000;

	uint8_t regs[8];
	am1815_read_bulk(rtc, 0x0, regs, 8);

	regs[0] = to_bcd(hundredths);
	regs[1] &= 0x80;
	regs[2] &= 0x80;
	regs[3] &= 0xC0;
	regs[4] &= 0xC0;
	regs[5] &= 0xE0;
	regs[6] = to_bcd((date.tm_year + 1900) % 100);
	regs[7] &= 0xF8;

	regs[1] |= to_bcd(date.tm_sec);
	regs[2] |= to_bcd(date.tm_min);
	regs[3] |= to_bcd(date.tm_hour);
	regs[4] |= to_bcd(date.tm_mday);
	regs[5] |= to_bcd(date.tm_mon + 1);
	regs[7] |= to_bcd(date.tm_wday);

	am1815_write_bulk(rtc, 0x0, regs, 8);
}
