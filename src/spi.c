// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <spi.h>
#include <gpio.h>

#include <am_mcu_apollo.h>
#include <am_bsp.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

struct spi_device
{
	struct spi_bus *parent;
	int chip_select;
	unsigned clock;
};

struct spi_bus
{
	void *handle;
	int iom_module;
	unsigned current_clock;
	struct spi_device devices[4];
};

static struct spi_bus busses[3]; //FIXME how many busses do we have?

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

struct iom_pin
{
	int pin;
	const am_hal_gpio_pincfg_t *config;
};

struct iom_pins
{
	struct iom_pin clk;
	struct iom_pin miso;
	struct iom_pin mosi;
	struct iom_pin cs[4];
};

struct iom_pins iom_pins[4] =
{
	{
		.clk = { AM_BSP_GPIO_IOM0_SCK, &g_AM_BSP_GPIO_IOM0_SCK },
		.miso = { AM_BSP_GPIO_IOM0_MISO, &g_AM_BSP_GPIO_IOM0_MISO },
		.mosi = { AM_BSP_GPIO_IOM0_MOSI, &g_AM_BSP_GPIO_IOM0_MOSI },
		.cs = {
			{AM_BSP_GPIO_IOM0_CS, &g_AM_BSP_GPIO_IOM0_CS},
#ifdef AM_BSP_GPIO_IOM0_CS1
			{AM_BSP_GPIO_IOM0_CS1, &g_AM_BSP_GPIO_IOM0_CS1},
#endif
#ifdef AM_BSP_GPIO_IOM0_CS2
			{AM_BSP_GPIO_IOM0_CS2, &g_AM_BSP_GPIO_IOM0_CS2},
#endif
#ifdef AM_BSP_GPIO_IOM0_CS3
			{AM_BSP_GPIO_IOM0_CS3, &g_AM_BSP_GPIO_IOM0_CS3},
#endif
		},
	},
	{
		.clk = { AM_BSP_GPIO_IOM1_SCK, &g_AM_BSP_GPIO_IOM1_SCK },
		.miso = { AM_BSP_GPIO_IOM1_MISO, &g_AM_BSP_GPIO_IOM1_MISO },
		.mosi = { AM_BSP_GPIO_IOM1_MOSI, &g_AM_BSP_GPIO_IOM1_MOSI },
		.cs = {
			{AM_BSP_GPIO_IOM1_CS, &g_AM_BSP_GPIO_IOM1_CS},
#ifdef AM_BSP_GPIO_IOM1_CS1
			{AM_BSP_GPIO_IOM1_CS1, &g_AM_BSP_GPIO_IOM1_CS1},
#endif
#ifdef AM_BSP_GPIO_IOM1_CS2
			{AM_BSP_GPIO_IOM1_CS2, &g_AM_BSP_GPIO_IOM1_CS2},
#endif
#ifdef AM_BSP_GPIO_IOM1_CS3
			{AM_BSP_GPIO_IOM1_CS3, &g_AM_BSP_GPIO_IOM1_CS3},
#endif
		},
	},
	{
		.clk = { AM_BSP_GPIO_IOM2_SCK, &g_AM_BSP_GPIO_IOM2_SCK },
		.miso = { AM_BSP_GPIO_IOM2_MISO, &g_AM_BSP_GPIO_IOM2_MISO },
		.mosi = { AM_BSP_GPIO_IOM2_MOSI, &g_AM_BSP_GPIO_IOM2_MOSI },
		.cs = {
			{AM_BSP_GPIO_IOM2_CS, &g_AM_BSP_GPIO_IOM2_CS},
#ifdef AM_BSP_GPIO_IOM2_CS1
			{AM_BSP_GPIO_IOM2_CS1, &g_AM_BSP_GPIO_IOM2_CS1},
#endif
#ifdef AM_BSP_GPIO_IOM2_CS2
			{AM_BSP_GPIO_IOM2_CS2, &g_AM_BSP_GPIO_IOM2_CS2},
#endif
#ifdef AM_BSP_GPIO_IOM2_CS3
			{AM_BSP_GPIO_IOM2_CS3, &g_AM_BSP_GPIO_IOM2_CS3},
#endif
		},
	},
	{
		.clk = { AM_BSP_GPIO_IOM3_SCK, &g_AM_BSP_GPIO_IOM3_SCK },
		.miso = { AM_BSP_GPIO_IOM3_MISO, &g_AM_BSP_GPIO_IOM3_MISO },
		.mosi = { AM_BSP_GPIO_IOM3_MOSI, &g_AM_BSP_GPIO_IOM3_MOSI },
		.cs = {
			{AM_BSP_GPIO_IOM3_CS, &g_AM_BSP_GPIO_IOM3_CS},
#ifdef AM_BSP_GPIO_IOM3_CS1
			{AM_BSP_GPIO_IOM3_CS1, &g_AM_BSP_GPIO_IOM3_CS1},
#endif
#ifdef AM_BSP_GPIO_IOM3_CS2
			{AM_BSP_GPIO_IOM3_CS2, &g_AM_BSP_GPIO_IOM3_CS2},
#endif
#ifdef AM_BSP_GPIO_IOM3_CS3
			{AM_BSP_GPIO_IOM3_CS3, &g_AM_BSP_GPIO_IOM3_CS3},
#endif
		},
	},
};

struct spi_bus *spi_bus_get_instance(enum spi_bus_instance instance)
{
	struct spi_bus *bus = busses + (int)instance;
	if (!bus->handle)
	{
		bus->iom_module = (int)instance;
		// This just initializes the handle-- no hardware function
		am_hal_iom_initialize(bus->iom_module, &bus->handle);
		// ... and here we turn on the hardware so we can modify settings
		am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_WAKE, false);
		bus->current_clock = 2000000u;
		spi_config.ui32ClockFreq = bus->current_clock;
		am_hal_iom_configure(bus->handle, &spi_config);
		am_hal_iom_enable(bus->handle);
		// Don't bother enabling pins, sleep is going to disable them anyway
		spi_bus_sleep(bus);
	}
	return bus;
}

struct spi_device *spi_device_get_instance(
	struct spi_bus *bus, enum spi_chip_select instance, uint32_t clock)
{
	struct spi_device *device = &bus->devices[(int)instance];
	if (!device->parent)
	{
		device->parent = bus;
		device->chip_select = convert_chip_select(bus->iom_module, instance);
		device->clock = select_clock(clock);
	}
	return device;
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

void spi_bus_deinitialize(struct spi_bus *bus)
{
	am_hal_iom_disable(bus->handle);
	am_bsp_iom_pins_disable(bus->iom_module, AM_HAL_IOM_SPI_MODE);
	am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
	am_hal_iom_uninitialize(bus->handle);
	memset(bus, 0, sizeof(*bus));
}

void spi_device_deinitialize(struct spi_device *device)
{
	memset(device, 0, sizeof(*device));
}

bool spi_bus_sleep(struct spi_bus *bus)
{
	// Note that turning off the hardware resets registers, which is why we
	// request saving the state
	// Also, spinloop while the device is busy
	// Gabriel Marcano: I ran into a bug where for some gods forsaken reason
	// only on POR, in spi_bus_get_instance, calling spi_bus_sleep would fail,
	// and apparently it's because the IOM is in use? I'm not even sure why it
	// would be if I'm literally just turning it on for the first time. Maybe
	// there's something the bootrom is doing that is not well documented???
	int status;
	do
	{
		status = am_hal_iom_power_ctrl(bus->handle, AM_HAL_SYSCTRL_DEEPSLEEP, true);
	} while (status == AM_HAL_STATUS_IN_USE);

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
	struct spi_device *device, uint8_t command, uint8_t *buffer, uint32_t size)
{
	uint32_t *buf = malloc((size + 3)/4 * 4);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen = 1,
		.ui32Instr = command,
		.eDirection = AM_HAL_IOM_RX,
		.ui32NumBytes = size,
		.pui32RxBuffer = buf,
		.bContinue = false,
		.ui8RepeatCount = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
	memcpy(buffer, buf, size);
	free(buf);
}

void spi_device_cmd_write(
	struct spi_device *device, uint8_t command, const uint8_t *buffer, uint32_t size)
{
	uint32_t *buf = malloc(((size + 3) / 4) * 4);
	memcpy(buf, buffer, size);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 1,
		.ui32Instr	   = command,
		.eDirection	  = AM_HAL_IOM_TX,
		.ui32NumBytes	= size,
		.pui32TxBuffer   = buf,
		.bContinue	   = false,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
	free(buf);
}

void spi_device_read(
	struct spi_device *device, uint8_t *buffer, uint32_t size)
{
	uint32_t *buf = malloc(((size + 3) / 4) * 4);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen = 0,
		.eDirection = AM_HAL_IOM_RX,
		.ui32NumBytes = size,
		.pui32RxBuffer = buf,
		.bContinue = false,
		.ui8RepeatCount = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
	memcpy(buffer, buf, size);
	free(buf);
}

void spi_device_write(
	struct spi_device *device, const uint8_t *buffer, uint32_t size)
{
	uint32_t *buf = malloc(((size + 3) / 4) * 4);
	memcpy(buf, buffer, size);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 0,
		.eDirection	  = AM_HAL_IOM_TX,
		.ui32NumBytes	= size,
		.pui32TxBuffer   = buf,
		.bContinue	   = false,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
	free(buf);
}

void spi_device_read_continue(
	struct spi_device *device, uint8_t *buffer, uint32_t size)
{
	uint32_t *buf = malloc(((size + 3) / 4) * 4);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen = 0,
		.eDirection = AM_HAL_IOM_RX,
		.ui32NumBytes = size,
		.pui32RxBuffer = buf,
		.bContinue = true,
		.ui8RepeatCount = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
	memcpy(buffer, buf, size);
	free(buf);
}

void spi_device_write_continue(
	struct spi_device *device, const uint8_t *buffer, uint32_t size)
{
	uint32_t *buf = malloc(((size + 3)/4) * 4);
	memcpy(buf, buffer, size);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 0,
		.eDirection	  = AM_HAL_IOM_TX,
		.ui32NumBytes	= size,
		.pui32TxBuffer   = buf,
		.bContinue	   = true,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_blocking_transfer(device->parent->handle, &transaction);
	free(buf);
}

void spi_device_readwrite(
	struct spi_device *device,
	uint32_t command,
	uint8_t *rx_buffer,
	const uint8_t *tx_buffer,
	uint32_t size)
{
	uint32_t *bufr = malloc(((size + 3)/4) * 4);
	uint32_t *bufw = malloc(((size + 3)/4) * 4);
	memcpy(bufw, tx_buffer, size);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 1,
		.ui32Instr	   = command,
		.eDirection	  = AM_HAL_IOM_FULLDUPLEX,
		.ui32NumBytes	= size,
		// FIXME I really don't like how I need to strip const here...
		.pui32TxBuffer   = bufw,
		.pui32RxBuffer = bufr,
		.bContinue	   = false,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_spi_blocking_fullduplex(device->parent->handle, &transaction);
	memcpy(rx_buffer, bufr, size);
	free(bufr);
	free(bufw);
}

void spi_device_readwrite_continue(
	struct spi_device *device,
	uint8_t *rx_buffer,
	const uint8_t *tx_buffer,
	uint32_t size)
{
	uint32_t *bufr = malloc(((size + 3)/4) * 4);
	uint32_t *bufw = malloc(((size + 3)/4) * 4);
	memcpy(bufw, tx_buffer, size);
	am_hal_iom_transfer_t transaction =
	{
		.ui32InstrLen	= 0,
		.ui32Instr	   = 0,
		.eDirection	  = AM_HAL_IOM_FULLDUPLEX,
		.ui32NumBytes	= size,
		// FIXME I really don't like how I need to strip const here...
		.pui32TxBuffer   = bufw,
		.pui32RxBuffer = bufr,
		.bContinue	   = true,
		.ui8RepeatCount  = 0,
		.ui32PauseCondition = 0,
		.ui32StatusSetClr = 0,

		.uPeerInfo.ui32SpiChipSelect = device->chip_select,
	};
	spi_device_update_clock(device);
	am_hal_iom_spi_blocking_fullduplex(device->parent->handle, &transaction);
	memcpy(rx_buffer, bufr, size);
	free(bufr);
	free(bufw);
}

void spi_device_toggle(struct spi_device *device, uint32_t size)
{
	// We need this for SD card support
	// and this is cursed-- we need to take over the SPI pins for the nCS
	// lines, keep it high ourselves, then clock size number of bytes
	struct gpio cs;
	const struct iom_pin *cs_pin =
		&iom_pins[device->parent->iom_module].cs[device->chip_select];
	gpio_init(&cs, cs_pin->pin, GPIO_MODE_OUTPUT, 1);
	uint8_t data[4] = {
		0xFFu, 0xFFu, 0xFFu, 0xFFu
	};
	spi_device_update_clock(device);
	for (; size > 4; size -= 4)
		spi_device_write(device, data, 4);
	spi_device_write(device, data, size);
	// Restore pin assignments
	am_hal_gpio_pinconfig(cs_pin->pin, *cs_pin->config);
}

void spi_device_hold_mosi(struct spi_device *device, bool level)
{
	struct gpio mosi;
	int pin = iom_pins[device->parent->iom_module].mosi.pin;
	gpio_init(&mosi, pin, GPIO_MODE_OUTPUT, level);
}

void spi_device_release_mosi(struct spi_device *device)
{
	const struct iom_pin *pin = &iom_pins[device->parent->iom_module].mosi;
	am_hal_gpio_pinconfig(pin->pin, *pin->config);
}
