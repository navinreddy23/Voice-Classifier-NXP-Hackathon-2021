/*
 * Copyright 2018-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "board.h"
#include "stdbool.h"
#include "timer.h"
#include "fsl_qtmr.h"
#include "mcop_mgr_inc.h"

#if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__REDLIB__)
#include <time.h>
#else
#include <sys/time.h>
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Timer settings */
#define SYSTICK_PRESCALE 1U
#define TICK_PRIORITY 1U

/* The QTMR instance/channel used for board */
#define BOARD_QTMR_BASEADDR       TMR2
#define BOARD_FIRST_QTMR_CHANNEL  kQTMR_Channel_0
#define BOARD_SECOND_QTMR_CHANNEL kQTMR_Channel_1
#define QTMR_ClockCounterOutput   kQTMR_ClockCounter0Output

/* Interrupt number and interrupt handler for the QTMR instance used */
#define QTMR_IRQ_ID      TMR2_IRQn
#define QTMR_IRQ_HANDLER TMR2_IRQHandler

/* QTMR Clock source divider for Ipg clock source, the value of two macros below should be aligned. */
#define QTMR_PRIMARY_SOURCE       (kQTMR_ClockDivide_128)
#define QTMR_CLOCK_SOURCE_DIVIDER (128U)
/* The frequency of the source clock after divided. */
#define QTMR_SOURCE_CLOCK (CLOCK_GetFreq(kCLOCK_IpgClk) / QTMR_CLOCK_SOURCE_DIVIDER)
/*******************************************************************************
 * Variables
 ******************************************************************************/

volatile uint32_t msTicks;

/*******************************************************************************
 * Code
 ******************************************************************************/
void QTMR_IRQ_HANDLER(void)
{
	msTicks++;
	MCOHW_Tick();
    /* Clear interrupt flag.*/
    QTMR_ClearStatusFlags(BOARD_QTMR_BASEADDR, BOARD_SECOND_QTMR_CHANNEL, kQTMR_CompareFlag);

    SDK_ISR_EXIT_BARRIER;
}

void TIMER_Init()
{
	qtmr_config_t qtmrConfig;

    QTMR_GetDefaultConfig(&qtmrConfig);
    qtmrConfig.primarySource = QTMR_PRIMARY_SOURCE;

    QTMR_Init(BOARD_QTMR_BASEADDR, BOARD_SECOND_QTMR_CHANNEL, &qtmrConfig);

    /* Set timer period to be 1 millisecond */
    QTMR_SetTimerPeriod(BOARD_QTMR_BASEADDR, BOARD_SECOND_QTMR_CHANNEL, MSEC_TO_COUNT(1U, QTMR_SOURCE_CLOCK));

    /* Enable at the NVIC */
    EnableIRQ(QTMR_IRQ_ID);

    /* Enable timer compare interrupt */
    QTMR_EnableInterrupts(BOARD_QTMR_BASEADDR, BOARD_SECOND_QTMR_CHANNEL, kQTMR_CompareInterruptEnable);

    /* Start the second channel to count on rising edge of the primary source clock */
    QTMR_StartTimer(BOARD_QTMR_BASEADDR, BOARD_SECOND_QTMR_CHANNEL, kQTMR_PriSrcRiseEdge);
}

uint32_t TIMER_GetTimeInMs()
{
    return msTicks;
}

#if defined(__ARMCC_VERSION) || defined(__REDLIB__)

clock_t clock ()
{
    return ((uint64_t)TIMER_GetTimeInUS() * CLOCKS_PER_SEC) / 1000000;
}

#elif defined(__ICCARM__)

int timespec_get(struct timespec* ts, int base)
{
    int us = TIMER_GetTimeInUS();
    ts->tv_sec = us / 1000000;
    ts->tv_nsec = (us % 1000000) * 1000;
    return TIME_UTC ;
}

#else

int gettimeofday (struct timeval *__restrict __p,void *__restrict __tz){
    int us = TIMER_GetTimeInMs();
    __p->tv_sec = us / 1000000;
    __p->tv_usec = us % 1000000;
    return 0;
}

#endif
