// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include <spi.h>
#include <gpio.h>

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

static int convert_chip_select(uint8_t module, enum spi_chip_select cs)
{
	// FIXME what about modules other than 0?
	switch (module)
	{
	default:
	case 0:
		switch (cs)
		{
#ifdef AM_BSP_IOM0_CS1_CHNL
		case SPI_CS_1:
			return AM_BSP_IOM0_CS1_CHNL;
#endif//AM_BSP_IOM0_CS1_CHNL
#ifdef AM_BSP_IOM0_CS2_CHNL
		case SPI_CS_2:
			return AM_BSP_IOM0_CS2_CHNL;
#endif//AM_BSP_IOM0_CS2_CHNL
#ifdef AM_BSP_IOM0_CS3_CHNL
		case SPI_CS_3:
			return AM_BSP_IOM0_CS3_CHNL;
#endif//AM_BSP_IOM0_CS3_CHNL
		case SPI_CS_0:
		default:
			return AM_BSP_IOM0_CS_CHNL;
		}
	case 1:
		switch (cs)
		{
#ifdef AM_BSP_IOM1_CS1_CHNL
		case SPI_CS_1:
			return AM_BSP_IOM1_CS1_CHNL;
#endif//AM_BSP_IOM1_CS1_CHNL
#ifdef AM_BSP_IOM1_CS2_CHNL
		case SPI_CS_2:
			return AM_BSP_IOM1_CS2_CHNL;
#endif//AM_BSP_IOM1_CS2_CHNL
#ifdef AM_BSP_IOM1_CS3_CHNL
		case SPI_CS_3:
			return AM_BSP_IOM1_CS3_CHNL;
#endif//AM_BSP_IOM1_CS3_CHNL
		case SPI_CS_0:
		default:
			return AM_BSP_IOM1_CS_CHNL;
		}
	case 2:
		switch (cs)
		{
#ifdef AM_BSP_IOM2_CS1_CHNL
		case SPI_CS_1:
			return AM_BSP_IOM2_CS1_CHNL;
#endif//AM_BSP_IOM2_CS1_CHNL
#ifdef AM_BSP_IOM2_CS2_CHNL
		case SPI_CS_2:
			return AM_BSP_IOM2_CS2_CHNL;
#endif//AM_BSP_IOM2_CS2_CHNL
#ifdef AM_BSP_IOM2_CS3_CHNL
		case SPI_CS_3:
			return AM_BSP_IOM2_CS3_CHNL;
#endif//AM_BSP_IOM2_CS3_CHNL
		case SPI_CS_0:
		default:
			return AM_BSP_IOM2_CS_CHNL;
		}
	case 3:
		switch (cs)
		{
#ifdef AM_BSP_IOM3_CS1_CHNL
		case SPI_CS_1:
			return AM_BSP_IOM3_CS1_CHNL;
#endif//AM_BSP_IOM3_CS1_CHNL
#ifdef AM_BSP_IOM3_CS2_CHNL
		case SPI_CS_2:
			return AM_BSP_IOM3_CS2_CHNL;
#endif//AM_BSP_IOM3_CS2_CHNL
#ifdef AM_BSP_IOM3_CS3_CHNL
		case SPI_CS_3:
			return AM_BSP_IOM3_CS3_CHNL;
#endif//AM_BSP_IOM3_CS3_CHNL
		case SPI_CS_0:
		default:
			return AM_BSP_IOM3_CS_CHNL;
		}
	}
}

static int get_pin(uint32_t module, enum spi_chip_select chip_select)
{
	switch (module)
	{
	default:
	case 0:
		switch (chip_select)
		{
#ifdef AM_BSP_GPIO_IOM0_CS1
		case SPI_CS_1:
			return AM_BSP_GPIO_IOM0_CS1;
#endif//AM_BSP_GPIO_IOM0_CS1
#ifdef AM_BSP_GPIO_IOM0_CS2
		case SPI_CS_2:
			return AM_BSP_GPIO_IOM0_CS2;
#endif//AM_BSP_GPIO_IOM0_CS2
#ifdef AM_BSP_GPIO_IOM0_CS3
		case SPI_CS_3:
			return AM_BSP_GPIO_IOM0_CS3;
#endif//AM_BSP_GPIO_IOM0_CS
		case SPI_CS_0:
		default:
			return AM_BSP_GPIO_IOM0_CS;
		}
	case 1:
		switch (chip_select)
		{
#ifdef AM_BSP_GPIO_IOM1_CS1
		case SPI_CS_1:
			return AM_BSP_GPIO_IOM1_CS1;
#endif//AM_BSP_GPIO_IOM1_CS1
#ifdef AM_BSP_GPIO_IOM1_CS2
		case SPI_CS_2:
			return AM_BSP_GPIO_IOM1_CS2;
#endif//AM_BSP_GPIO_IOM1_CS2
#ifdef AM_BSP_GPIO_IOM1_CS3
		case SPI_CS_3:
			return AM_BSP_GPIO_IOM1_CS3;
#endif//AM_BSP_GPIO_IOM1_CS
		case SPI_CS_0:
		default:
			return AM_BSP_GPIO_IOM1_CS;
		}
	case 2:
		switch (chip_select)
		{
#ifdef AM_BSP_GPIO_IOM2_CS1
		case SPI_CS_1:
			return AM_BSP_GPIO_IOM2_CS1;
#endif//AM_BSP_GPIO_IOM2_CS1
#ifdef AM_BSP_GPIO_IOM2_CS2
		case SPI_CS_2:
			return AM_BSP_GPIO_IOM2_CS2;
#endif//AM_BSP_GPIO_IOM2_CS2
#ifdef AM_BSP_GPIO_IOM2_CS3
		case SPI_CS_3:
			return AM_BSP_GPIO_IOM2_CS3;
#endif//AM_BSP_GPIO_IOM2_CS
		case SPI_CS_0:
		default:
			return AM_BSP_GPIO_IOM2_CS;
		}
	case 3:
		switch (chip_select)
		{
#ifdef AM_BSP_GPIO_IOM3_CS1
		case SPI_CS_1:
			return AM_BSP_GPIO_IOM3_CS1;
#endif//AM_BSP_GPIO_IOM3_CS1
#ifdef AM_BSP_GPIO_IOM3_CS2
		case SPI_CS_2:
			return AM_BSP_GPIO_IOM3_CS2;
#endif//AM_BSP_GPIO_IOM3_CS2
#ifdef AM_BSP_GPIO_IOM3_CS3
		case SPI_CS_3:
			return AM_BSP_GPIO_IOM3_CS3;
#endif//AM_BSP_GPIO_IOM3_CS
		case SPI_CS_0:
		default:
			return AM_BSP_GPIO_IOM3_CS;
		}
	}
}

void spi_bus_init(struct spi_bus *bus, uint32_t iomModule)
{
	bus->iom_module = iomModule;
	// This just initializes the handle-- no hardware function
	am_hal_iom_initialize(iomModule, &bus->handle);
	// ... and here we turn on the hardware so we can modify settings
	am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_WAKE, false);
	bus->current_clock = 2000000u;
	spi_config.ui32ClockFreq = bus->current_clock;
	am_hal_iom_configure(bus->handle, &spi_config);
	am_bsp_iom_pins_enable(iomModule, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_enable(bus->handle);
	spi_bus_sleep(bus);
}

void spi_bus_init_device(struct spi_bus *bus, struct spi_device *device, enum spi_chip_select chip_select, uint32_t clock)
{
	device->parent = bus;
	device->chip_select = convert_chip_select(bus->iom_module, chip_select);
	device->clock = select_clock(clock);
}

static void spi_device_update_clock(struct spi_device *device)
{
	if (device->parent->current_clock == device->clock)
		return;

	spi_config.ui32ClockFreq = device->clock;
	device->parent->current_clock = device->clock;
	am_hal_iom_configure(device->parent->handle, &spi_config);
}

void spi_device_set_clock(struct spi_device *device, uint32_t clock)
{
	device->clock = select_clock(clock);
}

void spi_bus_destroy(struct spi_bus *bus)
{
	am_hal_iom_disable(bus->handle);
	am_bsp_iom_pins_disable(bus->iom_module, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
	am_hal_iom_uninitialize(bus->handle);
	memset(bus, 0, sizeof(*bus));
}

void spi_device_destroy(struct spi_device *device)
{
	memset(device, 0, sizeof(*device));
}

bool spi_bus_sleep(struct spi_bus *bus)
{
	// Note that turning off the hardware resets registers, which is why we
	// request saving the state
	int status = am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_DEEPSLEEP, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}
	am_bsp_iom_pins_disable(bus->iom_module, AM_HAL_IOM_SPI_MODE);
	return true;
}

bool spi_bus_enable(struct spi_bus *bus)
{
	// This can fail if there is no saved state, which indicates we've never
	// gone asleep
	int status = am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_WAKE, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}
	am_bsp_iom_pins_enable(bus->iom_module, AM_HAL_IOM_SPI_MODE);
	return true;
}

void spi_device_cmd_read(
	struct spi_device *device, uint8_t command, uint32_t *buffer, uint32_t size)
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
}

void spi_device_cmd_write(
	struct spi_device *device, uint8_t command, const uint32_t *buffer, uint32_t size)
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
}

void spi_device_read(
	struct spi_device *device, uint32_t *buffer, uint32_t size)
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
}

void spi_device_write(
	struct spi_device *device, const uint32_t *buffer, uint32_t size)
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
}

void spi_device_read_continue(
	struct spi_device *device, uint32_t *buffer, uint32_t size)
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
}

void spi_device_write_continue(
	struct spi_device *device, const uint32_t *buffer, uint32_t size)
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
}

void spi_device_readwrite(
	struct spi_device *device,
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

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_spi_blocking_fullduplex(device->parent->handle, &transaction);
}

void spi_device_toggle(struct spi_device *device, uint32_t size)
{
	// We need this for SD card support
	// and this is cursed-- we need to take over the SPI pins for the nCS
	// lines, keep it high ourselves, then clock size number of bytes
	struct gpio cs;
	gpio_init(&cs, get_pin(device->parent->iom_module, device->chip_select), GPIO_MODE_OUTPUT, 1);
	uint32_t data = 0xFFFFFFFFu;
	spi_device_update_clock(device);
	for (; size > 4; size -= 4)
		spi_device_write(device, &data, 4);
	spi_device_write(device, &data, size);
	// Restore pin assignments
	am_bsp_iom_pins_enable(device->parent->iom_module, AM_HAL_IOM_SPI_MODE);
}
