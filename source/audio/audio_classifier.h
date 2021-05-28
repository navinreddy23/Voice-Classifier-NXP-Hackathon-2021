/*
 * audio_classifier.h
 *
 *  Created on: 28 May 2021
 *      Author: navin
 */

#ifndef AUDIO_AUDIO_CLASSIFIER_H_
#define AUDIO_AUDIO_CLASSIFIER_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
typedef struct
{
	uint8_t label;
	uint8_t accuracy;
}results_classifier_t;

void AUDIO_Classifier_Init(void* arg);

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* AUDIO_AUDIO_CLASSIFIER_H_ */
