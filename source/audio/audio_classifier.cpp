/*
 * audio_classifier.c
 *
 *  Created on: 28 May 2021
 *      Author: navin
 */
#include "fsl_debug_console.h"

#include "audio_dma.h"
#include "audio_classifier.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*******************************************************************************
 * Defines
 ******************************************************************************/
#define MEDIUM_PRIORITY	1
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static int AudioToSignal(size_t offset, size_t length, float *pOutBuffer);
static void TaskAudioClassifier(void* arg);
static uint8_t FilterResults(float*, size_t, results_classifier_t*);
static void CallbackOnRx(uint8* pBuffer);
static void PrintResults(ei_impulse_result_t* pRes, size_t topMatch);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static int16_t *pBuf = NULL;
static TaskHandle_t hTaskClassifier;
QueueHandle_t qResults;

/*******************************************************************************
 * Code
 *******************************************************************************/
/**
 * @brief Create Audio Classifier tasks and the result queue.
 */
void AUDIO_Classifier_Init(void* arg)
{
	xTaskCreate(TaskAudioClassifier, "Audio Classifier", 512, NULL, MEDIUM_PRIORITY, &hTaskClassifier);
	qResults = xQueueCreate(2, sizeof( results_classifier_t ) );
}

/**
 * @brief This task initiates audio recording, and waits for the notification.
 * 				Upon receiving the notification, it classifies the audio sample and
 * 				puts the results in a queue. These results are sent over the CANopen network.
 */
static void TaskAudioClassifier(void* arg)
{
	uint32_t notification;
	uint8_t topMatch;

	signal_t signal;
	EI_IMPULSE_ERROR ei_error;
	ei_impulse_result_t result = {0};
	results_classifier_t finalResult;

	float keyword_accuracy[EI_CLASSIFIER_LABEL_COUNT] = {0};

	AUDIO_DMA_SetCallBack(&CallbackOnRx);
	//Start receiving audio sample.
	AUDIO_DMA_Receive();

	signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
	signal.get_data = &AudioToSignal;

	for(;;)
	{
		xTaskNotifyWait(0, 0, &notification, portMAX_DELAY);

		AUDIO_DMA_Receive();

		ei_error = run_classifier(&signal, &result, false);
		if (ei_error != EI_IMPULSE_OK)
		{
			PRINTF("ERR: Failed to run classifier (%d)\n", ei_error);
		}

		for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
		{
			keyword_accuracy[ix] =  result.classification[ix].value;
		}

		topMatch = FilterResults(keyword_accuracy, EI_CLASSIFIER_LABEL_COUNT, &finalResult);

		//Send results to CANOpen PDO task through the Queue.
		xQueueSend(qResults, &finalResult, 0);

		PrintResults(&result, topMatch);
	}
}

/**
 * @brief Get the best results from an array.
 * @param pKeyAccuracies pointer to the array
 * @param len Length of the array
 * @param pRes Pointer to the results structure.
 */
static uint8_t FilterResults(float* pKeyAccuracies, size_t len, results_classifier_t* pRes)
{
	uint8_t topMatch = 0;
	float highestAccuracy = 0.0;

	for(size_t i = 0; i < len; i++)
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

/**
 * @brief This function converts the int16_t value to float.
 * 				It is necessary for Edge Impulse SDK as the run_classifier()
 * 				accepts in this format.
 * @param offset Internal to Edge Impulse SDK
 * @param length Internal to Edge Impulse SDK
 * @param pOutBuffer Pointer to output buffer. Also internal to EI SDK.
 */
static int AudioToSignal(size_t offset, size_t length, float *pOutBuffer)
{
	numpy::int16_to_float(&pBuf[offset], pOutBuffer, length);

	return 0;
}

/**
 * @brief This function is called when the audio is ready in the buffer.
 * 				Upon receiving the audio it notifies the Audio Classifier task to
 * 				wake-up and start classification.
 * @param pBuffer The pointer to the audio sample recorded.
 * @note It is possible to listen to the audio by uncommenting the last line.
 */
static void CallbackOnRx(uint8* pBuffer)
{
	BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
	pBuf = (int16_t*)pBuffer;

	xTaskNotifyFromISR(hTaskClassifier, 0, eNoAction, &pxHigherPriorityTaskWoken);
	//AUDIO_DMA_Transfer(pBuffer);
}

/**
 * @brief Formats and prints the results to the terminal.
 */
static void PrintResults(ei_impulse_result_t* pRes, size_t topMatch)
{
	PRINTF("\r\n----------------------------------------\r\n");
	PRINTF("       DSP: %d ms.\r\n", pRes->timing.dsp);
	PRINTF("       Classification: %d ms.\r\n", pRes->timing.classification);
	PRINTF("       Label: %s\r\n", pRes->classification[topMatch].label);
	PRINTF("       Accuracy: %0.2f %%\r\n", pRes->classification[topMatch].value*100);
	PRINTF("----------------------------------------\r\n\r\n");
}
