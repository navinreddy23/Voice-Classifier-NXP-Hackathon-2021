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

#include "audio.h"
#include "audio_classifier.h"

#include "timer.h"

#include "mcop_mgr_inc.h"
#include "CANOpen.h"
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

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
	InitializeHardware();

    PRINTF("\r\nInitialized hardware - AUDIO-DMA, CAN, TIMER, and UART\r\n");

    CANOpen_Init(NULL);

	AUDIO_Classifier_Init(NULL);

    PRINTF("\r\nStarting OS...\r\n");

    vTaskStartScheduler();

    while(1);

    return 0 ;
}

static void InitializeHardware(void)
{

    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    LIBCB_InitLeds();
    AUDIO_Init();
    TIMER_Init();
}

extern "C"
{
	void vApplicationStackOverflowHook (TaskHandle_t xTask, signed char *pcTaskName)
	{
		(void)xTask;
		(void)pcTaskName;
		while(1);
	}
}
