/*
 * Copyright 2018-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
typedef void(*cb_timer_handle_t)(void);

void TIMER_Init(void);
uint32_t TIMER_GetTimeInMs(void);
void TIMER_SetCallBack(cb_timer_handle_t funcPtr);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _TIMER_H_ */
