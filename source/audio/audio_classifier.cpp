/*
 * audio_classifier.c
 *
 *  Created on: 28 May 2021
 *      Author: navin
 */

#include "fsl_debug_console.h"

#include "audio.h"
#include "audio_classifier.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static int AudioToSignal(size_t offset, size_t length, float *pOutBuffer);
static void TaskAudioClassifier(void* arg);
static uint8_t FilteResults(float*, size_t, results_classifier_t*);
static void CallbackOnRx(uint8* buffer);
static void PrintResults(ei_impulse_result_t* pRes, uint8_t topMatch);
/*******************************************************************************
 * Variables
 ******************************************************************************/
static int16_t *pBuf = NULL;
static TaskHandle_t hClassifier;
QueueHandle_t qResults;

/*******************************************************************************
 * Code
*******************************************************************************/

void AUDIO_Classifier_Init(void* arg)
{
	xTaskCreate(TaskAudioClassifier, "EI Classifier", 512, NULL, 1, &hClassifier);
	qResults = xQueueCreate(2, sizeof( results_classifier_t ) );
}

static void TaskAudioClassifier(void* arg)
{
	uint32_t notification;
	uint8_t topMatch;

	signal_t signal;
	EI_IMPULSE_ERROR ei_error;
	ei_impulse_result_t result = {0};
	results_classifier_t finalResult;

	float keyword_accuracy[EI_CLASSIFIER_LABEL_COUNT] = {0};

    AUDIO_SetCallBack(&CallbackOnRx);
    AUDIO_Receive();

	signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
	signal.get_data = &AudioToSignal;

	for(;;)
	{
		xTaskNotifyWait(0,0,&notification,portMAX_DELAY);

		AUDIO_Receive();

		ei_error = run_classifier(&signal, &result, false);
		if (ei_error != EI_IMPULSE_OK)
		{
			PRINTF("ERR: Failed to run classifier (%d)\n", ei_error);
		}

		for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
		{
			keyword_accuracy[ix] =  result.classification[ix].value;
		}

		topMatch = FilteResults(keyword_accuracy, EI_CLASSIFIER_LABEL_COUNT, &finalResult);

		//Send results to CANOpen PDO task through the Queue.
		xQueueSend(qResults, &finalResult, 0);

		PrintResults(&result, topMatch);
	}
}

static uint8_t FilteResults(float* pKeyAccuracies, size_t size, results_classifier_t* pRes)
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

	pRes->label = topMatch;
	pRes->accuracy = highestAccuracy * 100;

	return topMatch;
}

static int AudioToSignal(size_t offset, size_t length, float *pOutBuffer)
{
    numpy::int16_to_float(&pBuf[offset], pOutBuffer, length);

    return 0;
}

static void CallbackOnRx(uint8* buffer)
{
	BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
	pBuf = (int16_t*)buffer;

	xTaskNotifyFromISR(hClassifier, 0, eNoAction, &pxHigherPriorityTaskWoken);
    //AUDIO_Transfer(buffer);
}

static void PrintResults(ei_impulse_result_t* pRes, uint8_t topMatch)
{

	PRINTF("Predictions ");
	PRINTF("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)-> ",
			pRes->timing.dsp, pRes->timing.classification, pRes->timing.anomaly);

	PRINTF("%s: %f\r\n", pRes->classification[topMatch].label, pRes->classification[topMatch].value);
}
