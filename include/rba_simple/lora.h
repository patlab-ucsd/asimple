// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#ifndef LORA_H_
#define LORA_H_

#include <spi.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Radio registers, from the Semtech SX1276 datasheet
#define LORA_FIFO 0x00u
#define LORA_OPMODE 0x01u
#define LORA_FREQ_MSB 0x06u
#define LORA_FREQ_MID 0x07u
#define LORA_FREQ_LSB 0x08u
#define LORA_PA_CONFIG 0x09u
#define LORA_PA_RAMP 0x0Au
#define LORA_PA_OCP 0x0Bu
#define LORA_LNA 0x0Cu
#define LORA_FIFO_ADDR 0x0Du
#define LORA_FIFO_TX_BASE 0x0Eu
#define LORA_FIFO_RX_BASE 0x0Fu
#define LORA_FIFO_RX_CURRENT_ADDR 0x10u
#define LORA_IRQ_FLAGS_MASK 0x11u
#define LORA_IRQ_FLAGS 0x12u
#define LORA_RX_BYTES 0x13u
#define LORA_RX_HEADER_CNT_MSB 0x14u
#define LORA_RX_HEADER_CNT_LSB 0x15u
#define LORA_RX_PACKET_CNT_MSB 0x16u
#define LORA_RX_PACKET_CNT_LSB 0x17u
#define LORA_MODEM_STATUS 0x18u
#define LORA_PACKET_SNR 0x19u
#define LORA_PACKET_RSSI 0x1Au
#define LORA_RSSI 0x1Bu
#define LORA_HOP_CHANNEL 0x1Cu
#define LORA_MODEM_CONFIG1 0x1Du
#define LORA_MODEM_CONFIG2 0x1Eu
#define LORA_SYMBOL_TIMEOUT 0x1Fu
#define LORA_PREAMBLE_LEN_MSB 0x20u
#define LORA_PREAMBLE_LEN_LSB 0x21u
#define LORA_PAYLOAD_LEN 0x22u
#define LORA_MAX_PAYLOAD_LEN 0x23u
#define LORA_HOP_PERIOD 0x24u
#define LORA_FIFO_RX_BYTE_ADDR 0x25u
#define LORA_MODEM_CONFIG3 0x26u
#define LORA_PPM_CORRECTION 0x27u
#define LORA_FREQ_ERROR_MSB 0x28u
#define LORA_FREQ_ERROR_MID 0x29u
#define LORA_FREQ_ERROR_LSB 0x2Au
#define LORA_RESERVED1 0x2Bu
#define LORA_RSSI_WIDEBAND 0x2Cu
#define LORA_RESERVED2 0x2Du
#define LORA_RESERVED3 0x2Eu
// See ERRATA 2.3 Receiver Spurious Reception of a LoRa Signal
#define LORA_INTERMEDIATE_FREQ2 0x2Fu
#define LORA_INTERMEDIATE_FREQ1 0x30u

#define LORA_DETECT_OPTIMIZE 0x31u
#define LORA_RESERVED4 0x32u
#define LORA_INVERT_IQ 0x33u
#define LORA_RESERVED5 0x34u
#define LORA_RESERVED6 0x35u
// See ERRATA 2.1 Sensitivity Optimization with a 500 kHz Bandwidth
#define LORA_HIGH_BW_OPTIMIZE1 0x36u

#define LORA_DETECTION_THRESHOLD 0x37u
#define LORA_RESERVED7 0x38u
#define LORA_SYNC_WORD 0x39u
// See ERRATA 2.1 Sensitivity Optimization with a 500 kHz Bandwidth
#define LORA_HIGH_BW_OPTIMIZE2 0x3Au

#define LORA_INVERT_IQ2 0x3Bu
#define LORA_RESERVED8 0x3Cu
#define LORA_RESERVED9 0x3Du
#define LORA_RESERVED10 0x3Eu
#define LORA_RESERVED11 0x3Fu

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
