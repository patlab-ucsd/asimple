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

#define LORA_DIO_MAPPING0_3 0x40u
#define LORA_DIO_MAPPING4_5 0x41u
#define LORA_VERSION 0x42u
#define LORA_TCXO_XTAL 0x4Bu
#define LORA_PA_DAC 0x4Du
#define LORA_FORMER_TEMPERATURE 0x5Bu
#define LORA_AGC_REF 0x61u
#define LORA_AGC_THRESH1 0x62u
#define LORA_AGC_THRESH2 0x63u
#define LORA_AGC_THRESH3 0x64u
#define LORA_PLL 0x70

/** Structure holding RFM95W information and state. */
struct lora
{
	struct spi spi;
};

enum lora_lna_gain
{
    LORA_LNA_G1,
    LORA_LNA_G2,
    LORA_LNA_G3,
    LORA_LNA_G4,
    LORA_LNA_G5,
    LORA_LNA_G6
};

/** Initializes the lora module.
 *
 * Internally, this module uses the first SPI module on the Artemis, taking
 * ownership over it. Specifically, the SPI is initialized using pin 11 as the
 * chip select for the LoRa radio.
 *
 * This implementation assumes a radio module compatible with the Semtech
 * SX1276.
 *
 * The init function checks to see if it recognizes the silicon version of the
 * module it finds. If it does not, it returns false.
 *
 * On a successful return, the LoRa module is fully configured, and set to its
 * sleep mode. Specifically:
 *  - Sets the radio to LoRa mode
 *  - The frequency is set to the parameter provided, setting up the module for
 *    the correct frequency range desired
 *  - The power amplifier is enabled, using the PA_OUTPUT pin for output
 *  - LNA high current is enabled
 *
 * All other parameters are left at their defaults. It is recommended to
 * explicitly set the spreading factor, bandwidth, and the coding rate after
 * initialization.
 *
 * FIXME do I want to allow passing in a spi device??? this way, so long as the
 * device is configured to use different chip selects, these can be used in
 * tandem?
 *
 * @param[out] lora Pointer to the lora structure to initialize.
 * @param[in] frequency
 *
 * @returns True on success, false otherwise. Fails if LoRa module is unknown.
 */
bool lora_init(struct lora *lora, uint32_t frequency);

/** Receives a single LoRa packet, blocking until it is received.
 *
 * FIXME is this acceptable? Or should we always allow trying to copy a packet,
 * so long as we have a valid one in the FIFO?
 * The lora module must be in its receiving mode, else nothing is returned.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[out] buffer Pointer to buffer to move packet to.
 * @param[in] buffer_size The size of the buffer in bytes.
 *
 * FIXME should we return if we actually return something??
 */
void lora_receive_packet(struct lora *lora, unsigned char buffer[], size_t buffer_size);

/** Sends a LoRa packet, blocking until it is transmitted.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] buffer Pointer to the buffer containing the data to transmit.
 * @param[in] buffer_size The number of bytes to transmit.
 *
 * FIXME should there be any status return?
 */
void lora_send_packet(struct lora *lora, const unsigned char buffer[], uint8_t buffer_size);

/** Configures the LoRa spreading factor for RX and TX.
 *
 * Valid spreading factors are between 6 and 12. Note, that for a spreading
 * factor of 6 the packet mode is automatically changed to implicit (packet
 * length must be agreed upon by transmitter and receiver).
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] spreading_factor A value between 6 and 12.
 *
 * @returns True on success, false if the requested spreading factor is
 *  invalid.
 */
bool lora_set_spreading_factor(struct lora *lora, uint8_t spreading_factor);

/** Returns the current LoRa spreading factor.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 *
 * @returns The current spreading factor.
 */
uint8_t lora_get_spreading_factor(struct lora *lora);

/** Returns the number of bytes in the last packet received.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 *
 * @returns The number of bytes in the last packet received.
 */
uint8_t lora_rx_amount(struct lora *lora);

/** Changes the LoRa module to receive mode. FIXME should we expose this?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 */
void lora_receive_mode(struct lora *lora);

/** Changes the LoRa module to transmit mode. FIXME should we expose this?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 */
void lora_transmit_mode(struct lora *lora);

/** Returns whether the LoRa module is in its transmit mode.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 *
 * @returns True if the module is in its transmit mode, false otherwise.
 */
bool lora_transmitting(struct lora *lora);

/** Changes the LoRa module's mode to sleep.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 */
void lora_sleep(struct lora *lora);

/** Changes the LoRa module's mode to standby.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 */
void lora_standby(struct lora *lora);

/** Sets the LoRa module's frequency to the value specified.
 *
 * FIXME can this fail?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] frequency The frequency to set the carrier to.
 */
void lora_set_frequency(struct lora *lora, uint32_t frequency);

/** Sets the LoRa module's bandwidth to the value specified.
 *
 * FIXME can this fail?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] bandwidth The bandwidth to set the module to.
 */
void lora_set_bandwidth(struct lora *lora, uint8_t bandwidth);

/** Gets the LoRa module's bandwidth setting.
 *
 * FIXME can this fail?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 *
 * @returns The current bandwidth the module is configured for.
 */
uint8_t lora_get_bandwidth(struct lora *lora);

/** Sets the LoRa module's coding rate to the value specified.
 *
 * FIXME can this fail?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] rate The coding rate to set the module to.
 */
void lora_set_coding_rate(struct lora *lora, uint8_t rate);

/** Gets the LoRa module's coding rate setting.
 *
 * FIXME can this fail?
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 *
 * @returns The current coding rate the module is configured for.
 */
uint8_t lora_get_coding_rate(struct lora *lora);

/** Get the value of the arbitrary LoRa register requested.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] address Address of the register to read.
 *
 * @returns The value of the register requested.
 */
uint8_t lora_get_register(struct lora *lora, uint8_t address);

/** Configures the Low Noise Amplifier (LNA) for receiving packets.
 *
 * According to the SX1276 datasheet, the LNA gain selection is controleld by
 * the auto gain control mode-- if that is enabled, the gain is automatically
 * selected, ignoring the value presented to this function. The value is
 * recorded in the hardware, however, should the AGC be turned off.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] gain The gain setting for the LNA. G1 is the highest gain, G6 is
 *  the lowest.
 * @param[in] high_current Whether to boost the current to 150% or not, ignored
 *  for low frequency operation.
 */
void lora_set_lna(struct lora *lora, enum lora_lna_gain gain, bool high_current);

/** Sets the transmit level in dBm.
 *
 * Note, that for the RFM95W module, the low power antenna pins are
 * disconnected-- for this board always set high_power to true.
 *
 * This function configures the Power Amplifier (PA), max power, and desired
 * power levels based on the given parameters.
 *
 * @param[in,out] lora Pointer to an initialized lora structure.
 * @param[in] dBm The transmit gain to target. Must be between -4 and 15 for
 *  normal power, or 2 and 17 for high power.
 * @param[in] high_power Whether to enable the Power Amplifier (PA) for
 * transmitting, using the PA_BOOST pin for output. Otherwise the RFO_LF or
 *  RFO_HF pins are used for output.
 */
bool lora_set_transmit_level(struct lora *lora, int8_t dBm, bool high_power);

#endif//LORA_H_
