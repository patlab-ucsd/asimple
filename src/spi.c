// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <spi.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Configuration structure for the IO Master.
// FIXME I don't like why this isn't const...
static am_hal_iom_config_t spi_config =
{
	.eInterfaceMode = AM_HAL_IOM_SPI_MODE,
	.ui32ClockFreq = AM_HAL_IOM_4MHZ,
	.eSpiMode = AM_HAL_IOM_SPI_MODE_0,
};

static int32_t select_clock(uint32_t clock)
{
	if (clock >= 48000000u)
		return AM_HAL_IOM_48MHZ;
	else if (clock >= 24000000u)
		return AM_HAL_IOM_24MHZ;
	else if (clock >= 16000000u)
		return AM_HAL_IOM_16MHZ;
	else if (clock >= 12000000u)
		return AM_HAL_IOM_12MHZ;
	else if (clock >= 8000000u)
		return AM_HAL_IOM_8MHZ;
	else if (clock >= 6000000u)
		return AM_HAL_IOM_6MHZ;
	else if (clock >= 4000000u)
		return AM_HAL_IOM_4MHZ;
	else if (clock >= 3000000u)
		return AM_HAL_IOM_3MHZ;
	else if (clock >= 2000000u)
		return AM_HAL_IOM_2MHZ;
	else if (clock >= 1500000u)
		return AM_HAL_IOM_1_5MHZ;
	else if (clock >= 1000000u)
		return AM_HAL_IOM_1MHZ;
	else if (clock >= 750000u)
		return AM_HAL_IOM_750KHZ;
	else if (clock >= 500000u)
		return AM_HAL_IOM_500KHZ;
	else if (clock >= 400000u)
		return AM_HAL_IOM_400KHZ;
	else if (clock >= 375000u)
		return AM_HAL_IOM_375KHZ;
	else if (clock >= 250000u)
		return AM_HAL_IOM_250KHZ;
	else if (clock >= 125000u)
		return AM_HAL_IOM_125KHZ;
	else if (clock >= 100000u)
		return AM_HAL_IOM_100KHZ;
	else if (clock >= 50000u)
		return AM_HAL_IOM_50KHZ;
	else // Any other clocks
		return AM_HAL_IOM_10KHZ;
}

static int convert_chip_select(enum spi_chip_select cs)
{
	switch (cs)
	{
		/*case SPI_CS_1:
			return AM_BSP_IOM0_CS1_CHNL;
		case SPI_CS_2:
			return AM_BSP_IOM0_CS2_CHNL;*///FIXME Not implemented in SDK for some reason
		case SPI_CS_3:
			return AM_BSP_IOM0_CS3_CHNL;
		case SPI_CS_0:
		default:
			return AM_BSP_IOM0_CS_CHNL;
	}
}

void spi_init(struct spi *spi, uint32_t iomModule, uint32_t clock)
{
	spi->iom_module = iomModule;
	// This just initializes the handle-- no hardware function
	am_hal_iom_initialize(iomModule, &spi->handle);
	// ... and here we turn on the hardware so we can modify settings
	am_hal_iom_power_ctrl(spi->handle, AM_HAL_SYSCTRL_WAKE, false);
	clock = select_clock(clock);
	spi_config.ui32ClockFreq = clock;
	am_hal_iom_configure(spi->handle, &spi_config);
	am_bsp_iom_pins_enable(iomModule, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_enable(spi->handle);
	spi_chip_select(spi, SPI_CS_0);
	spi_sleep(spi);
}

void spi_chip_select(struct spi *spi, enum spi_chip_select chip_select)
{
	spi->chip_select = convert_chip_select(chip_select);
}

void spi_destroy(struct spi *spi)
{
	am_hal_iom_disable(spi->handle);
	am_bsp_iom_pins_disable(spi->iom_module, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_power_ctrl(spi->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
	am_hal_iom_uninitialize(spi->handle);
	memset(spi, 0, sizeof(*spi));
}

bool spi_sleep(struct spi *spi)
{
	// Note that turning off the hardware resets registers, which is why we
	// request saving the state
	int status = am_hal_iom_power_ctrl(spi->handle, AM_HAL_SYSCTRL_DEEPSLEEP, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}
	am_bsp_iom_pins_disable(spi->iom_module, AM_HAL_IOM_SPI_MODE);
	return true;
}

bool spi_enable(struct spi *spi)
{
	// This can fail if there is no saved state, which indicates we've never
	// gone asleep
	int status = am_hal_iom_power_ctrl(spi->handle, AM_HAL_SYSCTRL_WAKE, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}
	am_bsp_iom_pins_enable(spi->iom_module, AM_HAL_IOM_SPI_MODE);
	return true;
}

void spi_cmd_read(
	struct spi *spi, uint32_t command, uint32_t *buffer, uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen = 1,
		.ui32Instr = command,
		.eDirection = AM_HAL_IOM_RX,
		.ui32NumBytes = size,
		.pui32RxBuffer = buffer,
		.bContinue = false,
		.ui8RepeatCount = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_cmd_write(
	struct spi *spi, uint32_t command, const uint32_t *buffer, uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 1,
		.ui32Instr	   = command,
		.eDirection	  = AM_HAL_IOM_TX,
		.ui32NumBytes	= size,
		// FIXME I really don't like how I need to strip const here...
		.pui32TxBuffer   = (uint32_t*)buffer,
		.bContinue	   = false,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_read(
	struct spi *spi, uint32_t *buffer, uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen = 0,
		.eDirection = AM_HAL_IOM_RX,
		.ui32NumBytes = size,
		.pui32RxBuffer = buffer,
		.bContinue = false,
		.ui8RepeatCount = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_write(
	struct spi *spi, const uint32_t *buffer, uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 0,
		.eDirection	  = AM_HAL_IOM_TX,
		.ui32NumBytes	= size,
		// FIXME I really don't like how I need to strip const here...
		.pui32TxBuffer   = (uint32_t*)buffer,
		.bContinue	   = false,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_read_continue(
	struct spi *spi, uint32_t *buffer, uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen = 0,
		.eDirection = AM_HAL_IOM_RX,
		.ui32NumBytes = size,
		.pui32RxBuffer = buffer,
		.bContinue = true,
		.ui8RepeatCount = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_write_continue(
	struct spi *spi, const uint32_t *buffer, uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 0,
		.eDirection	  = AM_HAL_IOM_TX,
		.ui32NumBytes	= size,
		// FIXME I really don't like how I need to strip const here...
		.pui32TxBuffer   = (uint32_t*)buffer,
		.bContinue	   = true,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_readwrite(
	struct spi *spi,
	uint32_t command,
	uint32_t *rx_buffer,
	const uint32_t *tx_buffer,
	uint32_t size)
{
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 1,
		.ui32Instr	   = command,
		.eDirection	  = AM_HAL_IOM_FULLDUPLEX,
		.ui32NumBytes	= size,
		// FIXME I really don't like how I need to strip const here...
		.pui32TxBuffer   = (uint32_t*)tx_buffer,
		.pui32RxBuffer = rx_buffer,
		.bContinue	   = false,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = spi->chip_select,
	};
	am_hal_iom_spi_blocking_fullduplex(spi->handle, &transaction);
}
