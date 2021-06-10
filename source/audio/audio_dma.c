/*
 * audio.c
 *
 *  Created on: May 23, 2021
 *      Author: navin
 */
#include "audio_dma.h"
#include "board.h"

#include "fsl_codec_common.h"
#include "fsl_wm8960.h"
#include "fsl_codec_adapter.h"

#include "fsl_sai.h"
#include "fsl_sai_edma.h"
#include "fsl_dmamux.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* SAI instance and clock */
#define AUDIO_CODEC_WM8960
#define AUDIO_SAI_INSTANCE	   SAI1
#define AUDIO_SAI_CHANNEL      (0)
#define AUDIO_SAI_IRQ          SAI1_IRQn
#define AUDIO_SAITxIRQHandler  SAI1_IRQHandler
#define AUDIO_SAI_TX_SYNC_MODE kSAI_ModeAsync
#define AUDIO_SAI_RX_SYNC_MODE kSAI_ModeSync
#define AUDIO_SAI_MCLK_OUTPUT  true
#define AUDIO_SAI_MASTER_SLAVE kSAI_Master

#define AUDIO_DATA_CHANNEL (1U)
#define AUDIO_BIT_WIDTH    kSAI_WordWidth16bits
#define AUDIO_SAMPLE_RATE  (kSAI_SampleRate16KHz)
#define AUDIO_MASTER_CLOCK AUDIO_SAI_CLK_FREQ

/* IRQ */
#define AUDIO_SAI_TX_IRQ SAI1_IRQn
#define AUDIO_SAI_RX_IRQ SAI1_IRQn

/* DMA */
#define AUDIO_DMA             DMA0
#define AUDIO_DMAMUX          DMAMUX
#define AUDIO_TX_EDMA_CHANNEL (0U)
#define AUDIO_RX_EDMA_CHANNEL (1U)
#define AUDIO_SAI_TX_SOURCE   kDmaRequestMuxSai1Tx
#define AUDIO_SAI_RX_SOURCE   kDmaRequestMuxSai1Rx

/* Select Audio/Video PLL (786.48 MHz) as sai1 clock source */
#define AUDIO_SAI1_CLOCK_SOURCE_SELECT (2U)
/* Clock pre divider for sai1 clock source */
#define AUDIO_SAI1_CLOCK_SOURCE_PRE_DIVIDER (0U)
/* Clock divider for sai1 clock source */
#define AUDIO_SAI1_CLOCK_SOURCE_DIVIDER (63U)
/* Get frequency of sai1 clock */
#define AUDIO_SAI_CLK_FREQ                                                        \
		(CLOCK_GetFreq(kCLOCK_AudioPllClk) / (AUDIO_SAI1_CLOCK_SOURCE_DIVIDER + 1U) / \
				(AUDIO_SAI1_CLOCK_SOURCE_PRE_DIVIDER + 1U))

/* I2C instance and clock */
#define AUDIO_I2C_INSTANCE LPI2C1

/* Select USB1 PLL (480 MHz) as master lpi2c clock source */
#define AUDIO_LPI2C_CLOCK_SOURCE_SELECT (0U)
/* Clock divider for master lpi2c clock source */
#define AUDIO_LPI2C_CLOCK_SOURCE_DIVIDER (5U)
/* Get frequency of lpi2c clock */
#define AUDIO_I2C_CLK_FREQ ((CLOCK_GetFreq(kCLOCK_Usb1PllClk) / 8) / (AUDIO_LPI2C_CLOCK_SOURCE_DIVIDER + 1U))

#define BOARD_MASTER_CLOCK_CONFIG()

#define AUDIO_SAMPLES 			16U
#define AUDIO_RESOLUTION	  	2U
#define AUDIO_NUM	  			(AUDIO_SAMPLES  *AUDIO_RESOLUTION)
#define BUFFER_SIZE   			(1000U)
#define BUFFER_NUMBER 			(2U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void BOARD_EnableSaiMclkOutput(bool enable);
static void RxCallbackFromISR(I2S_Type *base, sai_edma_handle_t *handle,
		status_t status, void *userData);
static void TxCallbackFromISR(I2S_Type *base, sai_edma_handle_t *handle,
		status_t status, void *userData);

/*******************************************************************************
 * Variables
 ******************************************************************************/
wm8960_config_t wm8960Config =
{
		.i2cConfig = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE, .codecI2CSourceClock = BOARD_CODEC_I2C_CLOCK_FREQ},
		.route     = kWM8960_RoutePlaybackandRecord,
		.rightInputSource = kWM8960_InputDifferentialMicInput2,
		.playSource       = kWM8960_PlaySourceDAC,
		.slaveAddress     = WM8960_I2C_ADDR,
		.bus              = kWM8960_BusI2S,
		.format = {.mclk_HZ = 6144000U, .sampleRate = kWM8960_AudioSampleRate16KHz, .bitWidth = kWM8960_AudioBitWidth16bit},
		.master_slave = false,
};

codec_config_t boardCodecConfig = {.codecDevType = kCODEC_WM8960, .codecDevConfig = &wm8960Config};

const clock_audio_pll_config_t audioPllConfig =
{
		.loopDivider = 32,  /* PLL loop divider. Valid range for DIV_SELECT divider value: 27~54. */
		.postDivider = 1,   /* Divider after the PLL, should only be 1, 2, 4, 8, 16. */
		.numerator   = 77,  /* 30 bit numerator of fractional loop divider. */
		.denominator = 100, /* 30 bit denominator of fractional loop divider */
};

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t Buffer[AUDIO_NUM * BUFFER_NUMBER * BUFFER_SIZE], 4);

AT_NONCACHEABLE_SECTION_INIT(sai_edma_handle_t txHandle);
AT_NONCACHEABLE_SECTION_INIT(sai_edma_handle_t rxHandle);
edma_handle_t dmaTxHandle = {0}, dmaRxHandle = {0};
edma_config_t dmaConfig = {0};

volatile uint32_t emptyBlock = BUFFER_NUMBER;
extern codec_config_t boardCodecConfig;
codec_handle_t codecHandle;

static sai_transfer_t xfer;
static sai_transceiver_t saiConfig;

static cb_rx_handle_t cbFuncOnRxCplt;
static uint8_t bufIndex = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void BOARD_EnableSaiMclkOutput(bool enable)
{
	if (enable)
	{
		IOMUXC_GPR->GPR1 |= IOMUXC_GPR_GPR1_SAI1_MCLK_DIR_MASK;
	}
	else
	{
		IOMUXC_GPR->GPR1 &= (~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR_MASK);
	}
}

static void RxCallbackFromISR(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
	if (kStatus_SAI_RxError == status)
	{
		/* Handle the error. */
	}

	if(cbFuncOnRxCplt != NULL)
	{
		cbFuncOnRxCplt(Buffer + (bufIndex * AUDIO_NUM * BUFFER_SIZE));
	}

	if(++bufIndex == BUFFER_NUMBER)
	{
		bufIndex = 0;
	}
}

static void TxCallbackFromISR(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData)
{
	;
}

void AUDIO_DMA_Init(void)
{
	CLOCK_InitAudioPll(&audioPllConfig);

	/*Clock setting for LPI2C*/
	CLOCK_SetMux(kCLOCK_Lpi2cMux, AUDIO_LPI2C_CLOCK_SOURCE_SELECT);
	CLOCK_SetDiv(kCLOCK_Lpi2cDiv, AUDIO_LPI2C_CLOCK_SOURCE_DIVIDER);

	/*Clock setting for SAI1*/
	CLOCK_SetMux(kCLOCK_Sai1Mux, AUDIO_SAI1_CLOCK_SOURCE_SELECT);
	CLOCK_SetDiv(kCLOCK_Sai1PreDiv, AUDIO_SAI1_CLOCK_SOURCE_PRE_DIVIDER);
	CLOCK_SetDiv(kCLOCK_Sai1Div, AUDIO_SAI1_CLOCK_SOURCE_DIVIDER);

	/*Enable MCLK clock*/
	BOARD_EnableSaiMclkOutput(true);

	/* Init DMAMUX */
	DMAMUX_Init(AUDIO_DMAMUX);
	DMAMUX_SetSource(AUDIO_DMAMUX, AUDIO_TX_EDMA_CHANNEL, (uint8_t)AUDIO_SAI_TX_SOURCE);
	DMAMUX_EnableChannel(AUDIO_DMAMUX, AUDIO_TX_EDMA_CHANNEL);
	DMAMUX_SetSource(AUDIO_DMAMUX, AUDIO_RX_EDMA_CHANNEL, (uint8_t)AUDIO_SAI_RX_SOURCE);
	DMAMUX_EnableChannel(AUDIO_DMAMUX, AUDIO_RX_EDMA_CHANNEL);

	/* Init DMA and create handle for DMA */
	EDMA_GetDefaultConfig(&dmaConfig);
	EDMA_Init(AUDIO_DMA, &dmaConfig);
	EDMA_CreateHandle(&dmaTxHandle, AUDIO_DMA, AUDIO_TX_EDMA_CHANNEL);
	EDMA_CreateHandle(&dmaRxHandle, AUDIO_DMA, AUDIO_RX_EDMA_CHANNEL);
#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
	EDMA_SetChannelMux(AUDIO_DMA, AUDIO_TX_EDMA_CHANNEL, DEMO_SAI_TX_EDMA_CHANNEL);
	EDMA_SetChannelMux(AUDIO_DMA, AUDIO_RX_EDMA_CHANNEL, DEMO_SAI_RX_EDMA_CHANNEL);
#endif

	/* SAI init */
	SAI_Init(AUDIO_SAI_INSTANCE);

	SAI_TransferTxCreateHandleEDMA(AUDIO_SAI_INSTANCE, &txHandle, TxCallbackFromISR, NULL, &dmaTxHandle);
	SAI_TransferRxCreateHandleEDMA(AUDIO_SAI_INSTANCE, &rxHandle, RxCallbackFromISR, NULL, &dmaRxHandle);

	/* I2S mode configurations */
	SAI_GetClassicI2SConfig(&saiConfig, AUDIO_BIT_WIDTH, kSAI_Stereo, 1U << AUDIO_SAI_CHANNEL);
	saiConfig.syncMode    = AUDIO_SAI_TX_SYNC_MODE;
	saiConfig.masterSlave = AUDIO_SAI_MASTER_SLAVE;
	SAI_TransferTxSetConfigEDMA(AUDIO_SAI_INSTANCE, &txHandle, &saiConfig);
	saiConfig.syncMode = AUDIO_SAI_RX_SYNC_MODE;
	SAI_TransferRxSetConfigEDMA(AUDIO_SAI_INSTANCE, &rxHandle, &saiConfig);

	/* set bit clock divider */
	SAI_TxSetBitClockRate(AUDIO_SAI_INSTANCE, AUDIO_MASTER_CLOCK, AUDIO_SAMPLE_RATE, AUDIO_BIT_WIDTH,
			AUDIO_DATA_CHANNEL);
	SAI_RxSetBitClockRate(AUDIO_SAI_INSTANCE, AUDIO_MASTER_CLOCK, AUDIO_SAMPLE_RATE, AUDIO_BIT_WIDTH,
			AUDIO_DATA_CHANNEL);

	/* master clock configurations */
	BOARD_MASTER_CLOCK_CONFIG();

	/* Use default setting to init codec */
	if (CODEC_Init(&codecHandle, &boardCodecConfig) != kStatus_Success)
	{
		assert(false);
	}
}

void AUDIO_DMA_Receive(void)
{
	xfer.data     = Buffer + (bufIndex * BUFFER_SIZE * AUDIO_NUM);
	xfer.dataSize = BUFFER_SIZE * AUDIO_NUM;
	if (kStatus_Success == SAI_TransferReceiveEDMA(AUDIO_SAI_INSTANCE, &rxHandle, &xfer))
	{
		//Handle error, if necessary.
	}
}

void AUDIO_DMA_Transfer(uint8_t* buffer)
{
	xfer.data     = buffer;
	xfer.dataSize = BUFFER_SIZE * AUDIO_NUM;
	SAI_TransferSendEDMA(AUDIO_SAI_INSTANCE, &txHandle, &xfer);
}

void AUDIO_DMA_SetCallBack(cb_rx_handle_t cbFuncPtr)
{
	cbFuncOnRxCplt = (cb_rx_handle_t)cbFuncPtr;
}
