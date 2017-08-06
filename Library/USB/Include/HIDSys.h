/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/
#ifndef __HIDSYS_H__
#define __HIDSYS_H__

#ifdef  __cplusplus
extern "C"
{
#endif

//#define HID_MAJOR_NUM	1
//#define HID_MINOR_NUM	00
//#define HID_BUILD_NUM	1
  
// E_HID_BUFFER_OVERRUN                 Allocated buffer is not enough.
// E_HID_CTRL_REG_TAB_FULL              Control register table is full.
// E_HID_EXCEED_INT_IN_PACKET_SIZE      Report size must be less than packet size of interrupt.
// E_HID_INVALID_EP_NUM                 Invalid EP number.
// E_HID_MUST_LESS_THAN_PACKET0_SIZE    Data size in control must be less than packet0 size.
// E_HID_NULL_POINTER                   NULL pointer.
// E_HID_UNDEFINE                       Undefined error.
// E_HID_INVALID_REG_NUM				  			Invaild register unmber

//#define E_HID_UNDEFINE                       _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 0)
//#define E_HID_NULL_POINTER                   _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 1)
//#define E_HID_BUFFER_OVERRUN                 _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 2)
//#define E_HID_INVALID_EP_NUM                 _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 3)
//#define E_HID_MUST_LESS_THAN_PACKET0_SIZE    _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 4)
//#define E_HID_EXCEED_INT_IN_PACKET_SIZE      _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 5)
//#define E_HID_CTRL_REG_TAB_FULL              _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 6)
//#define E_HID_INVALID_REG_NUM                _SYSINFRA_ERRCODE(TRUE, MODULE_ID_HID, 7)
			
//#define HID_VERSION_NUM    _SYSINFRA_VERSION(HID_MAJOR_NUM, HID_MINOR_NUM, HID_BUILD_NUM)

   
#define	HID_MAX_PACKET_SIZE_CTRL        8
#define HID_MAX_PACKET_SIZE_INT_IN		64


/* Define the interrupt In EP number */
#define CTRL_EP_NUM     0x00
#define INT_IN_EP_NUM	0x01



//***************************************************
// 		HID Class REQUEST
//***************************************************
#define GET_REPORT          0x01
#define GET_IDLE            0x02
#define GET_PROTOCOL        0x03
#define SET_REPORT          0x09
#define SET_IDLE            0x0A
#define SET_PROTOCOL        0x0B

#define HID_RPT_TYPE_INPUT		0x01
#define HID_RPT_TYPE_OUTPUT		0x02
#define HID_RPT_TYPE_FEATURE	0x03

typedef struct
{
	S_DRVUSB_VENDOR_INFO sVendorInfo;

	const uint8_t *au8DeviceDescriptor;
	const uint8_t *au8ConfigDescriptor;

	const uint8_t *pu8IntInEPDescriptor;

	const uint8_t *pu8ReportDescriptor;
	uint32_t u32ReportDescriptorSize;

	const uint8_t *pu8HIDDescriptor;

	uint8_t *pu8Report;
	uint32_t u32ReportSize;
    int8_t isReportReady;

	int8_t isReportProtocol;

	void * *device;
	
} S_HID_DEVICE;

extern S_HID_DEVICE g_HID_sDevice;

static __INLINE void _HID_CLR_CTRL_READY_AND_TRIG_STALL()
{
	_DRVUSB_CLEAR_EP_READY_AND_TRIG_STALL(0);
	_DRVUSB_CLEAR_EP_READY_AND_TRIG_STALL(1);
}

static __INLINE void _HID_CLR_CTRL_READY()
{
	_DRVUSB_CLEAR_EP_READY(0);
	_DRVUSB_CLEAR_EP_READY(1);
}


int32_t	HID_Open(void);
void HID_Close(void);
void HID_IntInCallback(void * pVoid);
int32_t	HID_SetReportDescriptor(const uint8_t * pu8ReportDescriptor, uint32_t u32ReportDescriptorSize);
int32_t	HID_SetReportBuf(uint8_t * pu8Report, uint32_t u32ReportSize);
int32_t	HID_SetIntInInterval(uint32_t u32MilliSec);
void HID_CtrlSetupSetReport(void * pVoid);
void HID_CtrlSetupSetIdle(void * pVoid);
void HID_CtrlSetupSetProtocol(void * pVoid);
void HID_CtrlSetupGetDescriptor(void * pVoid);
void HID_CtrlGetDescriptorIn(void * pVoid);
void HID_CtrlGetDescriptorOut(void * pVoid);
void HID_CtrlSetupGetProtocol(void * pVoid);

#ifdef  __cplusplus
}
#endif

#endif // #ifndef __HIDSYS_H__


