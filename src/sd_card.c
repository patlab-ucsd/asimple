// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <sd_card.h>
#include <systick.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define SD_CARD_START_TOKEN 0xFEu
#define SD_CARD_STOP_WRITE_TOKEN 0xFDu

static const uint8_t crc7_table[256] = {
	0x00, 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x90, 0x82, 0xb4, 0xa6,
	0xd8, 0xca, 0xfc, 0xee, 0x32, 0x20, 0x16, 0x04, 0x7a, 0x68, 0x5e, 0x4c,
	0xa2, 0xb0, 0x86, 0x94, 0xea, 0xf8, 0xce, 0xdc, 0x64, 0x76, 0x40, 0x52,
	0x2c, 0x3e, 0x08, 0x1a, 0xf4, 0xe6, 0xd0, 0xc2, 0xbc, 0xae, 0x98, 0x8a,
	0x56, 0x44, 0x72, 0x60, 0x1e, 0x0c, 0x3a, 0x28, 0xc6, 0xd4, 0xe2, 0xf0,
	0x8e, 0x9c, 0xaa, 0xb8, 0xc8, 0xda, 0xec, 0xfe, 0x80, 0x92, 0xa4, 0xb6,
	0x58, 0x4a, 0x7c, 0x6e, 0x10, 0x02, 0x34, 0x26, 0xfa, 0xe8, 0xde, 0xcc,
	0xb2, 0xa0, 0x96, 0x84, 0x6a, 0x78, 0x4e, 0x5c, 0x22, 0x30, 0x06, 0x14,
	0xac, 0xbe, 0x88, 0x9a, 0xe4, 0xf6, 0xc0, 0xd2, 0x3c, 0x2e, 0x18, 0x0a,
	0x74, 0x66, 0x50, 0x42, 0x9e, 0x8c, 0xba, 0xa8, 0xd6, 0xc4, 0xf2, 0xe0,
	0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x70, 0x82, 0x90, 0xa6, 0xb4,
	0xca, 0xd8, 0xee, 0xfc, 0x12, 0x00, 0x36, 0x24, 0x5a, 0x48, 0x7e, 0x6c,
	0xb0, 0xa2, 0x94, 0x86, 0xf8, 0xea, 0xdc, 0xce, 0x20, 0x32, 0x04, 0x16,
	0x68, 0x7a, 0x4c, 0x5e, 0xe6, 0xf4, 0xc2, 0xd0, 0xae, 0xbc, 0x8a, 0x98,
	0x76, 0x64, 0x52, 0x40, 0x3e, 0x2c, 0x1a, 0x08, 0xd4, 0xc6, 0xf0, 0xe2,
	0x9c, 0x8e, 0xb8, 0xaa, 0x44, 0x56, 0x60, 0x72, 0x0c, 0x1e, 0x28, 0x3a,
	0x4a, 0x58, 0x6e, 0x7c, 0x02, 0x10, 0x26, 0x34, 0xda, 0xc8, 0xfe, 0xec,
	0x92, 0x80, 0xb6, 0xa4, 0x78, 0x6a, 0x5c, 0x4e, 0x30, 0x22, 0x14, 0x06,
	0xe8, 0xfa, 0xcc, 0xde, 0xa0, 0xb2, 0x84, 0x96, 0x2e, 0x3c, 0x0a, 0x18,
	0x66, 0x74, 0x42, 0x50, 0xbe, 0xac, 0x9a, 0x88, 0xf6, 0xe4, 0xd2, 0xc0,
	0x1c, 0x0e, 0x38, 0x2a, 0x54, 0x46, 0x70, 0x62, 0x8c, 0x9e, 0xa8, 0xba,
	0xc4, 0xd6, 0xe0, 0xf2
};

static uint8_t crc7_update(uint8_t *data, size_t size, uint8_t crc)
{
	for (size_t i = 0; i < size; ++i)
	{
		uint8_t byte = data[i];
		crc = crc7_table[(byte ^ crc) & 0xFF];
	}
	return crc >> 1;
}

static const uint16_t crc16_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108,
	0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
	0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b,
	0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
	0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee,
	0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
	0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d,
	0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5,
	0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
	0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4,
	0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
	0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13,
	0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
	0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e,
	0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1,
	0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
	0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0,
	0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
	0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657,
	0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
	0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882,
	0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2e,
	0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
	0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d,
	0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
	0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

static uint16_t crc16_update(uint8_t *data, size_t size, uint16_t crc)
{
	for (size_t i = 0; i < size; ++i)
	{
		uint8_t byte = data[i];
		crc = crc16_table[(byte ^ (crc >> 8)) & 0xFF] ^ (crc << 8);
	}
	return crc;
}

static uint8_t begin_transaction(struct sd_card *sd_card, uint8_t command, uint32_t argument)
{
	uint8_t buf[6];
	buf[0] = 0x40 | command;
	buf[1] = argument >> 24;
	buf[2] = argument >> 16;
	buf[3] = argument >> 8;
	buf[4] = argument;
	buf[5] = (crc7_update(buf, 5, 0) << 1) | 1;

	spi_device_write_continue(sd_card->spi, buf, 6);

	return 0;
}

static void read_spi(struct sd_card *sd_card, uint8_t *buffer, size_t size)
{
	spi_device_hold_mosi(sd_card->spi, true);
	spi_device_read_continue(sd_card->spi, buffer, size);
	spi_device_release_mosi(sd_card->spi);
}

static void read_spi_last(struct sd_card *sd_card, uint8_t *buffer, size_t size)
{
	spi_device_hold_mosi(sd_card->spi, true);
	spi_device_read(sd_card->spi, buffer, size);
	spi_device_release_mosi(sd_card->spi);
}

static uint8_t get_R1(struct sd_card *sd_card)
{
	uint8_t buf;
	// N_CR per the spec is between 1 and 8 8 clock cycle counts
	// 7.5.4 Timing Values in Physical Layer Specification Version 3.01
	size_t retry = 0;
	for (; retry < 80; ++retry)
	{
		read_spi(sd_card, &buf, 1);
		if (!(buf & 0x80))
			break;
	}
	return buf;
}

static uint8_t initialize_v1(struct sd_card *sd_card)
{
	// READ_OCR, to check for valid voltages from SD card
	uint8_t cmd_result[5];
	uint32_t status = sd_card_command_result(sd_card, 58, 0x0, cmd_result, 5);
	if (status != 0x01)
		return status;
	if (!(cmd_result[3] & 0x80) || cmd_result[2] != 0xFF)
	{
		// FIXME We reject the SD card voltage ranges!
		return 0xFFu;
	}

	// Send ACMD41
	do
	{
		status = sd_card_command(sd_card, 55, 0x0);
		status = sd_card_command(sd_card, 41, 0x40000000);
	} while (status == 0x01);

	if (status == 0x00)
	{
		spi_device_set_clock(sd_card->spi, 25000000);
		// FIXME get CSD, get size
	}
	return status;
}

static uint8_t initialize_v2(struct sd_card *sd_card)
{
	uint8_t cmd_result[30] = {0};
	// READ_OCR, to check for valid voltages from SD card
	uint32_t status = sd_card_command_result(sd_card, 58, 0x0, cmd_result, 5);
	if (status != 0x01)
		return status;
	if (!(cmd_result[3] & 0x80) || cmd_result[2] != 0xFF)
	{
		// FIXME We reject the SD card voltage ranges!
		return 0xFFu;
	}

	// Send ACMD41
	do
	{
		status = sd_card_command(sd_card, 55, 0x0);
		status = sd_card_command(sd_card, 41, 0x40000000);
	} while (status == 0x01);

	if (status != 0x00)
		return status;

	// READ_OCR, to check for CCS
	status = sd_card_command_result(sd_card, 58, 0x0, cmd_result, 5);
	// We shouldn't be IDLE anymore, so bail if we are
	if (status != 0x00)
		return status;

	if (!(cmd_result[1] & 0x80))
	{
		// FIXME Something bad happened, card isn't powered up?
		return 0xFFu;
	}

	bool ccs = cmd_result[1] & 0x40;
	if (ccs)
	{
		// FIXME Ver2.00 or later high capacity or extended
	}
	else
	{
		// FIXME Ver2.00 or later standard capacity
	}

	// FIXME do we care about supporting version 1?

	status = sd_card_command_result(sd_card, 9, 0, cmd_result, 30);
	status = sd_card_command_result(sd_card, 9, 0, cmd_result, 30);
	if (status != 0)
		return status;

	if (cmd_result[1] != SD_CARD_START_TOKEN)
	{
		// FIXME we did not receive a start_block token!
		return 0xFFu;
	}
	uint16_t crc = crc16_update(cmd_result+2, 16, 0);
	if (crc != ((cmd_result[18] << 8) | cmd_result[19]))
	{
		// FIXME we did not receive a valid CRC!
		return 0xFFu;
	}
	// FIXME we now have CSD-- what fields do we want to check?
	uint8_t *csd = cmd_result+2;
	sd_card->blocks = (csd[9] | csd[8] << 8 | (0x3F & csd[7]) << 16) + 1;
	sd_card->blocks *= 1024;

	// FIXME should we enable CRC7 for every command? By default SPI doesn't
	// need it, even if we're actually computing it.
	//spi_device_set_clock(sd_card->spi, 25000000);

	//FIXME should we disbble pullup on CS?
	return 0;
}

uint8_t sd_card_init(struct sd_card *sd_card, struct spi_device *spi)
{
	if (!systick_started())
	{
		// FIXME systick must be started!
		return 0xFFu;
	}
	sd_card->spi = spi;

	spi_device_set_clock(spi, 100000);
	// 10 bytes * 8 = 80 clocks, SD needs at least 74 clocks
	spi_device_toggle(spi, 10);

	// Try to set card to IDLE state
	uint32_t status = sd_card_command(sd_card, 0, 0);
	if (status != 0x01)
		return status;

	// Send CMD8 -- Send Interface Condition
	// Used to inform SD of valid supply voltages.
	// We're telling it we're supplying between 2.7V and 3.6V, and using a
	// check pattern of 0xAA. It should return an R7 response of 5 bytes:
	//
	//	39-32   |	31-28	| 27-12 |   11-8  |	 7-0
	//  R1 status | CMD version |   0   | voltage | pattern echo
	uint8_t cmd_result[5];
	status = sd_card_command_result(sd_card, 8, 0x000001AA, cmd_result, 5);

	// If command is invalid, this might be a V1 card
	if (status == 0x05)
		return initialize_v1(sd_card);
	// Else continue with V2 initialization
	if (status != 0x01)
		return status;

	if (cmd_result[4] != 0xAA && cmd_result[3] != 0x01)
	{
		// FIXME SD card rejected our voltage!
		return 0xFFu;
	}

	return initialize_v2(sd_card);
}

uint8_t sd_card_command(
	struct sd_card *sd_card, uint8_t command, uint32_t data)
{
	uint8_t result;
	sd_card_command_result(sd_card, command, data, &result, 1);
	return result;
}

uint8_t sd_card_command_result(
	struct sd_card *sd_card,
	uint8_t command,
	uint32_t data,
	uint8_t *result,
	size_t size)
{
	begin_transaction(sd_card, command, data);

	// N_CR per the spec is between 1 and 8 8 clock cycle counts
	// 7.5.4 Timing Values in Physical Layer Specification Version 3.01
	size_t retry = 0;
	read_spi(sd_card, result, 1);
	for (; retry < 7; ++retry)
	{
		read_spi(sd_card, result, 1);
		if (!(result[0] & 0x80))
			break;
	}
	if (retry >= 7)
		return result[0];

	// In case we ask for more than one result byte
	if (size - 1)
	{
		// FIXME this only handles CID and CSD???
		// N_CX is between 0 and 8 8 clock cycle counts
		if (command == 9 || command == 10)
		{
			for (retry = 0; retry < 8; ++retry)
			{
				read_spi(sd_card, result+1, 1);
				if (!(result[1] != 0xFE))
					break;
			}
			read_spi_last(sd_card, result + 2, size - 2);
		}
		else
			read_spi_last(sd_card, result + 1, size - 1);
	}

	// We need to toggle 8 clocks with CS being deasserted for spec conformance
	// This is also set CS high for us, in case the last spi command we sent
	// was a continue transaction
	spi_device_toggle(sd_card->spi, 1);
	return result[0];
}

uint8_t sd_card_read_blocks(
	struct sd_card *sd_card,
	uint32_t block,
	uint8_t *buffer,
	size_t blocks)
{
	if (blocks == 0)
		return 0;

	if (block + blocks - 1 > sd_card->blocks)
		return 0xFFu;

	uint8_t result;
	// FIXME this only works for CCS=1 cards
	begin_transaction(sd_card, blocks == 1 ? 17 : 18, block);
	uint8_t r1 = get_R1(sd_card);
	if (r1 != 0x00)
	{
		result = r1;
		goto terminate;
	}

	for (uint8_t *pos = buffer; pos < buffer + blocks * 512; pos += 512)
	{
		// FIXME up to 100ms for V2, what about V1?
		unsigned start = systick_jiffies();
		uint8_t token;
		do
		{
			read_spi(sd_card, &token, 1);
		} while(token != SD_CARD_START_TOKEN && (systick_jiffies() - start < 100));

		if (token != SD_CARD_START_TOKEN)
		{
			result = token;
			goto terminate;
		}

		read_spi(sd_card, pos, 512);
		uint16_t crc16;
		read_spi(sd_card, (uint8_t*)&crc16, 2);
		// Data is actually in big endian...
		crc16 = crc16 >> 8 | crc16 << 8;

		if (crc16 != crc16_update(pos, 512, 0))
		{
			result = 0xFFu;
			goto terminate;
		}
	}

	if (blocks != 1)
	{
		return sd_card_command(sd_card, 12, 0);
	}
	result = 0;

terminate:
	spi_device_toggle(sd_card->spi, 1);
	return result;
}


