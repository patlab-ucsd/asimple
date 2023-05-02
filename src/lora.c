// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <lora.h>
#include <spi.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// FIXME overall error checking is missing

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

bool lora_init(struct lora *lora, uint32_t frequency)
{
	struct spi *spi = &lora->spi;
	spi_init(spi, 0);
	spi_enable(spi);

	// do not initialize if the version is unknown
	int version = read_register(spi, LORA_VERSION);
	if (version != 0x12)
	{
		return false;
	}

	// Can only switch between FSQ and LoRa while sleeping...
	lora_sleep(lora);

	// Enable Lora mode
	write_register(spi, LORA_OPMODE, read_register(spi, LORA_OPMODE) | 0x80);

	lora_set_frequency(lora, frequency);
	lora_set_lna(lora, LORA_LNA_G1, true);

	// Set pointers for FIFOs, RX and TX
	// FIXME these are hard-coded
	// FIXME do I care about them being separated?
	write_register(spi, LORA_FIFO_TX_BASE, 0x80);
	write_register(spi, LORA_FIFO_RX_BASE, 0x00);

	// Enable AGC
	write_register(spi, LORA_MODEM_CONFIG3, 0x04);

	// Gabriel Marcano: Note that the RFM95W module does not connect the RFO_*
	// pins, and only connects PA_BOOST. As such, for this board, high_power
	// must always be true.
	lora_set_transmit_level(lora, 2, true);

	return true;
}

void lora_destroy(struct lora *lora)
{
	spi_destroy(&lora->spi);
	memset(lora, 0, sizeof(*lora));
}

bool lora_set_transmit_level(struct lora *lora, int8_t dBm, bool high_power)
{
	uint8_t max_power;
	int8_t output_power;

	if (high_power)
	{
		// valid range from +2 to +17 dBm
		if ((dBm < 2) || (dBm > 17))
			return false;

		// Set max output power to 17 dBm (actually 16.8 dBm)
		// Pmax=10.8+0.6*MaxPower
		// MaxPower = (Pmax - 10.8) / 0.6
		max_power = 10;

		// Pout = 17 - (15 - OutputPower)
		// OutputPower = Pout - 2
		output_power = dBm - 2;
	}
	else
	{
		// valid range from -4 to 15 dBm
		if ((dBm < -4) || (dBm > 15))
			return false;
		// Delta between max output and current output can't be more than 13
		if (dBm < 0)
		{
			max_power = 0;
			output_power = dBm + 4;
		}
		else
		{
			max_power = 7; // Pmax = 15
			// Pout=Pmax-(15-OutputPower)
			// OutputPower = Pout + 15 - Pmax
			output_power = dBm;
		}
	}

	uint8_t pa_config = (high_power ? 1 : 0) << 7;
	pa_config |= (max_power & 0x3) << 4;
	pa_config |= (output_power & 0x0F);

	struct spi *spi = &lora->spi;
	write_register(spi, LORA_PA_CONFIG, pa_config);

	return true;
}

void lora_receive_packet(struct lora *lora, unsigned char buffer[], size_t buffer_size)
{
	// FIXME should we block until we receive something?
	// FIXME what if we don't receive something?
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

void lora_send_packet(struct lora *lora, const unsigned char buffer[], uint8_t buffer_size)
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

	while (!(read_register(spi, LORA_IRQ_FLAGS) & 0x08))
	{
		// FIXME is there an interrupt way to wait while going to sleep?
		// wait
	}
	tx_irq = read_register(spi, LORA_IRQ_FLAGS) & 0x08;
	write_register(spi, LORA_IRQ_FLAGS, tx_irq);
}

bool lora_set_spreading_factor(struct lora *lora, uint8_t spreading_factor)
{
	struct spi *spi = &lora->spi;
	if ((spreading_factor < 6) || (spreading_factor > 12))
		return false;

	// with spreading_factor 6, required:
	// - header must be implicit
	// - register 0x31 must be 0b101 for lower 3 bits
	// - 0x37 must be 0x0C
	if (spreading_factor == 6)
	{
		// FIXME change packet mode to implicit, and remember previous?
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
	return true;
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

void lora_set_lna(struct lora *lora, enum lora_lna_gain gain, bool high_current)
{
	struct spi *spi = &lora->spi;

	// configure LNA current
	uint8_t lna_config = read_register(spi, LORA_LNA);
	if (high_current)
	{
		lna_config |= 0x03u;
	}
	else
	{
		lna_config &= 0xFCu;
	}

	// Clear top 3 bits, LnaGain
	lna_config &= 0x1Fu;
	switch (gain)
	{
		case LORA_LNA_G1:
			lna_config |= (0x1 << 5);
			break;
		case LORA_LNA_G2:
			lna_config |= (0x2 << 5);
			break;
		case LORA_LNA_G3:
			lna_config |= (0x3 << 5);
			break;
		case LORA_LNA_G4:
			lna_config |= (0x4 << 5);
			break;
		case LORA_LNA_G5:
			lna_config |= (0x5 << 5);
			break;
		case LORA_LNA_G6:
			lna_config |= (0x6 << 5);
			break;
	}

	write_register(spi, LORA_LNA, lna_config);
}

void lora_set_frequency(struct lora *lora, uint32_t frequency)
{
	struct spi *spi = &lora->spi;

	if (frequency > 800000000)
	{
		uint8_t reg = read_register(spi, LORA_OPMODE);
		reg &= 0xF7;
		write_register(spi, LORA_OPMODE, reg);
	}
	else
	{
		uint8_t reg = read_register(spi, LORA_OPMODE);
		reg |= 0x08;
		write_register(spi, LORA_OPMODE, reg);
	}

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
