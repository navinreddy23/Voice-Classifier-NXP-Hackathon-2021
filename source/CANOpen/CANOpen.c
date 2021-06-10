/*
 * CANOpen.cpp
 *
 *  Created on: 28 May 2021
 *      Author: navin
 */
#include <stdint.h>

#include "audio_classifier.h"
#include "mcop_mgr_inc.h"

#include "timer.h"
#include "CANOpen.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void TaskCANOpenManager(void* arg);
static void TaskCANOpenProcessPDO(void* arg);
static void Tick(void);
/*******************************************************************************
 * Variables
 ******************************************************************************/
extern QueueHandle_t qResults; //Defined in audio_classifier.cpp

/*******************************************************************************
 * Code
 ******************************************************************************/

void CANOpen_Init(void* arg)
{
	xTaskCreate(TaskCANOpenManager, "CANOpen Manager", 512, NULL, 2, NULL);
	xTaskCreate(TaskCANOpenProcessPDO, "CANOpen PDO Processor", 512, NULL, 0, NULL);

	TIMER_SetCallBack(Tick);
}

static void TaskCANOpenProcessPDO(void* arg)
{
	results_classifier_t finalResult = {0};
	uint8_t label;
	uint8_t accuracy;

	for(;;)
	{
		//Queue receive (Blocks until the results are sent)
		xQueueReceive(qResults, &finalResult, portMAX_DELAY);

		label = finalResult.label;
		accuracy = finalResult.accuracy;

		MCO_WriteProcessData(P600001_VC_Command, 1, &(label));

		MCO_WriteProcessData(P600002_VC_Accuracy, 1, &(accuracy));
	}
}

static void TaskCANOpenManager(void* arg)
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

		// Application is processed in TaskCANOpenProcessPDO()

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

static void Tick(void)
{
	MCOHW_Tick();
}
