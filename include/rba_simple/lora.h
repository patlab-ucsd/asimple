// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef LORA_H_
#define LORA_H_

#include <spi.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Structure holding RFM95W information and state. */
struct lora
{
	struct spi spi;
};

/** Initialize the lora module.
 *
 * @param[out] lora Pointer to the lora structure to initialize.
 * @param[in] frequency
 */
void lora_init(struct lora *lora, uint32_t frequency);

void lora_receive_packet(struct lora *lora, unsigned char buffer[], size_t buffer_size);
void lora_send_packet(struct lora *lora, unsigned char buffer[], uint8_t buffer_size);
void lora_set_spreading_factor(struct lora *lora, uint8_t spreading_factor);
uint8_t lora_get_spreading_factor(struct lora *lora);
uint8_t lora_rx_amount(struct lora *lora);
void lora_receive_mode(struct lora *lora);
void lora_transmit_mode(struct lora *lora);
bool lora_transmitting(struct lora *lora);
void lora_sleep(struct lora *lora);
void lora_standby(struct lora *lora);
void lora_set_frequency(struct lora *lora, uint32_t frequency);
void lora_set_bandwidth(struct lora *lora, uint8_t bandwidth);
uint8_t lora_get_bandwidth(struct lora *lora);
void lora_set_coding_rate(struct lora *lora, uint8_t rate);
uint8_t lora_get_coding_rate(struct lora *lora);
uint8_t lora_get_register(struct lora *lora, uint8_t address);
#endif//LORA_H_
