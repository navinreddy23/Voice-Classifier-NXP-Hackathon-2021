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
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "audio.h"
#include "timer.h"
#include "mcop_mgr_inc.h"
#include "FreeRTOS.h"
#include "task.h"

typedef struct
{
	uint8_t label;
	uint8_t accuracy;
}results_t;

static int16_t *gBuf = NULL;
static results_t finalResult;
static TaskHandle_t hClassifier;

extern "C" void vApplicationStackOverflowHook (TaskHandle_t xTask,
											   signed char *pcTaskName);
static void InitializeHardware(void);
static int AudioToSignal(size_t offset, size_t length, float *pOutBuffer);

static int AudioToSignal(size_t offset, size_t length, float *pOutBuffer)
{
    numpy::int16_to_float(&gBuf[offset], pOutBuffer, length);

    return 0;
}

static void CallbackOnRx(uint8* buffer)
{
	BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
	gBuf = (int16_t*)buffer;

	xTaskNotifyFromISR(hClassifier, 0, eNoAction, &pxHigherPriorityTaskWoken);
    //AUDIO_Transfer(buffer);
}

static void CANOpen_ProcessApp(void)
{
	uint8_t label = finalResult.label;
	uint8_t accuracy = finalResult.accuracy;

    MCO_WriteProcessData(P600001_VC_Command, 1, &(label));

    MCO_WriteProcessData(P600002_VC_Accuracy, 1, &(accuracy));

}

static uint8_t FilteResults(float* pKeyAccuracies, size_t size)
{
	uint8_t topMatch = 0;
	float highestAccuracy = 0.0;

	for(uint8_t i = 0; i < size; i++)
	{
		if(pKeyAccuracies[i] > highestAccuracy)
		{
			highestAccuracy = pKeyAccuracies[i];
			topMatch = i;
		}
	}

	finalResult.label = topMatch;
	finalResult.accuracy = highestAccuracy * 100;

	return topMatch;
}

void TaskAudioClassifier(void* arg)
{
	uint32_t notification;
	uint8_t topMatch;

	signal_t signal;
	EI_IMPULSE_ERROR ei_error;
	ei_impulse_result_t result = {0};

	float keyword_accuracy[EI_CLASSIFIER_LABEL_COUNT] = {0};

    AUDIO_SetCallBack(&CallbackOnRx);
    AUDIO_Receive();

	for(;;)
	{
		xTaskNotifyWait(0,0,&notification,portMAX_DELAY);

		AUDIO_Receive();

		signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
		signal.get_data = &AudioToSignal;

		ei_error = run_classifier(&signal, &result, false);
		if (ei_error != EI_IMPULSE_OK)
		{
			PRINTF("ERR: Failed to run classifier (%d)\n", ei_error);
		}

		for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
		{
			keyword_accuracy[ix] =  result.classification[ix].value;
		}

		topMatch = FilteResults(keyword_accuracy, EI_CLASSIFIER_LABEL_COUNT);

		PRINTF("Predictions ");
		PRINTF("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)-> ",
				result.timing.dsp, result.timing.classification, result.timing.anomaly);

		PRINTF("%s: %f\r\n", result.classification[topMatch].label, result.classification[topMatch].value);
	}
}

void TaskCANOpenManager(void* arg)
{
	uint8_t NMTReset = TRUE; // reset all slave nodes once
	uint16_t NMT_ResetDelay;    // delay before resetting slave nodes

	NMTReset = TRUE;
	NMT_ResetDelay = MCOHW_GetTime() + 500;

	 MCOUSER_ResetCommunication();

	for(;;)
	{
		// Operate on CANopen protocol stack, slave
		MCO_ProcessStack();
		// Operate on application
		CANOpen_ProcessApp();

		if (MY_NMT_STATE == NMTSTATE_OP)
		{
			MGR_ProcessMgr();
			USER_ProcessMgr();
		}

		if (NMTReset && MCOHW_IsTimeExpired(NMT_ResetDelay))
		{
			MGR_TransmitNMT(NMTMSG_RESETAPP, 0);
			NMTReset = FALSE; // only do it once
		}

		vTaskDelay(10);
	}
}

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
	InitializeHardware();


    PRINTF("\r\nInitialized hardware - AUDIO-DMA, CAN, TIMER, and UART\r\n");

    xTaskCreate(TaskAudioClassifier, "EI classifier", 512, NULL, 1, &hClassifier);
    xTaskCreate(TaskCANOpenManager, "CANOpen Manager", 512, NULL, 2, NULL);

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
