/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "USB.h"
#include "HIDSys.h"
#include "HID_API.h"


//#define DBG_PRINTF     printf

#define Maximum(a,b)		((a)>(b) ? (a) : (b))


S_HID_DEVICE g_HID_sDevice;

/* Endpoint description */
S_DRVUSB_EP_CTRL sEpDescription[] = 
{
	{CTRL_EP_NUM   | EP_INPUT , HID_MAX_PACKET_SIZE_CTRL   , NULL},   /* EP Id 0, EP Addr 0, input , max packe size = 64 */ 
	{CTRL_EP_NUM   | EP_OUTPUT, HID_MAX_PACKET_SIZE_CTRL   , NULL},   /* EP Id 1, EP Addr 0, output, max packe size = 64 */ 
	{INT_IN_EP_NUM | EP_INPUT , HID_MAX_PACKET_SIZE_INT_IN , NULL},   /* EP Id 2, EP Addr 1, input , max packe size =  8 */ 
	{0x00,  0, NULL}
};

/* bus event call back */
S_DRVUSB_EVENT_PROCESS g_sBusOps[6] = 
{
	{NULL, NULL},                               /* attach event callback */
	{NULL, NULL},                               /* detach event callback */
	{DrvUSB_BusResetCallback, &g_HID_sDevice},  /* bus reset event callback */
	{DrvUSB_BusSuspendCallback, &g_HID_sDevice},/* bus suspend event callback */
	{DrvUSB_BusResumeCallback, &g_HID_sDevice},                               /* bus resume event callback */
	{DrvUSB_CtrlSetupAck, &g_HID_sDevice},      /* setup event callback */
};

/* USB event call back */
S_DRVUSB_EVENT_PROCESS g_sUsbOps[12] = 
{
	{DrvUSB_CtrlDataInAck   , &g_HID_sDevice},/* ctrl pipe0 (EP address 0) In ACK callback */
	{DrvUSB_CtrlDataOutAck  , &g_HID_sDevice},/* ctrl pipe0 (EP address 0) Out ACK callback */
	{HID_IntInCallback      , &g_HID_sDevice},/* EP address 1 In ACK callback */
	{NULL, NULL},                             /* EP address 1 Out ACK callback */
	{NULL, NULL},                             /* EP address 2 In ACK callback */  
	{NULL, NULL},                             /* EP address 2 Out ACK callback */  
	{NULL, NULL},                             /* EP address 3 In ACK callback */  
	{NULL, NULL},                             /* EP address 3 Out ACK callback */  
	{NULL, NULL},                             /* EP address 4 In ACK callback */  
	{NULL, NULL},                             /* EP address 4 Out ACK callback */  
	{NULL, NULL},                             /* EP address 5 In ACK callback */  
	{NULL, NULL},                             /* EP address 5 Out ACK callback */  
};

/*ctrl pipe call back.                                                                  */
/*it will be call by DrvUSB_CtrlSetupAck, DrvUSB_CtrlDataInAck and DrvUSB_CtrlDataOutAck*/
/*if in ack handler and out ack handler is 0, default handler will be called            */
S_DRVUSB_CTRL_CALLBACK_ENTRY g_asCtrlCallbackEntry[] =
{   //request type,command     ,setup ack handler         , in ack handler      ,out ack handler,  parameter
	{REQ_STANDARD, SET_ADDRESS, DrvUSB_CtrlSetupSetAddress, DrvUSB_CtrlDataInSetAddress, 0, &g_HID_sDevice},
	{REQ_STANDARD, CLEAR_FEATURE, DrvUSB_CtrlSetupClearSetFeature, 0, 0, &g_HID_sDevice},
	{REQ_STANDARD, SET_FEATURE, DrvUSB_CtrlSetupClearSetFeature, 0, 0, &g_HID_sDevice},
	{REQ_STANDARD, GET_CONFIGURATION, DrvUSB_CtrlSetupGetConfiguration, 0, 0, &g_HID_sDevice},
	{REQ_STANDARD, GET_STATUS, DrvUSB_CtrlSetupGetStatus, 0, 0, &g_HID_sDevice},
	{REQ_STANDARD, GET_INTERFACE, DrvUSB_CtrlSetupGetInterface, 0, 0, &g_HID_sDevice},
	{REQ_STANDARD, SET_INTERFACE, DrvUSB_CtrlSetupSetInterface, 0, 0, &g_HID_sDevice},
	{REQ_STANDARD, GET_DESCRIPTOR, HID_CtrlSetupGetDescriptor, HID_CtrlGetDescriptorIn, 0/*HID_CtrlGetDescriptorOut*/, &g_HID_sDevice},
	{REQ_STANDARD, SET_CONFIGURATION, DrvUSB_CtrlSetupSetConfiguration, 0, 0, &g_HID_sDevice},
	/* To support boot protocol */
	{REQ_CLASS, SET_REPORT, HID_CtrlSetupSetReport, 0, 0, &g_HID_sDevice},
	{REQ_CLASS, SET_IDLE, HID_CtrlSetupSetIdle, 0, 0, &g_HID_sDevice},
	{REQ_CLASS, SET_PROTOCOL, HID_CtrlSetupSetProtocol, 0, 0, &g_HID_sDevice},
	{REQ_CLASS, GET_PROTOCOL, HID_CtrlSetupGetProtocol, 0, 0, &g_HID_sDevice},
};


static void HID_UsbStartCallBack(void * pVoid);
static int32_t HID_IsConfigureValue(uint8_t u8ConfigureValue);
static void HID_Reset(S_HID_DEVICE *psDevice);
static void HID_Start(S_HID_DEVICE *psDevice);

//register to USB driver
S_DRVUSB_CLASS sHidUsbClass = 
{
	(void *)&g_HID_sDevice, 
	HID_UsbStartCallBack,
	HID_IsConfigureValue
};


static const uint8_t * gpu8UsbBuf = 0;
static uint32_t gu32BytesInUsbBuf = 0;
static int8_t gIsOverRequest = 0;


//uint32_t HID_GetVersion(VOID)
//{
//	return HID_VERSION_NUM;
//}

/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      To initial the descriptors and install the handlers.             */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     not used now                                           */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
int32_t HID_Open(void)
{
	int32_t i32Ret = 0;
	
	g_HID_sDevice.isReportProtocol = 1;

	g_HID_sDevice.device = (void *)DrvUSB_InstallClassDevice(&sHidUsbClass);
	
	g_HID_sDevice.au8DeviceDescriptor = g_HID_au8DeviceDescriptor;
	g_HID_sDevice.au8ConfigDescriptor = g_HID_au8ConfigDescriptor;
	
	g_HID_sDevice.pu8HIDDescriptor = g_HID_sDevice.au8ConfigDescriptor + LEN_CONFIG + LEN_INTERFACE;	
	g_HID_sDevice.pu8IntInEPDescriptor = g_HID_sDevice.au8ConfigDescriptor + LEN_CONFIG + LEN_INTERFACE + LEN_HID;
	
	g_HID_sDevice.sVendorInfo.psVendorStringDesc = &g_HID_sVendorStringDesc;
	g_HID_sDevice.sVendorInfo.psProductStringDesc = &g_HID_sProductStringDesc;
	g_HID_sDevice.sVendorInfo.u16VendorId = *(uint16_t *)&g_HID_sDevice.au8DeviceDescriptor[8];
	g_HID_sDevice.sVendorInfo.u16ProductId = *(uint16_t *)&g_HID_sDevice.au8DeviceDescriptor[10];

	i32Ret = DrvUSB_InstallCtrlHandler(g_HID_sDevice.device, g_asCtrlCallbackEntry, 
					sizeof(g_asCtrlCallbackEntry) / sizeof(g_asCtrlCallbackEntry[0]));



	return i32Ret;
}

void HID_Close(void)
{

}

static void HID_UsbStartCallBack(void * pVoid)
{
	HID_Reset((S_HID_DEVICE *)pVoid);
	HID_Start((S_HID_DEVICE *)pVoid);
}

static void HID_Reset(S_HID_DEVICE *psDevice)
{	
	DrvUSB_Reset(1);
}

static void HID_Start(S_HID_DEVICE *psDevice)
{
    __weak extern void HID_SetFirstOutReport(void);
    __weak extern void HID_SetFirstInReport(void);

    /* To prepare the first IN report if it is necessary */
    if((uint32_t)HID_SetFirstInReport)
        HID_SetFirstInReport();


    /* To prepare the first OUT report if it is necessary */
    if((uint32_t)HID_SetFirstOutReport)
        HID_SetFirstOutReport();
}


/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      interrupt pipe call back function                                */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by g_sUsbOps[]                        */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      None                                                             */
/*                                                                       */
/*************************************************************************/
void HID_IntInCallback(void * pVoid)
{
    S_HID_DEVICE* psDevice = (S_HID_DEVICE*) pVoid;
	
	psDevice->isReportReady = 0;
}

int32_t HID_GetVendorInfo(S_VENDOR_INFO *psVendorInfo)
{
	if (! psVendorInfo)
	{
		//return (E_HID_NULL_POINTER);
		return -1;
	}

	psVendorInfo->u16VendorId = g_HID_sDevice.sVendorInfo.u16VendorId;
	psVendorInfo->u16ProductId = g_HID_sDevice.sVendorInfo.u16ProductId;
	psVendorInfo->psVendorStringDesc = g_HID_sDevice.sVendorInfo.psVendorStringDesc;
	psVendorInfo->psProductStringDesc = g_HID_sDevice.sVendorInfo.psProductStringDesc;
	
	return 0;
}

/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      Set vendor and product ID, and the string descriptor respectively*/
/*                                                                       */
/* INPUTS                                                                */
/*      psVendorInfo     vendor info structure                           */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
int32_t HID_SetVendorInfo(const S_VENDOR_INFO *psVendorInfo)
{
	if (! psVendorInfo ||
	        ! psVendorInfo->psVendorStringDesc ||
	        ! psVendorInfo->psProductStringDesc)
	{
		//return (E_HID_NULL_POINTER);
		return -1;
	}

	g_HID_sDevice.sVendorInfo.u16VendorId = psVendorInfo->u16VendorId;
	g_HID_sDevice.sVendorInfo.u16ProductId = psVendorInfo->u16ProductId;
	g_HID_sDevice.sVendorInfo.psVendorStringDesc = psVendorInfo->psVendorStringDesc;
	g_HID_sDevice.sVendorInfo.psProductStringDesc = psVendorInfo->psProductStringDesc;

	return 0;
}


/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      Set report descriptor. if not set, default will be used          */
/*      default HID report descriptor is mouse.                          */
/*                                                                       */
/* INPUTS                                                                */
/*      pu8ReportDescriptor     report descriptor buffer                 */
/*      u32ReportDescriptorSize report descriptor size                   */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
int32_t HID_SetReportDescriptor(const uint8_t* pu8ReportDescriptor, uint32_t u32ReportDescriptorSize)
{
	if (pu8ReportDescriptor == NULL)
	{
		return -1;
	}

	g_HID_sDevice.pu8ReportDescriptor = pu8ReportDescriptor;
	g_HID_sDevice.u32ReportDescriptorSize = u32ReportDescriptorSize;
	
	return 0;
}

/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      Set report buffer and size for interrupt pipe                    */
/*                                                                       */
/* INPUTS                                                                */
/*      pu8Report     buffer that will be sent to host when interupt IN  */
/*						happen                                           */
/*      u32ReportSize     buffer size                                    */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
int32_t HID_SetReportBuf(uint8_t* pu8Report, uint32_t u32ReportSize)
{
	if (pu8Report == NULL)
	{
		return -1;
	}
	if (u32ReportSize > HID_MAX_PACKET_SIZE_INT_IN)
	{
		return -1;
	}

	g_HID_sDevice.pu8Report = pu8Report;
	g_HID_sDevice.u32ReportSize = u32ReportSize;
    g_HID_sDevice.isReportReady = 0;

	return 0;
}


/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      whether or not the configure value is configure value of HID     */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by DrvUSB_RegisterCtrl                */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
static int32_t HID_IsConfigureValue(uint8_t u8ConfigureValue)
{
	return (u8ConfigureValue == g_HID_au8ConfigDescriptor[5]);
}


/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      The handler of Set Report request of HID request.                */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by DrvUSB_InstallCtrlHandler          */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
void HID_CtrlSetupSetReport(void * pVoid)
{
	S_DRVUSB_DEVICE *psDevice = (S_DRVUSB_DEVICE *)((S_HID_DEVICE *)pVoid)->device;

    //DBG_PRINTF("HID - Set Report");
	if(psDevice->au8Setup[3] == 1)
	{
        /* Report Type = input */

		// Trigger next Control Out DATA1 Transaction.
		_DRVUSB_SET_EP_TOG_BIT(1,0);
		_DRVUSB_TRIG_EP(1, 0);

        //DBG_PRINTF(" - Input\n");
	}
	else if (psDevice->au8Setup[3] == 2)
	{
		_DRVUSB_SET_EP_TOG_BIT(1, 0);
		_DRVUSB_TRIG_EP(1, 0x00);

        //DBG_PRINTF(" - Output\n");
	}
	else if (psDevice->au8Setup[3] == 3)
	{
        /* Request Type = Feature */
		_DRVUSB_SET_EP_TOG_BIT(1,0);
		_DRVUSB_TRIG_EP(1,0x00);

        //DBG_PRINTF(" - Feature\n");
	}
	else
	{
		// Not support. Reply STALL.       
        //DBG_PRINTF(" - Unknown\n");
		_HID_CLR_CTRL_READY_AND_TRIG_STALL();
	}
}

/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      The handler of Set Idle request of HID request.                  */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by DrvUSB_InstallCtrlHandler          */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
void HID_CtrlSetupSetIdle(void * pVoid)
{
	_DRVUSB_SET_EP_TOG_BIT(0, 0);
	_DRVUSB_TRIG_EP(0,0x00);
    
	//DBG_PRINTF("Set idle\n");
}

/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      The handler of Set Protocol request of HID request.              */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by DrvUSB_InstallCtrlHandler          */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      None                                                             */
/*                                                                       */
/*************************************************************************/
void HID_CtrlSetupSetProtocol(void * pVoid)
{
	S_HID_DEVICE *psDevice = (S_HID_DEVICE *) pVoid;
	S_DRVUSB_DEVICE *pUsbDevice = (S_DRVUSB_DEVICE *)psDevice->device;
	
	psDevice->isReportProtocol = pUsbDevice->au8Setup[2];

	_DRVUSB_SET_EP_TOG_BIT(0,0);
	_DRVUSB_TRIG_EP(0,0x00);
}



/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      The handler of Get Protocol request of HID request.              */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by DrvUSB_InstallCtrlHandler          */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      None                                                             */
/*                                                                       */
/*************************************************************************/
void HID_CtrlSetupGetProtocol(void * pVoid)
{
	S_HID_DEVICE *psDevice = (S_HID_DEVICE *) pVoid;
	
	DrvUSB_DataIn(0, (const uint8_t *)&psDevice->isReportProtocol, 1);
}


static uint16_t Minimum(uint16_t a, uint16_t b)
{
	if (a > b)
		return b;
	else
		return a;
}



void HID_PrepareDescriptors(const uint8_t *pu8Descriptor, uint32_t u32DescriptorSize, uint32_t u32RequestSize, uint32_t u32MaxPacketSize)
{
    
    gu32BytesInUsbBuf = u32RequestSize;
    if(u32RequestSize > u32DescriptorSize)
    {
        gu32BytesInUsbBuf = u32DescriptorSize;
        gIsOverRequest = 1;
    }
    gpu8UsbBuf = pu8Descriptor;

    //DBG_PRINTF("Get descriptor 0x%08x %d size.\n", pu8Descriptor, u32DescriptorSize);

	if(gu32BytesInUsbBuf < u32MaxPacketSize)
	{
	    DrvUSB_DataIn(0, gpu8UsbBuf, gu32BytesInUsbBuf); 
	    gpu8UsbBuf = 0;
	    gu32BytesInUsbBuf = 0;   
	}
	else
	{
		DrvUSB_DataIn(0, gpu8UsbBuf, u32MaxPacketSize);
		gpu8UsbBuf += u32MaxPacketSize;
		gu32BytesInUsbBuf -= u32MaxPacketSize;
    }

}


void HID_CtrlGetDescriptorOut(void * pVoid)
{
    gu32BytesInUsbBuf = 0;
    gpu8UsbBuf = 0;
    gIsOverRequest = 0;
}

void HID_CtrlGetDescriptorIn(void * pVoid)
{
    uint32_t u32Len;


    //DBG_PRINTF(" >>> 0x%08x %d size.\n", gpu8UsbBuf, gu32BytesInUsbBuf);
	
    if(gpu8UsbBuf)
    {

        if(gu32BytesInUsbBuf == 0)
        {
            /* Zero packet */
    		DrvUSB_DataIn(0, gpu8UsbBuf, 0);
    		gpu8UsbBuf = 0;
        }
        else
        {
            u32Len = Minimum(gu32BytesInUsbBuf, HID_MAX_PACKET_SIZE_CTRL);
    		DrvUSB_DataIn(0, gpu8UsbBuf, u32Len);
    		gpu8UsbBuf += u32Len;
    		gu32BytesInUsbBuf -= u32Len;
    		
    		if(gu32BytesInUsbBuf == 0)
    		{
                if(u32Len < HID_MAX_PACKET_SIZE_CTRL)
                {
                    /* This should be last IN packet due to it is less than UAC_MAX_PACKET_SIZE_EP0 */
                    gpu8UsbBuf = 0;
                }
                else
                {
                    if(!gIsOverRequest)
                    {
    		            /* This should be the last IN packet because there is no more data to 
                           transfer and it is not over request transfer */
                        gpu8UsbBuf = 0;
                    }
                 }
            }
    		
        }
    }
    else
    {
  	    /* The EP id 1 should always be used as control (OUT) endpoint */
		_DRVUSB_TRIG_EP(1,0x00);
    }
}


/*************************************************************************/
/*                                                                       */
/* DESCRIPTION                                                           */
/*      setup ACK handler for Get descriptor command                     */
/*                                                                       */
/* INPUTS                                                                */
/*      pVoid     parameter passed by DrvUSB_RegisterCtrl                */
/*                                                                       */
/* OUTPUTS                                                               */
/*      None                                                             */
/*                                                                       */
/* RETURN                                                                */
/*      0           Success                                              */
/*		Otherwise	error												 */
/*                                                                       */
/*************************************************************************/
void HID_CtrlSetupGetDescriptor(void * pVoid)
{
	S_HID_DEVICE *psDevice = (S_HID_DEVICE *) pVoid;
	S_DRVUSB_DEVICE *pUsbDevice = (S_DRVUSB_DEVICE *)psDevice->device;
	uint16_t u16Len;

	u16Len = 0;
	u16Len = pUsbDevice->au8Setup[7];
	u16Len <<= 8;
	u16Len += pUsbDevice->au8Setup[6];
	
	gIsOverRequest = 0;
	gu32BytesInUsbBuf = 0;
	gpu8UsbBuf = 0;
	switch (pUsbDevice->au8Setup[3])
	{
		// Get Device Descriptor
	case DESC_DEVICE:
	{
        HID_PrepareDescriptors(g_HID_sDevice.au8DeviceDescriptor, LEN_DEVICE, u16Len, HID_MAX_PACKET_SIZE_CTRL);

	    /* Prepare the OUT to avoid HOST stop data phase without all data transfered. */
		_DRVUSB_TRIG_EP(1,0x00);

		break;
	}

	// Get Configuration Descriptor
	case DESC_CONFIG:
	{	
        HID_PrepareDescriptors(g_HID_sDevice.au8ConfigDescriptor, g_HID_au8ConfigDescriptor[2], u16Len, HID_MAX_PACKET_SIZE_CTRL);
		break;
    }
		// Get HID Descriptor
	case DESC_HID:
    {
        HID_PrepareDescriptors(g_HID_sDevice.pu8HIDDescriptor, LEN_HID, u16Len, HID_MAX_PACKET_SIZE_CTRL);
		break;
    }
		// Get Report Descriptor
	case DESC_HID_RPT:
	{
        HID_PrepareDescriptors(g_HID_sDevice.pu8ReportDescriptor, g_HID_sDevice.u32ReportDescriptorSize, u16Len, HID_MAX_PACKET_SIZE_CTRL);
		break;
    }
		// Get String Descriptor
	case DESC_STRING:
	{
		// Get Language
		if (pUsbDevice->au8Setup[2] == 0)
		{
            HID_PrepareDescriptors(g_HID_au8StringLang, 4, u16Len, HID_MAX_PACKET_SIZE_CTRL);
		}
		else
		{
			// Get String Descriptor
			switch (pUsbDevice->au8Setup[2])
			{
			case 1:
                HID_PrepareDescriptors((const uint8_t *)g_HID_sDevice.sVendorInfo.psVendorStringDesc, g_HID_sDevice.sVendorInfo.psVendorStringDesc->byLength, u16Len, HID_MAX_PACKET_SIZE_CTRL);
				break;

			case 2:
                HID_PrepareDescriptors((const uint8_t *)g_HID_sDevice.sVendorInfo.psProductStringDesc, g_HID_sDevice.sVendorInfo.psProductStringDesc->byLength, u16Len, HID_MAX_PACKET_SIZE_CTRL);
               
 				break;

			case 3:
                HID_PrepareDescriptors(g_HID_au8StringSerial, g_HID_au8StringSerial[0], u16Len, HID_MAX_PACKET_SIZE_CTRL);
				break;

			default:
				/* Not support. Reply STALL. */
				DrvUSB_ClrCtrlReadyAndTrigStall();
			}
		}

		break;
	}
	default:
		/* Not support. Reply STALL. */
		DrvUSB_ClrCtrlReadyAndTrigStall();
	}
}

