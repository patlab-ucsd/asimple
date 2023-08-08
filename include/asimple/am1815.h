// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
/// @file

#ifndef AM1815_H_
#define AM1815_H_

#include <spi.h>

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure representing the AM1815 RTC */
struct am1815
{
	struct spi_device *spi;
};

enum am1815_pulse_width
{
	AM1815_LEVEL = 0,
	AM1815_SHORTEST = 1,
	AM1815_1DIV64 = 2,
	AM1815_1DIV4 = 3,
};

/** Initializes the RTC structure.
 *
 * @param[out] rtc RTC object to initialize
 * @param[in,out] device The SPI object to use for communication with the
 *  physical RTC. It should already be initialized.
 */
void am1815_init(struct am1815 *rtc, struct spi_device *device);

/** Reads a register from the RTC.
 *
 * @param[in] rtc RTC to read the register from.
 * @param[in] addr RTC register address to read from.
 *
 * @returns The value of the register.
 */
uint8_t am1815_read_register(struct am1815 *rtc, uint8_t addr);

/** Writes a value to a register on the RTC.
 *
 * @param[in,out] rtc RTC to write to.
 * @param[in] addr Register address to write to.
 * @param[in] data Data to write to the register.
 */
void am1815_write_register(struct am1815 *rtc, uint8_t addr, uint8_t data);

/** Reads a series of registers in sequence.
 *
 * @param[in,out] rtc RTC to read from.
 * @param[in] addr Register address to start reading from.
 * @param[out] data Output buffer to save incoming reads to.
 * @param[in] size Size of output buffer, and how much to read.
 */
void am1815_read_bulk(struct am1815 *rtc, uint8_t addr, uint8_t *data, size_t size);

/** Writes a series of registers in sequence.
 *
 * @param[in,out] rtc RTC to read from.
 * @param[in] addr Register address to start writing to.
 * @param[out] data Output buffer with data to write to the RTC.
 * @param[in] size Size of input buffer, and how much to write.
 */
void am1815_write_bulk(struct am1815 *rtc, uint8_t addr, const uint8_t *data, size_t size);

/** Set RPT bits in Countdown Timer Control register to control how often the alarm interrupt repeats.
 * 0 means disable alarm, 1 means once per year, 2 means once per month, 3 means once per week,
 * 4 means once per day, 5 means once per hour, 6 means once per minute, 7 means once per second.
 *
 * @param[in,out] rtc RTC to set the repeat function.
 * @param[in] repeat number to decide how often the alarm repeats
 *
 * @returns true if successful and false if unsuccessful (repeat out of range)
 */
bool am1815_repeat_alarm(struct am1815 *rtc, int repeat);

/** Enables the trickle charging of the backup battery on the RTC.
 *
 * FIXME what are the settings?
 *
 * @param[in,out] rtc RTC to enable trickle charging of backup battery on.
 */
void am1815_enable_trickle(struct am1815 *rtc);

/** Disables the trickle charging of the backup battery on the RTC.
 *
 * @param[in,out] rtc RTC to disable trickle charging of backup battery of.
 */
void am1815_disable_trickle(struct am1815 *rtc);

/** Reads the time on the RTC.
 *
 * @param[in] rtc RTC to read the time of.
 *
 * @returns The RTC's time in seconds and microseconds. Note that the AM1815
 *  has at best centisecond resolution.
 */
struct timeval am1815_read_time(struct am1815 *rtc);

/** Writes the time to the RTC.
 *
 * @param[in] rtc RTC to write the time to.
 */
void am1815_write_time(struct am1815 *rtc, const struct timeval *time);

/** Reads the alarm time on the RTC.
 *
 * @param[in] rtc RTC to read the time of.
 *
 * @returns The RTC's alarm time in seconds and microseconds. Since there is no year,
 * we set the year to 0 (i.e, 0 years since 1900). Note that the AM1815 has at best
 * centiseconds resolution.
*/
struct timeval am1815_read_alarm(struct am1815 *rtc);

/** Writes the alarm time on the RTC.
 *
 * @param[in, out] rtc RTC to write the alarm time of.
 * @param[in] atime The time in seconds and microseconds that we want to set the alarm to.
*/
void am1815_write_alarm(struct am1815 *rtc, struct timeval *atime);

/**
 * Reads current value of RTC's countdown timer register.
 *
 * @param[in] rtc RTC to read the countdown timer of.
 *
 * @returns The current value of the countdown register (0x19).
*/
uint8_t am1815_read_timer(struct am1815 *rtc);

/**
 * Sets the RTC's countdown timer to a certain period. The timer will repeatedly
 * generate an interrupt at the specified interval.
 *
 * @param[in, out] rtc RTC to set the countdown timer of.
 * @param[in] timer The period (in seconds) that the timer will be set to. If
 * this parameter is 0 or too close to 0, then the timer will be disabled.
 *
 * @returns The actual period (in seconds) that the timer was set to. (It may
 * be different from the timer parameter because of the RTC's limitations.)
*/
double am1815_write_timer(struct am1815 *rtc, double timer);

/** Enable the alarm interrupt and output to FOUT.
 *
 * @param[in,out] rtc RTC to enable alarm on.
 * @param[in] pulse Pulse to set for alarm.
 */
void am1815_enable_alarm_interrupt(struct am1815 *rtc, enum am1815_pulse_width pulse);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//AM1815_H_
