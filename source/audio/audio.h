/*
 * audio.h
 *
 *  Created on: May 23, 2021
 *      Author: navin
 */

#ifndef AUDIO_AUDIO_H_
#define AUDIO_AUDIO_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef void(*cb_rx_t)(uint8_t*);
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void AUDIO_Init(void);
void AUDIO_Receive(void);
void AUDIO_Transfer(uint8_t* buffer);
void AUDIO_SetCallBack(cb_rx_t);

#if defined(__cplusplus)
}
#endif /* __cplusplus */


#endif /* AUDIO_AUDIO_H_ */