/*
 * Copyright 2016-2021 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    main.cpp
 * @brief   Application entry point.
 */
#include "board.h"
#include "peripherals.h"
#include "fsl_debug_console.h"

#include "audio_dma.h"
#include "audio_classifier.h"
#include "timer.h"
#include "CANOpen.h"

#include "mcop_mgr_inc.h"
#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void InitializeHardware(void);
extern "C" void vApplicationStackOverflowHook (TaskHandle_t xTask,
				signed char *pcTaskName);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/***
 * @brief Application entry point
 * 				Initialises HW and Application tasks and Starts the OS
 */
int main(void)
{
	InitializeHardware();

	PRINTF("Core Clock frequency: %u MHz\r\n", SystemCoreClock/1000/1000);
	PRINTF("\r\nInitialized hardware - AUDIO-DMA, CAN, TIMER, and UART\r\n");

	//Create tasks and queues.
	CANOpen_Init(NULL);
	AUDIO_Classifier_Init(NULL);

	PRINTF("\r\nStarting OS...\r\n");

	vTaskStartScheduler();

	while(1);

	return 0;
}

/**
 * @brief Initialize all the peripherals that are necessary.
 * 				Baud rate for UART is set to 460800 bps.
 */
static void InitializeHardware(void)
{
	BOARD_ConfigMPU();
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();

	LIBCB_InitLeds();
	AUDIO_DMA_Init();
	TIMER_Init();
}

extern "C"
{
/**
 * @brief FreeRTOS hook function for catching stack overflow.
 */
void vApplicationStackOverflowHook (TaskHandle_t xTask, signed char *pcTaskName)
{
	(void)xTask;
	(void)pcTaskName;
	while(1);
}
}
