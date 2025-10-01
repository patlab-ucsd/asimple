// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
/// @file

#ifndef SD_CARD_H_
#define SD_CARD_H_

#include <spi.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct sd_card
{
	struct spi_device *spi;
	size_t blocks;
};

/** Initializes the SD card.
 *
 * This goes through the entire initialization process for SD cards.
 *
 * @param[in,out] sd_card SD card structure to initialize.
 * @param[in,out] spi SPI device to which the SD card is connected.
 *
 * @returns 0 on success, some error token or error response from the SD card
 *  on an error.
 */
uint8_t sd_card_init(struct sd_card *sd_card, struct spi_device *spi);

/** Sends a command to an initialized SD card.
 *
 * @param[in,out] sd_card SD card to send a command to.
 * @param[in] command Command to send to the SD card.
 * @param[in] data Argument to the command to send.
 *
 * @returns The R1 response from the SD card.
 */
uint8_t
sd_card_command(struct sd_card *sd_card, uint8_t command, uint32_t data);

/** Sends a command to an initialized SD card.
 *
 * @param[in,out] sd_card SD card to send a command to.
 * @param[in] command Command to send to the SD card.
 * @param[in] data Argument to the command to send.
 * @param[out] result The result data from the command, starting with the SD
 *  card response as the first byte.
 * @param[in] size The number of bytes to read from the SD card as a response.
 *
 * @returns The R1 response from the SD card.
 */
uint8_t sd_card_command_result(
	struct sd_card *sd_card, uint8_t command, uint32_t data, uint8_t *result,
	size_t size
);

/** Reads the given number of blocks from the SD card.
 *
 * @param[in,out] sd_card SD card to read from.
 * @param[in] block The blocks to number to read from.
 * @param[out] buffer Where to read the data to.
 * @param[in] blocks How many blocks to read.
 *
 * @returns The R1 response from the SD card. On an error, the contents of
 *  buffer are undefined.
 */
uint8_t sd_card_read_blocks(
	struct sd_card *sd_card, uint32_t block, uint8_t *buffer, size_t blocks
);

/** Writes the given number of blocks to the SD card.
 *
 * @param[in,out] sd_card SD card to write to.
 * @param[in] block The block number to write to.
 * @param[out] buffer The data to write to the SD card.
 * @param[in] blocks How many blocks to write.
 *
 * @returns The R1 response from the SD card.
 *
 * FIXME On an error, we should check CMD13 to figure out what went wrong if
 * possible.
 */
uint8_t sd_card_write_blocks(
	struct sd_card *sd_card, uint32_t block, const uint8_t *buffer,
	size_t blocks
);

/** Detects whether an SD card is plugged in or not.
 *
 * FIXME still need to implement.
 *
 * From spec: For Card detection, the host detects that the line is pulled high.
 * SET_CLR_CARD_DETECT (ACMD42) to clear this, should be done by card init?
 */
bool sd_card_detected(struct spi_device *spi);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SD_CARD_H_
