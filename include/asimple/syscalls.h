// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

/** Links the given RTC object with the time syscalls.
 *
 * gettimeofday uses the am1815 to get the time. This function informs the
 * syscall which RTC object to use for that purpose.
 *
 * @param[in,out] rtc AM1815 RTC object to use for system time.
 *
 * @post gettimeofday syscall should work properly.
 */
void initialize_time(struct am1815 *rtc_);

#endif//SYSCALLS_H_
