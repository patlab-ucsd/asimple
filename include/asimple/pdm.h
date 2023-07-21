// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023
// SPDX-FileCopyrightText: Ambiq Micro, Inc., 2023

#ifndef PDM_H_
#define PDM_H_

#include "am_bsp.h"
#include "am_util.h"
#include <uart.h>
#include <stdint.h>

#define PDM_SIZE                4096
#define PDM_BYTES               (PDM_SIZE * 2)

volatile bool g_bPDMDataReady = false;
uint32_t g_ui32PDMDataBuffer1[PDM_SIZE];
uint32_t g_ui32PDMDataBuffer2[PDM_SIZE];
uint32_t g_ui32SampleFreq;

void *PDMHandle;

/**
 * PDM initialization.
*/
void pdm_init(void);

/**
 * Gets 4096 btyes of DMA data from the microphone.
 * 
 * @param[in,out] g_ui32PDMDataBuffer buffer array to store the data.
*/
void pdm_data_get(uint32_t* g_ui32PDMDataBuffer);

/**
 * PDM interrupt handler.
*/
void am_pdm0_isr(void);

/**
 * Print the DMA data from the microhpone to UART.
 * 
 * @param[in] uart UART structure to sned the data to.
 * @param[in] g_ui32PDMDataBuffer buffer array of the data we're sending.
*/
void pcm_print(struct uart *uart, uint32_t* g_ui32PDMDataBuffer);

#endif//PDM_H_
