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
 * @file    Impulse_Accel.cpp
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1021.h"
#include "fsl_debug_console.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */

static float raw_features[] = {3.7230, -0.2179, 11.5233, 3.5745, -0.1209, 11.3880, 3.0490, 0.6752, 10.5429, 2.6528, 1.9381, 11.0600, 2.4900, 2.4792, 12.1111, 1.8280, 2.7545, 12.3265, 1.5431, 3.4572, 12.9155, 1.5060, 4.1899, 13.2471, 1.2151, 4.5023, 13.2124, 0.6189, 5.1044, 12.7072, 0.2921, 5.6288, 12.6055, 0.1293, 5.8215, 13.1657, -0.1939, 6.5206, 13.5392, -0.4812, 6.3805, 13.1130, -0.4106, 6.1423, 12.6450, -0.6752, 6.4057, 12.7048, -0.7757, 6.6798, 12.0656, -0.9218, 6.9719, 11.3832, -1.0475, 6.7924, 10.4256, -1.0654, 6.6307, 9.6965, -0.9924, 6.4727, 9.5026, -1.0247, 6.3829, 9.5972, -1.0175, 6.0645, 9.5337, -0.7853, 6.0765, 8.9423, -0.5507, 6.0621, 7.9883, -0.6321, 5.8335, 7.6064, -0.5614, 5.2014, 7.1216, -0.4896, 4.6962, 6.6559, -0.3783, 4.4652, 6.3889, -0.4872, 3.9433, 5.5067, -0.6859, 4.0318, 5.0841, -0.6859, 4.0737, 5.1104, -0.6369, 3.3962, 4.8794, -0.4836, 2.6157, 4.8064, -0.3160, 2.2697, 5.0673, -0.2394, 1.9441, 5.0506, 0.1209, 0.9517, 4.7740, 0.2885, 0.3316, 4.6017, 0.2897, -0.4465, 3.9313, 0.3388, -1.1073, 3.6057, 0.6273, -1.4413, 4.4939, 0.8164, -1.4090, 6.3542, 1.3587, -1.4952, 7.0820, 1.7549, -1.5981, 7.9978, 1.9369, -1.3934, 8.4671, 1.9668, -1.5443, 9.0297, 2.1620, -1.8387, 9.3733, 2.8048, -2.5223, 10.4770, 3.0155, -2.6013, 10.8601, 3.1795, -2.4684, 11.3545, 3.1783, -2.1620, 10.8242, 2.9568, -2.1308, 10.9367, 3.4943, -2.2302, 11.0840, 3.4812, -1.3803, 11.3246, 2.5750, -0.6393, 12.5708, 2.4098, -0.0646, 13.1011, 2.3535, 0.0359, 12.8736, 2.1500, 0.2023, 11.8776, 1.6915, 0.3508, 11.4275, 1.3336, 0.7542, 12.2787, 0.9182, 1.5227, 13.3561, 0.3256, 2.3703, 13.6350, -0.1125, 2.9389, 13.6948, -0.5052, 3.8319, 14.1761, -1.0211, 3.7517, 14.1126, -1.7945, 4.9165, 13.8612, -2.1500, 6.0166, 14.2204, -2.4265, 6.4141, 14.5448, -2.5845, 6.4512, 13.8445, -2.5486, 6.4033, 13.4698, -2.5762, 6.3386, 13.0352, -2.4840, 6.3063, 12.2068, -2.5845, 6.2824, 10.9630, -2.4708, 5.9292, 9.9000, -2.4457, 5.6946, 9.7779, -2.3762, 5.9987, 10.4064, -2.1859, 6.2082, 10.1801, -2.1368, 5.8371, 9.0956, -2.0830, 5.6503, 7.8757, -1.8771, 5.4073, 6.6966, -1.6017, 4.5490, 6.6355, -1.3539, 4.2413, 6.4416, -1.3647, 4.2222, 6.6631, -1.1995, 4.2653, 6.2668, -1.3012, 3.9552, 6.1136, -1.2773, 3.0730, 5.2349, -1.1516, 2.8060, 5.0314, -0.9266, 2.5893, 5.5402, -0.7338, 2.2422, 5.9687, -0.6476, 2.0542, 6.0585, -0.4226, 1.3910, 5.6695, -0.0239, 0.2454, 4.9931, 0.2346, -0.1233, 4.6232, 0.5758, -0.3005, 4.5478, 0.8248, -0.7973, 4.6938, 1.1097, -1.1169, 5.6276, 1.3467, -1.2522, 6.4476, 1.8364, -1.4102, 7.9895, 2.2685, -2.0207, 8.6107, 2.6360, -2.5127, 8.2516, 2.9006, -2.7078, 8.3809, 3.1735, -2.8431, 9.3158, 3.6332, -3.1568, 9.9311, 3.8511, -3.4213, 10.2568, 4.2701, -3.6823, 10.9619, 4.3323, -3.0993, 11.5448, 4.0749, -2.7210, 11.6083, 4.1264, -2.2589, 10.9535, 3.7122, -1.7418, 11.0014, 3.2226, -1.6257, 11.1187, 2.4146, -0.8715, 11.3353, 2.2877, -0.2610, 11.2504, 2.1129, 0.1245, 11.4048, 1.6807, 0.8823, 12.2583, 1.1420, 1.3779, 12.8078, 0.6285, 2.2960, 13.0843, 0.3579, 2.6552, 13.1418, 0.0180, 3.0694, 13.4387, -0.4262, 3.4093, 13.6230, -0.7901, 4.1922, 13.8600, -1.2821, 4.5921, 13.9055, -1.4916, 5.1918, 13.9079, -1.8016, 5.6539, 13.6853, -1.9154, 5.7090, 13.5811, -2.1943, 5.6204, 12.7132};

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("\r\nHello World - Impulse Accel project - Includes added\r\n");

    if ( ( sizeof(raw_features) / sizeof(float))  != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)
    {
    	PRINTF("The size of your 'features' array is not correct. Expected %d items, but had %u\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, ( sizeof(raw_features) / sizeof(float) ) );
    }
    else
    {
    	PRINTF("The size of your 'features' array is CORRECT. Expected %d items, and has %u\r\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, ( sizeof(raw_features) / sizeof(float) ) );
    }

    int size = ( sizeof(raw_features) / sizeof(float) ) ;

    ei_impulse_result_t result;

	signal_t signal;
	numpy::signal_from_buffer(&raw_features[0], size, &signal);

    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, true);

    PRINTF("run_classifier returned: %d\r\n", res);

	PRINTF("[");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
    {
    	PRINTF("%.5f", result.classification[ix].value);
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        printf(", ");
#else
        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1)
        {
        	PRINTF(", ");
        }
#endif
    }

    PRINTF("]\n");

    while(1)
    {
        __asm volatile ("nop");
    }

    return 0 ;
}
