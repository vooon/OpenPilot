/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
* File Name          : usb_desc.c
* Author             : MCD Application Team
* Version            : V3.2.1
* Date               : 07/05/2010
* Description        : Descriptors for Custom HID Demo
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#include "usb_lib.h"
#include "pios_usb.h"
#include "pios_usb_hid.h"
#include "pios_usb_hid_desc.h"

extern  uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ];

uint8_t *PIOS_HID_USR_DeviceDescriptor( uint8_t speed , uint16_t *length);
uint8_t *PIOS_HID_LangIDStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *PIOS_HID_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *PIOS_HID_ProductStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *PIOS_HID_SerialStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *PIOS_HID_ConfigStrDescriptor( uint8_t speed , uint16_t *length);
uint8_t *PIOS_HID_InterfaceStrDescriptor( uint8_t speed , uint16_t *length);

USBD_DEVICE PIOS_HID_USR_desc = 
{
    PIOS_HID_USR_DeviceDescriptor,
    PIOS_HID_LangIDStrDescriptor,
    PIOS_HID_ManufacturerStrDescriptor,
    PIOS_HID_ProductStrDescriptor,
    PIOS_HID_SerialStrDescriptor,
    PIOS_HID_ConfigStrDescriptor,
    PIOS_HID_InterfaceStrDescriptor,

};

// *************************************************
// USB Standard Device Descriptor

__ALIGN_BEGIN uint8_t PIOS_HID_DeviceDescriptor[PIOS_HID_SIZ_DEVICE_DESC] __ALIGN_END =
  {
    0x12,                       // bLength
    USB_DEVICE_DESCRIPTOR_TYPE, // bDescriptorType
    0x00,                       // bcdUSB
    0x02,
    0x00,                       // bDeviceClass
    0x00,                       // bDeviceSubClass
    0x00,                       // bDeviceProtocol
    0x40,                       // bMaxPacketSize40
    (uint8_t)((PIOS_USB_VENDOR_ID) & 0xff),   // idVendor
    (uint8_t)((PIOS_USB_VENDOR_ID) >> 8),
    (uint8_t)((PIOS_USB_PRODUCT_ID) & 0xff),  // idProduct
    (uint8_t)((PIOS_USB_PRODUCT_ID) >> 8),
    (uint8_t)((PIOS_USB_VERSION_ID) & 0xff),  // bcdDevice
    (uint8_t)((PIOS_USB_VERSION_ID) >> 8),
    USBD_IDX_MFC_STR,            // Index of string descriptor describing manufacturer
    USBD_IDX_PRODUCT_STR,        // Index of string descriptor describing product
    USBD_IDX_SERIAL_STR,         // Index of string descriptor describing the device serial number
    0x01                         // bNumConfigurations
  };


#ifdef USB_OTG_HS_CORE
/* USB Standard Device Descriptor */
//__ALIGN_BEGIN const uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};
#endif


// *************************************************
// USB Configuration Descriptor
//   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor

const uint8_t PIOS_HID_ConfigDescriptor[PIOS_HID_SIZ_CONFIG_DESC] =
  {
    0x09,         // bLength: Configuation Descriptor size
    USB_CONFIGURATION_DESCRIPTOR_TYPE, // bDescriptorType: Configuration
    PIOS_HID_SIZ_CONFIG_DESC,
    // wTotalLength: Bytes returned
    0x00,
    0x01,         // bNumInterfaces: 1 interface
    0x01,         // bConfigurationValue: Configuration value
    0x00,         // iConfiguration: Index of string descriptor describing the configuration
    0xC0,         // bmAttributes: Bus powered
    0x7D,         // MaxPower 250 mA - needed to power the RF transmitter

    // ************** Descriptor of Custom HID interface ****************
    // 09
    0x09,         // bLength: Interface Descriptor size
    USB_INTERFACE_DESCRIPTOR_TYPE, // bDescriptorType: Interface descriptor type
    0x00,         // bInterfaceNumber: Number of Interface
    0x00,         // bAlternateSetting: Alternate setting
    0x02,         // bNumEndpoints
    0x03,         // bInterfaceClass: HID
    0x00,         // bInterfaceSubClass : 1=BOOT, 0=no boot
    0x00,         // nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse
    0,            // iInterface: Index of string descriptor

    // ******************** Descriptor of Custom HID HID ********************
    // 18
    0x09,         // bLength: HID Descriptor size
    HID_DESCRIPTOR_TYPE, // bDescriptorType: HID
    0x10,         // bcdHID: HID Class Spec release number
    0x01,
    0x00,         // bCountryCode: Hardware target country
    0x01,         // bNumDescriptors: Number of HID class descriptors to follow
    0x22,         // bDescriptorType
    PIOS_HID_SIZ_REPORT_DESC, // wItemLength: Total length of Report descriptor
    0x00,

    // ******************** Descriptor of Custom HID endpoints ******************
    // 27
    0x07,          // bLength: Endpoint Descriptor size
    USB_ENDPOINT_DESCRIPTOR_TYPE, // bDescriptorType:

    0x81,          // bEndpointAddress: Endpoint Address (IN)
    0x03,          // bmAttributes: Interrupt endpoint
    0x40,          // wMaxPacketSize: 2 Bytes max
    0x00,
    0x04,          // bInterval: Polling Interval in ms
    // 34
    	
    0x07,	// bLength: Endpoint Descriptor size
    USB_ENDPOINT_DESCRIPTOR_TYPE,	// bDescriptorType:
			// Endpoint descriptor type
    0x01,	// bEndpointAddress:
			//	Endpoint Address (OUT)
    0x03,	// bmAttributes: Interrupt endpoint
    0x40,	// wMaxPacketSize: 2 Bytes max
    0x00,
    0x04,	// bInterval: Polling Interval in ms
    // 41
  };

// *************************************************

 const uint8_t PIOS_HID_ReportDescriptor[PIOS_HID_SIZ_REPORT_DESC] =
  {                    
    0x06, 0x9c, 0xff,      // USAGE_PAGE (Vendor Page: 0xFF00)
    0x09, 0x01,            // USAGE (Demo Kit)
    0xa1, 0x01,            // COLLECTION (Application)
    // 6
    
    // Data 1
    0x85, 0x01,            //     REPORT_ID (1)
    0x09, 0x02,            //     USAGE (LED 1)
    0x15, 0x00,            //     LOGICAL_MINIMUM (0)
    0x25, 0xff,            //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,            //     REPORT_SIZE (8)
    0x95, PIOS_USB_HID_DATA_LENGTH+1,            //     REPORT_COUNT (1)
    0x81, 0x83,            //     INPUT (Const,Var,Array)
    // 20
	  
    // Data 1
    0x85, 0x02,            //     REPORT_ID (2)
    0x09, 0x03,            //     USAGE (LED 1)
    0x15, 0x00,            //     LOGICAL_MINIMUM (0)
    0x25, 0xff,            //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,            //     REPORT_SIZE (8)
    0x95, PIOS_USB_HID_DATA_LENGTH+1,            //     REPORT_COUNT (1)
    0x91, 0x82,            //     OUTPUT (Data,Var,Abs,Vol)
    // 34
	  
    0xc0 	          //     END_COLLECTION
  };

// *************************************************
// USB String Descriptors (optional)

uint8_t PIOS_HID_StringLangID[PIOS_HID_SIZ_STRING_LANGID] =
  {
    PIOS_HID_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09, 0x04			// LangID = 0x0409: U.S. English
  };


/**
* @brief  USBD_USR_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_USR_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(PIOS_HID_DeviceDescriptor);
  return PIOS_HID_DeviceDescriptor;
}


/**
* @brief  PIOS_HID_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(PIOS_HID_StringLangID);  
  return PIOS_HID_StringLangID;
}


/**
* @brief  PIOS_HID_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (PIOS_USB_VENDOR_STR, USBD_StrDesc, length);
  return USBD_StrDesc;
}


/**
* @brief  PIOS_HID_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (PIOS_USB_PRODUCT_STR, USBD_StrDesc, length);    
  return USBD_StrDesc;
}


/**
* @brief  PIOS_HID_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (PIOS_HID_StringSerial, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  PIOS_HID_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (PIOS_USB_INTERFACE_STR, USBD_StrDesc, length);
  return USBD_StrDesc;  
}


/**
* @brief  PIOS_HID_InterfaceStrDescriptor 
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  PIOS_HID_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (PIOS_USB_INTERFACE_STR, USBD_StrDesc, length);
  return USBD_StrDesc;  
}


uint8_t PIOS_HID_StringSerial[PIOS_HID_SIZ_STRING_SERIAL] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // 26 nulls
  };

// *************************************************
