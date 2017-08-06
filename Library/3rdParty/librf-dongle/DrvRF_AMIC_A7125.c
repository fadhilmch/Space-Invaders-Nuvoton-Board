/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/*
 *  AMIC A7125 Driver
 *
 */

//#include "Driver/DrvAIC.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvSys.h"
#include "Driver/DrvRF_AMIC_A7125.h"

#define DEFAULT_R01   0x62  // Default value for R01 to elimate read back time
#define DEFAULT_R20   0x07	// Default for the write-only reg. ID code length is 4 bytes

// Initial table for AMIC A7125
static const uint8_t gA7125InitTable[57] = {
	0x00,					// R00, Reset/Mode
	DEFAULT_R01,			// R01, Mode Control
	0x00,					// R02, Calc
	RF_MAX_FIFO_SIZE - 1,	// R03, FIFO I
	0x00,					// R04, FIFO II
	0x00,					// R05, FIFO Data
	0x00,	// R06, ID Data
	0x00,	// R07, RC OSC I
	0x00,	// R08, RC OSC II
	0x00,	// R09, RC OSC III
	0x00,	// R0A, CKO,
	0x01,	// R0B, GIO1
	0x19,	// R0C, GIO2
	0x1F,	// R0D, Data Rate Clock
	0x50,	// R0E, PLL I
	0x0E,	// R0F, PLL II
	0x96,	// R10, PLL III
	0x00,	// R11, PLL IV
	0x04,	// R12, PLL V
	0x3C,	// R13, Channel Group I
	0x78,	// R14, Channel Group II
	0xD7,	// R15, TX I
	0x40,	// R16, TX II
	0x10,	// R17, Delay I, *** was 0x12
	0x61,	// R18, Delay II, *** was 0x41
	0x62,	// R19, RX
	0xA0,	// R1A, RX Gain I
	0x00,	// R1B, RX Gain II
	0x00,	// R1C, RX Gain III
	0xC2,	// R1D, RX Gain IV
	0x00,	// R1E, RSSI Threshold
	0xEA,				// R1F, ADC Control
	DEFAULT_R20,		// R20, Code I
	0x56,				// R21, Code II
	0x2A,	// R22, Code III
	0x06,	// R23, IF Calibration I
	0x00,	// R24, IF Calibration II
	0x05,	// R25, VCO current Calibration
	0x44,	// R26, VCO band Calibration I
	0x80,	// R27, VCO band Calibration II
	0x30,	// R28, VCO deviation Calibration I
	0x20,	// R29, VCO deviation Calibration II
	0x80,	// R2A, VCO deviation Calibration III
	0x00,	// R2B, VCO modulation
	0x7A,	// R2C, Battery detect, *** was 0x6A
#ifdef RF_4dB_MODULE
    0x3E,	// R2D, TX test, for 4dB module
#else    	
	0x2F,	// R2D, TX test, *** was 0x3F.
#endif
	0x47,	// R2E, Rx DEM test I
	0x80,	// R2F, Rx DEM test II
	0xF1,	// R30, Charge Pump Current I
	0x11,	// R31, Charge Pump Current II
	0x04,	// R32, Crystal test, *** was 0x05
	0x45,	// R33, PLL test
	0x18,	// R34, VCO test
	0x10,	// R35, RF Analog test
	0xFF,	// R36, IFAT
	0x37,	// R37, Channel Select
	0xFF	// R38, VRB
};

S_RF_DRV_DATA gsAmicRfData;			// Internal RF global variables

/*************************************/
/*                                   */
/* Internal functions to control SPI */
/*                                   */
/*************************************/

static void SetSpiGo(void)
{
	SPI0->CNTRL.GO_BUSY  = 1;
}

static void WaitSpiFinish(void)
{
    while(SPI0->CNTRL.GO_BUSY  == 1);
}

static void SetBurstTransfer(int32_t i32BurstCnt)
{
	SPI0->CNTRL.TX_NUM = (i32BurstCnt - 1);
}

static void SetSpiCsLow(void)
{
    SPI0->SSR.SSR = 1;
}

static void SetSpiCsHigh(void)
{
    SPI0->SSR.SSR = 0;
}

/*
 *  Write one byte data to SPI bus and wait until finish
 */
static void WriteSpi(uint32_t uData)
{
	SetBurstTransfer(1);
	SPI0->TX[0] = uData;
	SetSpiGo();
	WaitSpiFinish();
}

/*
 *  Read one byte data from SPI bus and return it
 */
static uint32_t ReadSpi(void)
{
	SetBurstTransfer(1);
	SetSpiGo();
	WaitSpiFinish();
	return(SPI0->RX[0]);
}

/*
 *  Send a "strobe" command to AMIC RF module
 */
static void SendStrobeCommand(uint8_t u8Cmd)
{
	SetSpiCsLow();
	WriteSpi(u8Cmd);   
	SetSpiCsHigh();
}

/*
 *  RF Calibration
 */
static int32_t ChannelGroupCalibration(uint8_t u8Channel)
{
	uint8_t t, /* vb,  */ vbcf, /* vcb, */  vccf;
	
	AMIC_A7125_WriteReg(RF_AMIC_REG_PLL1, u8Channel);
	AMIC_A7125_WriteReg(RF_AMIC_REG_CALIRATION, 0x1C);
	DrvSYS_Delay(2000);
	while(AMIC_A7125_ReadReg(RF_AMIC_REG_CALIRATION) & 0x1C);
	
	t = AMIC_A7125_ReadReg(RF_AMIC_REG_VCO_BANK_CALI1);
	//vb = t & 0x07;
	vbcf = (t >> 3) & 0x01;

	t = AMIC_A7125_ReadReg(RF_AMIC_REG_VCO_CURR_CALI);
	//vcb = t & 0x0f;
	vccf = (t >> 4) & 0x01;
	
	return (vbcf || vccf) ? (ERR_AMIC_CALIBRATION_FAIL) : (E_SUCCESS);
}

static int32_t DoCalibration(void)
{
	uint8_t t, /* fb, */ fbcf /* , fcd, dvt */;
	
	SendStrobeCommand(RF_AMIC_CMD_STANDBY);
	AMIC_A7125_WriteReg(RF_AMIC_REG_CALIRATION, 0x03);
	while(AMIC_A7125_ReadReg(RF_AMIC_REG_CALIRATION) & 0x03);
	SendStrobeCommand(RF_AMIC_CMD_PLL);
	
	if (ChannelGroupCalibration(30) || ChannelGroupCalibration(90) ||
		ChannelGroupCalibration(150))
		return ERR_AMIC_CALIBRATION_FAIL;
		
	SendStrobeCommand(RF_AMIC_CMD_STANDBY);
//	DrvSYS_RoughDelay(2000);
		
	t = AMIC_A7125_ReadReg(RF_AMIC_REG_IF_CALIBRATION1);
	//fb = t & 0x0f;
	fbcf = (t >> 4) & 0x01;

	t = AMIC_A7125_ReadReg(RF_AMIC_REG_IF_CALIBRATION2);
	//fcd = t & 0x1F;
	
	/* dvt = */ AMIC_A7125_ReadReg(RF_AMIC_REG_VCO_BANK_CALI2);
	
	return (fbcf) ? (ERR_AMIC_CALIBRATION_FAIL) : (E_SUCCESS);
}


/*********************/
/*                   */
/*   API functions   */
/*                   */
/*********************/

void AMIC_A7125_WriteReg(uint8_t u8Addr, uint8_t u8Data)
{
	SetBurstTransfer(2);
	SetSpiCsLow();
	
	SPI0->TX[0] = (uint32_t)(u8Addr & 0x3F);
	SPI0->TX[1] = (uint32_t)u8Data;

	SetSpiGo();
	WaitSpiFinish();
	SetSpiCsHigh();
}

uint8_t AMIC_A7125_ReadReg(uint8_t u8Addr)
{
    uint32_t uData;
    
    uData = (u8Addr & 0x7F) | 0x40;

	SetSpiCsLow();
	WriteSpi(uData);
	uData = ReadSpi();
	SetSpiCsHigh();
	
	return (uint8_t)uData;
}

void AMIC_A7125_WriteFifo(uint8_t * pBuffer, uint8_t nLen)
{
    uint32_t remainder, integer, i;

    integer  = nLen & (~0x03);
    remainder = nLen & 0x03;
		
	SetSpiCsLow();	
	WriteSpi(RF_AMIC_REG_FIFO_DATA);

	SetBurstTransfer(2);
	for (i = 0; i < integer; i+=2)
	{

		SPI0->TX[0] = pBuffer[i];
		SPI0->TX[1] = pBuffer[i + 1];

	   	SetSpiGo();
		WaitSpiFinish();
	}

    if (remainder != 0)
	{
		SPI0->TX[0] = pBuffer[i];

		SetBurstTransfer(1);
		SetSpiGo();
		WaitSpiFinish();	
	}	
	
	SetSpiCsHigh();
}

void AMIC_A7125_ReadFifo(uint8_t * pBuffer, uint8_t nLen)
{
    uint32_t remainder, integer, i;

	integer = nLen & (~0x03);
	remainder = nLen & 0x03;
	
	SetSpiCsLow();	
	WriteSpi(RF_AMIC_REG_FIFO_DATA | 0x40);

	SetBurstTransfer(2);
	for(i = 0; i < integer; i+=2)
	{
	    SetSpiGo();
	    WaitSpiFinish();
        pBuffer[i] = SPI0->RX[0];
        pBuffer[i+1] = SPI0->RX[1];
	}	
	
	if (remainder != 0)
	{
	    SetBurstTransfer(1);
	    SetSpiGo();
	    WaitSpiFinish();
	
	    pBuffer[i] = SPI0->RX[0];

	}

	SetSpiCsHigh();	
}

int32_t AMIC_A7125_Init(void)
{
	int i;

		
	gsAmicRfData.R20 = DEFAULT_R20;

#ifdef RF_ENABLE_EXT_PA
	DrvGPIO_Open(GPIOA, DRVGPIO_BIT4, IO_OUTPUT, IO_PULL_UP, IO_LOWDRV); // TX SW
    DrvGPIO_Open(GPIOA, DRVGPIO_BIT5, IO_OUTPUT, IO_PULL_UP, IO_LOWDRV); // RX SW
	RF_TRXSW_OFF();
#endif	

	// Init SPI0 for A7125

	DrvGPIO_InitFunction(E_FUNC_SPI0);
	//DrvGPIO_InitFunction(E_FUNC_SPI0);
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 8);
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	DrvSPI_DisableAutoSS(eDRVSPI_PORT0);   // Must manually control SS otherwise SS activate every 8 bits
	DrvSPI_SetClockFreq(eDRVSPI_PORT0, 8000000, 0);	
	//SPI0->DIVIDER.DIVIDER = 3;//1;	  PCLK = 32 MHz	// FIXME: in what clk rate we're running...??
	
	AMIC_A7125_Reset();
	DrvSYS_Delay(100000);

	
	// Configure all registers
	for(i = 1; i <= 0x04; i++)
		AMIC_A7125_WriteReg(i, gA7125InitTable[i]);
		
	for(i = 0x07; i<= 0x23; i++)
		AMIC_A7125_WriteReg(i, gA7125InitTable[i]);
	
	for(i = 0x25; i<= 0x38; i++)
		AMIC_A7125_WriteReg(i, gA7125InitTable[i]);

	// Set ID and read it back to see if the module works or not.
	AMIC_A7125_SetId(RF_DEFAULT_ID);
	if (AMIC_A7125_GetId() != RF_DEFAULT_ID)
		return ERR_AMIC_INIT_FAIL;
		
	if (DoCalibration())
		return ERR_AMIC_CALIBRATION_FAIL;

	AMIC_A7125_SetChannel(RF_DEFAULT_CHANNEL);
	//AMIC_A7125_OpCmd(RF_CMD_IDLE);  --?

	AMIC_A7125_ResetTxFifo();
	AMIC_A7125_ResetRxFifo();

	return E_SUCCESS;		  
}

void AMIC_A7125_Reset(void)
{
    AMIC_A7125_WriteReg(RF_AMIC_REG_MODE, 0);
}

void AMIC_A7125_ResetTxFifo(void)
{
	SendStrobeCommand(RF_AMIC_CMD_TX_FIFO_RESET);
}

void AMIC_A7125_ResetRxFifo(void)
{
	SendStrobeCommand(RF_AMIC_CMD_RX_FIFO_RESET);
}

void AMIC_A7125_SetPayloadSize(uint16_t u16Size)
{
	if (u16Size > RF_MAX_FIFO_SIZE)
		u16Size = RF_MAX_FIFO_SIZE;
	
	gsAmicRfData.u8PacketSize = u16Size;
	AMIC_A7125_WriteReg(RF_AMIC_REG_FIFO1, u16Size - 1);
}

uint16_t AMIC_A7125_GetPayloadSize(void)
{
	return gsAmicRfData.u8PacketSize;
}

void AMIC_A7125_SetId(uint32_t u32Id)
{
	SetSpiCsLow();
	WriteSpi(RF_AMIC_REG_ID_DATA);
	
	SetBurstTransfer(2);
	SPI0->TX[0] = (u32Id & 0xFF);
	SPI0->TX[1] = ((u32Id >> 8) & 0xFF);
	SetSpiGo();
	WaitSpiFinish();
	SPI0->TX[0] = ((u32Id >> 16) & 0xFF);
	SPI0->TX[1] = ((u32Id >> 24) & 0xFF);

	SetSpiGo();
	WaitSpiFinish();
	SetSpiCsHigh();
}

uint32_t AMIC_A7125_GetId(void)
{
	uint32_t i = 0;
	SetSpiCsLow();
	WriteSpi(RF_AMIC_REG_ID_DATA | 0x40);
	
	SetBurstTransfer(2);
	SetSpiGo();
	WaitSpiFinish();
	i |= SPI0->RX[0] | (SPI0->RX[1] << 8);

	SetSpiGo();
	WaitSpiFinish();
	i |= (SPI0->RX[0] << 16) | (SPI0->RX[1] << 24); 

	SetSpiCsHigh();	
	return (i);	
}

void AMIC_A7125_OpCmd(E_RF_OP_CMD eMode)
{
#ifdef RF_ENABLE_EXT_PA
	if (eMode == RF_CMD_TX)
	{
		RF_TXSW_ON();
	}
	else if (eMode == RF_CMD_RX)
	{
		RF_RXSW_ON();
	}
#endif

    if (eMode == RF_CMD_RX && gsAmicRfData.bEnableRssi)
        // The ADCM bit (bit 0 in R01) is auto-clear after evaluation is done.
	    AMIC_A7125_WriteReg(RF_AMIC_REG_MODE_CONTROL, DEFAULT_R01 | 0x01);
	
	SendStrobeCommand((uint8_t)eMode);
}

void AMIC_A7125_SetChannel(uint8_t u8Channel)
{
	if (gsAmicRfData.u8Channel != u8Channel)
	{
		gsAmicRfData.u8Channel = u8Channel;
		AMIC_A7125_WriteReg(RF_AMIC_REG_PLL1, /*gau8ChannelFreq[u8Channel]*/ u8Channel);
	}
}

uint8_t AMIC_A7125_GetChannel(void)
{
	return gsAmicRfData.u8Channel;
}

void AMIC_A7125_EnableCrc(void)
{
	gsAmicRfData.R20 |= 0x08;	// CRC select bit
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE1, gsAmicRfData.R20);
}

void AMIC_A7125_DisableCrc(void)
{
	gsAmicRfData.R20 &= ~0x08;	// CRC select bit
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE1, gsAmicRfData.R20);
}

void AMIC_A7125_EnableFec(void)
{
	gsAmicRfData.R20 |= 0x10;	// FEC select bit
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE1, gsAmicRfData.R20);
}

void AMIC_A7125_DisableFec(void)
{
	gsAmicRfData.R20 &= ~0x10;	// FEC select bit
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE1, gsAmicRfData.R20);
}

void AMIC_A7125_EnableEncryption(void)
{
	gsAmicRfData.R20 |= 0x20;	// Whitening select bit
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE1, gsAmicRfData.R20);
}

void AMIC_A7125_DisableEncryption(void)
{
	gsAmicRfData.R20 &= ~0x20;	// Whitening select bit
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE1, gsAmicRfData.R20);
}

void AMIC_A7125_SetEncryptionKey(uint32_t u32Key)
{
	// Only 7 bits are valid for AMIC spec.
	AMIC_A7125_WriteReg(RF_AMIC_REG_CODE3, (uint8_t)u32Key);
}

void AMIC_A7125_EnableRssi(void)
{
    gsAmicRfData.bEnableRssi = TRUE;
}

void AMIC_A7125_DisableRssi(void)
{
    gsAmicRfData.bEnableRssi = FALSE;
}

uint8_t AMIC_A7125_GetRssi(void)
{
    return AMIC_A7125_ReadReg(RF_AMIC_REG_RSSI);
}
