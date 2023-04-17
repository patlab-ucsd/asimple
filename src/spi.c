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

const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_HANDSHAKE =
{
	.uFuncSel	   = AM_HAL_PIN_10_GPIO,
	.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
	.eIntDir		= AM_HAL_GPIO_PIN_INTDIR_LO2HI,
	.eGPInput	   = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};

void spi_init(struct spi *spi, uint32_t iomModule)
{
	spi->iom_module = iomModule;
	am_hal_iom_initialize(iomModule, &spi->handle);
	am_hal_iom_power_ctrl(spi->handle, AM_HAL_SYSCTRL_WAKE, false);
	am_hal_iom_configure(spi->handle, &spi_config);
	am_bsp_iom_pins_enable(iomModule, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_enable(spi->handle);
}

void spi_destroy(struct spi *spi)
{
	am_bsp_iom_pins_disable(spi->iom_module, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_disable(spi->handle);
	memset(spi, 0, sizeof(*spi));
}

void spi_read(
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

		.uPeerInfo.ui32SpiChipSelect = AM_BSP_GPIO_IOM0_CS_CHNL,
	};
	am_hal_iom_blocking_transfer(spi->handle, &transaction);
}

void spi_write(
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

		.uPeerInfo.ui32SpiChipSelect = AM_BSP_GPIO_IOM0_CS_CHNL,
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

		.uPeerInfo.ui32SpiChipSelect = AM_BSP_GPIO_IOM0_CS_CHNL,
	};
	am_hal_iom_spi_blocking_fullduplex(spi->handle, &transaction);
}
