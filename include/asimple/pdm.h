// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023
// SPDX-FileCopyrightText: Kristin Ebuengan, 2023
// SPDX-FileCopyrightText: Melody Gill, 2023
/// @file

#ifndef PDM_H_
#define PDM_H_

#include "am_bsp.h"
#include "am_util.h"
#include <uart.h>
#include <stdint.h>

#define PDM_SIZE                4096
#define PDM_BYTES               (PDM_SIZE * 2)

/** Structure representing the microphone */
struct pdm
{
	uint32_t g_ui32PDMDataBuffer1[PDM_SIZE];
    uint32_t g_ui32PDMDataBuffer2[PDM_SIZE];
    void *PDMHandle;
};

/**
 * PDM initialization.
 *
 * @param[in] pdm PDM structure to initialize.
*/
void pdm_init(struct pdm *pdm);

/**
 * Get g_ui32PDMDataBuffer1 from the PDM struct.
 *
 * @param[in] pdm PDM structure to get from.
 *
 * @returns g_ui32PDMDataBuffer1 from the PDM struct.
*/
uint32_t* pdm_get_buffer1(struct pdm *pdm);

/**
 * Get g_ui32PDMDataBuffer2 from the PDM struct.
 *
 * @param[in] pdm PDM structure to get from.
 *
 * @returns g_ui32PDMDataBuffer2 from the PDM struct.
*/
uint32_t* pdm_get_buffer2(struct pdm *pdm);

/**
 * Calls PDM FIFO Flush.
 *
 * @param[in] pdm PDM structure to flush
*/
void pdm_flush(struct pdm *pdm);

/**
 * Gets 4096 btyes of DMA data from the microphone.
 *
 * @param[in] pdm PDM structure to get data from.
 * @param[in,out] g_ui32PDMDataBuffer buffer array to store the data.
*/
void pdm_data_get(struct pdm *pdm, uint32_t* g_ui32PDMDataBuffer);

/**
 * Print the DMA data from the microhpone to UART.
 *
 * @param[in] uart UART structure to sned the data to.
 * @param[in] g_ui32PDMDataBuffer buffer array of the data we're sending.
*/
void pcm_print(struct uart *uart, uint32_t* g_ui32PDMDataBuffer);

/**
 * Return whether PDM data is ready to be printed.
 *
 * @return g_bPDMDataReady
*/
bool isPDMDataReady(void);

#endif//PDM_H_
