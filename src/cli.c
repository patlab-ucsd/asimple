// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

static const int ring_size = 4;

void cli_init(struct cli *cli)
{
	ring_buffer_init(&cli->history, ring_size);
}

int read_line(char buf[], size_t size, bool echo)
{
	int c;
	size_t i = 0;

	while (1)
	{
		c = getchar();
		if (c == EOF)
		{
			buf[i] = '\0';
			return -1;
		}

		if (echo)
			putchar((char)c);
		// Check for special characters
		if (c == '\b') {
			if (i > 0) {
				--i;
			}
			if (echo)
			{
				putchar(' ');
				putchar('\b');
				fflush(stdout);
			}
			continue;
		}
		else if (c == '\r') {
			buf[i] = '\0';
			if (echo)
			{
				putchar('\n');
				fflush(stdout);
			}
			break;
		}
		if (echo)
			fflush(stdout);
		if (i < size-1)
			buf[i++] = c;
	}
	return 0;
}

cli_line_buffer* cli_read_line(struct cli *cli)
{
	cli_line_buffer *buf = ring_buffer_get(&cli->history, ring_size - 1);
	int result = read_line((char*)buf, ring_size, cli->echo);
	if (result == 0)
	{
		ring_buffer_advance(&cli->history);
		return buf;
	}
	return NULL;
}

void ring_buffer_init(struct ring_buffer *buf, size_t size)
{
	buf->empty = true;
	buf->end = 0;
	buf->start = 0;
	static_assert(sizeof(buf->data[0]) > 8, "wrong size guess");
	buf->data = malloc(sizeof(buf->data[0])*size);
}

void ring_buffer_destroy(struct ring_buffer *buf)
{
	buf->empty = true;
	buf->end = 0;
	buf->start = 0;
	free(buf->data);
	buf->data = NULL;
}

cli_line_buffer *ring_buffer_get(struct ring_buffer *buf, size_t index)
{
	const size_t ring_size = 4;
	return &buf->data[(buf->start - index) % ring_size];
}

void ring_buffer_advance(struct ring_buffer *buf)
{
	if (buf->empty)
		buf->empty = false;
	buf->start = (buf->start + 1) % ring_size;
	if (buf->start == buf->end)
		buf->end = (buf->end + 1) % ring_size;;
}

size_t ring_buffer_in_use(struct ring_buffer *buf)
{
	size_t result = (buf->start - buf->end + ring_size) % ring_size;
	if (result == 0 && !buf->empty)
		return ring_size;
	return result;
}

bool ring_buffer_empty(const struct ring_buffer *buf)
{
	return buf->empty;
}
