/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/


#ifndef _RF_AMIC_CTRL_API_H_
#define _RF_AMIC_CTRL_API_H_

#include "Driver/DrvRF_AMIC_A7125.h"

#define RF_PACKET_SIZE			64	// Default packet (payload) size

#define DISABLE_RF_INTR()   do{NVIC_DisableIRQ(EINT0_IRQn);}while(0)
#define ENABLE_RF_INTR()    do{GPIOB->u32ISRC = (1 << 8);NVIC_ClearPendingIRQ(EINT0_IRQn);NVIC_EnableIRQ(EINT0_IRQn);}while(0)


// 8byte header + 50 byte data + 6byte unused

#define RF_PACKET_DATA_OFFSET   8			// audio data offset
#define RF_PACKET_DATA_SIZE    50

#define RF_MAX_DEVS	            1	// Maximum devices supported.
#define RF_MAX_CHS              16	// Number of candidated frequencis for hopping

/* Error codes */
#define ERR_RF_BUFFER_OVERFLOW		-15
#define ERR_RF_BUFFER_UNDERFLOW		-16
#define ERR_RF_ACK_WAITING			-17
#define ERR_RF_ACK_TIMEOUT			-18
#define ERR_RF_ACK_MISMATCH			-19


#ifdef __cplusplus
extern "C"
{
#endif

/* Exported control functions */
int32_t  RfInit(void);
uint32_t  RfBsMain(uint32_t *id);
void RfSetId(uint8_t id);

void RegisterDevId(uint8_t  u8DevId);
void RfTrigger(void);

uint8_t  MasterEvalRf(void);
void ShowTransRate(void);

#ifdef __cplusplus
}
#endif

#endif	// _RF_AMIC_CTRL_API_H_
