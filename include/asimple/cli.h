// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
/// @file

#ifndef CLI_H_
#define CLI_H_

#include <stddef.h>
#include <stdbool.h>

typedef char cli_line_buffer[32];

struct ring_buffer
{
	bool empty;
	size_t end;
	size_t start;
	cli_line_buffer data[4];
};

void ring_buffer_initialize(struct ring_buffer *buf);
cli_line_buffer *ring_buffer_get_current(struct ring_buffer *buf);
cli_line_buffer *ring_buffer_get(struct ring_buffer *buf, size_t index);
void ring_buffer_advance(struct ring_buffer *buf);
bool ring_buffer_empty(const struct ring_buffer *buf);
size_t ring_buffer_in_use(struct ring_buffer *buf);

struct cli
{
	bool echo;
	struct ring_buffer history;
};

void cli_initialize(struct cli *cli);
cli_line_buffer *cli_read_line(struct cli *cli);

/** Read a line from the console.
 *
 * Reads a line from the console. It discards the carriage return at the end.
 *
 * @param[out] buf Buffer to hold the contents read from the line.
 * @param[in] in Size of the buffer. Any additional characters past size - 1
 *  are dropped/ignored.
 * @param[in] echo Whether to print out incoming characters or not.
 *
 * @post The given buffer holds at most size - 1 characters. String in buffer
 *  is null delimited.
 * @returns RETURNCODE_SUCCESS on success, anything else on error.
 */
int read_line(char buf[], size_t size, bool echo);

#endif//CLI_H_
