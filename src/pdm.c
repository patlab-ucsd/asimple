// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023
// SPDX-FileCopyrightText: Ambiq Micro, Inc., 2023

#include <pdm.h>
#include <uart.h>

#include <arm_math.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <stdint.h>
#include <stdatomic.h>

struct pdm
{
	uint32_t g_ui32PDMDataBuffer1[PDM_SIZE];
    uint32_t g_ui32PDMDataBuffer2[PDM_SIZE];
    void *PDMHandle;
	atomic_uint refcount;
};

static struct pdm pdm;

static volatile bool g_bPDMDataReady = false;

am_hal_pdm_config_t g_sPdmConfig =
{
	.eClkDivider = AM_HAL_PDM_MCLKDIV_1,
	.eLeftGain = AM_HAL_PDM_GAIN_P405DB,
	.eRightGain = AM_HAL_PDM_GAIN_P405DB,
	.ui32DecimationRate = 48,
	.bHighPassEnable = 0,
	.ui32HighPassCutoff = 0xB,
	.ePDMClkSpeed = AM_HAL_PDM_CLK_750KHZ,
	.bInvertI2SBCLK = 0,
	.ePDMClkSource = AM_HAL_PDM_INTERNAL_CLK,
	.bPDMSampleDelay = 0,
	.bDataPacking = 1,
	.ePCMChannels = AM_HAL_PDM_CHANNEL_RIGHT,
	.ui32GainChangeDelay = 1,
	.bI2SEnable = 0,
	.bSoftMute = 0,
	.bLRSwap = 0,
};

struct pdm* pdm_get_instance(void)
{
	if (!pdm.PDMHandle)
	{
		// Initialize, power-up, and configure the PDM.
		am_hal_pdm_initialize(0, &pdm.PDMHandle);
		am_hal_pdm_power_control(pdm.PDMHandle, AM_HAL_PDM_POWER_ON, false);
		am_hal_pdm_configure(pdm.PDMHandle, &g_sPdmConfig);
		am_hal_pdm_enable(pdm.PDMHandle);

		// Configure the necessary pins.
		am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_DATA, g_AM_BSP_GPIO_MIC_DATA);
		am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_CLK, g_AM_BSP_GPIO_MIC_CLK);

		// Configure and enable PDM interrupts (set up to trigger on DMA
		// completion).
		am_hal_pdm_interrupt_enable(
			pdm.PDMHandle,
			(AM_HAL_PDM_INT_DERR
				| AM_HAL_PDM_INT_DCMP
				| AM_HAL_PDM_INT_UNDFL
				| AM_HAL_PDM_INT_OVF));

		am_hal_pdm_fifo_flush(pdm.PDMHandle);
		pdm_sleep(&pdm);
	}
	pdm.refcount++;
	return &pdm;
}

bool pdm_sleep(struct pdm *pdm)
{
	// Note that turning off the hardware resets registers, which is why we
	// request saving the state
	// Also, spinloop while the device is busy
	// Implementation based off spi.c
	NVIC_DisableIRQ(PDM_IRQn);
	int status;
	do
	{
		status = am_hal_pdm_power_control(pdm->PDMHandle, AM_HAL_SYSCTRL_DEEPSLEEP, true);
	} while (status == AM_HAL_STATUS_IN_USE);

	am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_DATA, g_AM_HAL_GPIO_DISABLE);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_CLK, g_AM_HAL_GPIO_DISABLE);
	return true;
}

bool pdm_enable(struct pdm *pdm)
{
	// This can fail if there is no saved state, which indicates we've never
	// gone asleep
	int status = am_hal_pdm_power_control(pdm->PDMHandle, AM_HAL_SYSCTRL_WAKE, true);
	if (status != AM_HAL_STATUS_SUCCESS)
	{
		return false;
	}

	am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_DATA, g_AM_BSP_GPIO_MIC_DATA);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_CLK, g_AM_BSP_GPIO_MIC_CLK);
	NVIC_EnableIRQ(PDM_IRQn);
	return true;
}

void pdm_disable(struct pdm *pdm)
{
	if (pdm->refcount)
	{
		if (!--(pdm->refcount))
		{
			NVIC_DisableIRQ(PDM_IRQn);
			am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_DATA, g_AM_HAL_GPIO_DISABLE);
			am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_CLK, g_AM_HAL_GPIO_DISABLE);
			am_hal_pdm_power_control(pdm->PDMHandle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
			am_hal_pdm_deinitialize(pdm->PDMHandle);
			memset(pdm, 0, sizeof(*pdm));
		}
	}
}

uint32_t* pdm_get_buffer1(struct pdm *pdm)
{
	return pdm->g_ui32PDMDataBuffer1;
}

uint32_t* pdm_get_buffer2(struct pdm *pdm)
{
	return pdm->g_ui32PDMDataBuffer2;
}

void pdm_flush(struct pdm *pdm)
{
	am_hal_pdm_fifo_flush(pdm->PDMHandle);
}

void pdm_data_get(struct pdm *pdm, uint32_t* g_ui32PDMDataBuffer)
{
	am_hal_interrupt_master_disable();
	g_bPDMDataReady = false;
	am_hal_interrupt_master_enable();

	// Configure DMA and target address.
	am_hal_pdm_transfer_t sTransfer;
	sTransfer.ui32TargetAddr = (uint32_t ) g_ui32PDMDataBuffer;
	sTransfer.ui32TotalCount = PDM_BYTES;

	// Start the data transfer.
	am_hal_pdm_dma_start(pdm->PDMHandle, &sTransfer);
}

void am_pdm0_isr(void)
{
	uint32_t ui32Status;

	// Read the interrupt status.
	am_hal_pdm_interrupt_status_get(pdm.PDMHandle, &ui32Status, true);
	am_hal_pdm_interrupt_clear(pdm.PDMHandle, ui32Status);

	if (ui32Status & AM_HAL_PDM_INT_DCMP)
	{
		g_bPDMDataReady = true;
	}
}

void pcm_print(struct uart *uart, uint32_t* g_ui32PDMDataBuffer)
{
	int16_t *pi16PDMData = (int16_t *) g_ui32PDMDataBuffer;

	// Convert the PDM samples to floats.
	for (uint32_t i = 0; i < PDM_SIZE; i++)
	{
		uint16_t data = pi16PDMData[i];
		for (size_t sent = 0; sent != 2;) {
			sent += uart_write(uart, (uint8_t *)&data + sent, 2 - sent);
		}
	}
}

bool isPDMDataReady(void)
{
	am_hal_interrupt_master_disable();
	bool ready = g_bPDMDataReady;
	am_hal_interrupt_master_enable();
	return ready;
}
