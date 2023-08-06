// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
/// @file

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <uart.h>
#include <am1815.h>
#include <asimple_littlefs.h>

/** Links the given RTC object with the time syscalls.
 *
 * gettimeofday uses the am1815 to get the time. This function informs the
 * syscall which RTC object to use for that purpose.
 *
 * @param[in,out] rtc AM1815 RTC object to use for system time.
 *
 * @post gettimeofday syscall should work properly.
 */
void syscalls_rtc_init(struct am1815 *rtc);

/** Links the given UART object with stdin, stdout, and stderr.
 *
 * @param[in,out] uart UART object to use for system console IO.
 *
 * @post stdio functions like printf should work.
 */
void syscalls_uart_init(struct uart *uart);

/** Links the given UART object with stdin, stdout, and stderr.
 *
 * @param[in,out] uart UART object to use for system console IO.
 *
 * @post stdio functions like printf should work.
 */
void syscalls_littlefs_init(struct asimple_littlefs *fs);

#endif//SYSCALLS_H_
