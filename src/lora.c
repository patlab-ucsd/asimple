// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <lora.h>
#include <spi.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>

static uint8_t read_register(struct spi *spi, uint8_t address)
{
	uint32_t rx_buffer;
	spi_read(spi, address, &rx_buffer, 1);
	return rx_buffer;
}

static void write_register(struct spi *spi, uint8_t address, uint8_t data)
{
	uint32_t tx_buffer = data;
	spi_write(spi, address | 0x80, &tx_buffer, 1);
}

static void change_mode(struct spi *spi, uint8_t mode)
{
	uint8_t new_mode = read_register(spi, LORA_OPMODE);
	new_mode &= ~0x7u; // Clear mode bits
	new_mode |= mode & 0x7u;
	write_register(spi, LORA_OPMODE, new_mode);
}

void lora_init(struct lora *lora, uint32_t frequency)
{
	struct spi *spi = &lora->spi;
	spi_init(spi, 0);
	//int version = read_register(spi, 0x42);
	lora_sleep(lora);

	// Enable Lora mode
	write_register(spi, LORA_OPMODE, read_register(spi, 0x01) | 0x80);

	if (frequency > 800000000)
	{
		uint8_t reg = read_register(spi, LORA_OPMODE);
		reg &= 0xF7;
		write_register(spi, LORA_OPMODE, reg);
	}
	// FIXME what about low freq?

	lora_set_frequency(lora, frequency);
	// Set pointers for FIFOs, RX and TX
	write_register(spi, LORA_FIFO_TX_BASE, 0x80);
	write_register(spi, LORA_FIFO_RX_BASE, 0x00);

	// Turn on HF LNA boost
	write_register(spi, LORA_LNA, read_register(spi, LORA_LNA) | 0x03);

	// Enable AGC
	write_register(spi, LORA_MODEM_CONFIG3, 0x04);

	// Configure which pin to output of
	// enable RFO pin, 14dB
	write_register(spi, LORA_PA_CONFIG, 0x7E);

	lora_standby(lora);
}

void lora_receive_packet(struct lora *lora, unsigned char buffer[], size_t buffer_size)
{
	struct spi *spi = &lora->spi;
	// If we're not in reading mode, bail
	if ((read_register(spi, LORA_OPMODE) & 0x7) != 0x05)
		return;

	// FIXME check for explicit mode???

	uint8_t read_irq = read_register(spi, LORA_IRQ_FLAGS) & 0xF0;

	// If no CRC payload error, and RxDone
	// Ignoring timeout and valid header IRQs? FIXME
	if (!(read_irq & (1 << 5)) && (read_irq & (1 << 6)))
	{
		// FIXME should we be clearing ALL read IRQs?
		write_register(spi, LORA_IRQ_FLAGS, read_irq);

		uint8_t length = read_register(spi, LORA_RX_BYTES);
		// Move current pointer to current Rx location
		write_register(spi, LORA_FIFO_ADDR, read_register(spi, LORA_FIFO_RX_CURRENT_ADDR));

		lora_standby(lora);

		// Now, start reading?
		for (uint8_t i = 0; i < length; ++i)
		{
			// FIXME use buffer_size!
			buffer[i] = read_register(spi, LORA_FIFO);
		}
		// Update pointer to FIFO to beginning
		write_register(spi, LORA_FIFO_ADDR, 0x0);
		lora_receive_mode(lora);
	}
}

void lora_send_packet(struct lora *lora, unsigned char buffer[], uint8_t buffer_size)
{
	struct spi *spi = &lora->spi;
	// FIXME do we want to not transmit if we're in receive mode???

	lora_standby(lora);

	// Clear IRQ if set
	uint8_t tx_irq = read_register(spi, LORA_IRQ_FLAGS) & 0x08;
	write_register(spi, LORA_IRQ_FLAGS, tx_irq);

	// FIXME check for explicit mode?

	// FIXME 0x80 is hardcoded TX base address
	write_register(spi, LORA_FIFO_ADDR, 0x80);
	// FIXME what if buffer_size > space available in FIFO buffer?
	write_register(spi, LORA_PAYLOAD_LEN, buffer_size);

	for (uint8_t i = 0; i < buffer_size; ++i)
	{
		write_register(spi, LORA_FIFO, buffer[i]);
	}

	lora_transmit_mode(lora);

	tx_irq = read_register(spi, LORA_IRQ_FLAGS) & 0x08;
	while (!(read_register(spi, LORA_IRQ_FLAGS & 0x08)))
	{
		// FIXME is there an interrupt way to wait while going to sleep?
		// wait
	}
	tx_irq = read_register(spi, LORA_IRQ_FLAGS) & 0x08;
	write_register(spi, LORA_IRQ_FLAGS, tx_irq);
}

void lora_set_spreading_factor(struct lora *lora, uint8_t spreading_factor)
{
	struct spi *spi = &lora->spi;
	if (spreading_factor < 6)
		spreading_factor = 6;
	else if (spreading_factor > 12)
		spreading_factor = 12;

	// with spreading_factor 6, required:
	// - header must be implicit
	// - register 0x31 must be 0b101 for lower 3 bits
	// - 0x37 must be 0x0C
	if (spreading_factor == 6)
	{
		write_register(spi, LORA_DETECT_OPTIMIZE, 0xC5);
		write_register(spi, LORA_DETECTION_THRESHOLD, 0x0C);
	}
	else
	{
		write_register(spi, LORA_DETECT_OPTIMIZE, 0xC3);
		write_register(spi, LORA_DETECTION_THRESHOLD, 0x0A);
	}

	uint8_t config = read_register(spi, LORA_MODEM_CONFIG2);
	config &= 0x0F;
	config |= spreading_factor << 4;
	write_register(spi, LORA_MODEM_CONFIG2, config);
}

uint8_t lora_get_spreading_factor(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	return read_register(spi, LORA_MODEM_CONFIG2) >> 4;
}

uint8_t lora_rx_amount(struct lora *lora)
{
	struct spi *spi = &lora->spi;

	// FIXME should we respond based on any of the IRQs?
	uint8_t read_irq = read_register(spi, LORA_IRQ_FLAGS) & 0xF0;
	if ((read_irq & 0x70) == 0x50)
		return read_register(spi, LORA_RX_BYTES);
	return 0;
}

void lora_receive_mode(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	/* explicit header */
	write_register(spi, LORA_MODEM_CONFIG1, read_register(spi, LORA_MODEM_CONFIG1) & ~0x1u);
	change_mode(spi, 0x05);
}

void lora_transmit_mode(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	/* explicit header */
	write_register(spi, LORA_MODEM_CONFIG1, read_register(spi, LORA_MODEM_CONFIG1) & ~0x1u);
	change_mode(spi, 0x03);
}


bool lora_transmitting(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	return (read_register(spi, LORA_OPMODE) & 0x07) == 0x3;
}

void lora_sleep(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	change_mode(spi, 0);
}

void lora_standby(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	change_mode(spi, 1);
}

void lora_set_frequency(struct lora *lora, uint32_t frequency)
{
	struct spi *spi = &lora->spi;

	// fsf = fstep * frequency
	// fstep = f_xosc / 2^19
	// fsf = f_xosc / 2^19 * frequency
	// For this chip, f_xosc = 32MHz
	uint32_t fsf = (1 << 19) / 32000000.0 * frequency;
	write_register(spi, LORA_FREQ_MSB, fsf >> 16);
	write_register(spi, LORA_FREQ_MID, fsf >> 8);
	write_register(spi, LORA_FREQ_LSB, fsf >> 0);
}

void lora_set_bandwidth(struct lora *lora, uint8_t bandwidth)
{
	struct spi *spi = &lora->spi;
	uint8_t reg = read_register(spi, LORA_MODEM_CONFIG1);
	reg &= 0x0F;
	reg |= bandwidth << 4;
	write_register(spi, LORA_MODEM_CONFIG1, reg);
}

uint8_t lora_get_bandwidth(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	return read_register(spi, LORA_MODEM_CONFIG1) >> 4;
}

void lora_set_coding_rate(struct lora *lora, uint8_t rate)
{
	struct spi *spi = &lora->spi;
	uint8_t reg = read_register(spi, LORA_MODEM_CONFIG1);
	reg &= 0xF1;
	reg |= rate << 1;
	write_register(spi, LORA_MODEM_CONFIG1, reg);
}

uint8_t lora_get_coding_rate(struct lora *lora)
{
	struct spi *spi = &lora->spi;
	return (read_register(spi, LORA_MODEM_CONFIG1) >> 1) & 0x7;
}

uint8_t lora_get_register(struct lora *lora, uint8_t address)
{
	struct spi *spi = &lora->spi;
	return read_register(spi, address);
}

/*
const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_HANDSHAKE =
{
	.uFuncSel	   = AM_HAL_PIN_10_GPIO,
	.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
	.eIntDir		= AM_HAL_GPIO_PIN_INTDIR_LO2HI,
	.eGPInput	   = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};*/
