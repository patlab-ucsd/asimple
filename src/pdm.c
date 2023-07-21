// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023
// Copyright: Kristin Ebuengan, 2023
// Copyright: Melody Gill, 2023
// Copyright: Ambiq Micro, Inc., 2023

#include <pdm.h>
#include <uart.h>

#include <arm_math.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#define PDM_SIZE                4096
#define PDM_BYTES               (PDM_SIZE * 2)

volatile bool g_bPDMDataReady = false;
uint32_t g_ui32PDMDataBuffer1[PDM_SIZE];
uint32_t g_ui32PDMDataBuffer2[PDM_SIZE];

void *PDMHandle;

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

void pdm_init(void)
{
    // Initialize, power-up, and configure the PDM.
    am_hal_pdm_initialize(0, &PDMHandle);
    am_hal_pdm_power_control(PDMHandle, AM_HAL_PDM_POWER_ON, false);
    am_hal_pdm_configure(PDMHandle, &g_sPdmConfig);
    am_hal_pdm_enable(PDMHandle);

    // Configure the necessary pins.
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_DATA, g_AM_BSP_GPIO_MIC_DATA);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MIC_CLK, g_AM_BSP_GPIO_MIC_CLK);

    // Configure and enable PDM interrupts (set up to trigger on DMA
    // completion).
    am_hal_pdm_interrupt_enable(PDMHandle, (AM_HAL_PDM_INT_DERR
                                            | AM_HAL_PDM_INT_DCMP
                                            | AM_HAL_PDM_INT_UNDFL
                                            | AM_HAL_PDM_INT_OVF));

    NVIC_EnableIRQ(PDM_IRQn);

    // Start the data transfer
    am_hal_pdm_enable(PDMHandle);
    am_util_delay_ms(100);
    am_hal_pdm_fifo_flush(PDMHandle);
}

void pdm_data_get(uint32_t* g_ui32PDMDataBuffer)
{
    // Configure DMA and target address.
    am_hal_pdm_transfer_t sTransfer;
    sTransfer.ui32TargetAddr = (uint32_t ) g_ui32PDMDataBuffer;
    sTransfer.ui32TotalCount = PDM_BYTES;

    // Start the data transfer.
    am_hal_pdm_dma_start(PDMHandle, &sTransfer);
}

void am_pdm0_isr(void)
{
    uint32_t ui32Status;

    // Read the interrupt status.
    am_hal_pdm_interrupt_status_get(PDMHandle, &ui32Status, true);
    am_hal_pdm_interrupt_clear(PDMHandle, ui32Status);

    if (ui32Status & AM_HAL_PDM_INT_DCMP)
    {
        g_bPDMDataReady = true;
    }
}

void pcm_print(struct uart *uart, uint32_t* g_ui32PDMDataBuffer)
{
    float fMaxValue;
    uint32_t ui32MaxIndex;
    int16_t *pi16PDMData = (int16_t *) g_ui32PDMDataBuffer;
    uint32_t ui32LoudestFrequency;

    // Convert the PDM samples to floats.
    for (uint32_t i = 0; i < PDM_SIZE; i++)
    {
        uint16_t data = pi16PDMData[i];
        for (size_t sent = 0; sent != 2;) {
            sent += uart_write(uart, (uint8_t *)&data + sent, 2 - sent);
        }
    }
}