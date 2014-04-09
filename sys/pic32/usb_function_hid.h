/*
 * USB HID Function Driver File
 *
 * This file contains all of functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the HID function
 * driver. This file should be included in projects that use the HID
 * function driver.  This file should also be included into the
 * usb_descriptors.c file and any other user file that requires access to the
 * HID interface.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC(R) Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PIC Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
#ifndef HID_H
#define HID_H

/*
 * Default HID configuration.
 */
#ifndef HID_EP
#   define HID_EP               1
#endif
#ifndef HID_INTF_ID
#   define HID_INTF_ID          0x00
#endif
#ifndef HID_BD_OUT
#   define HID_BD_OUT           USB_EP_1_OUT
#endif
#ifndef HID_INT_OUT_EP_SIZE
#   define HID_INT_OUT_EP_SIZE  3
#endif
#ifndef HID_BD_IN
#   define HID_BD_IN            USB_EP_1_IN
#endif
#ifndef HID_INT_IN_EP_SIZE
#   define HID_INT_IN_EP_SIZE   3
#endif
#ifndef HID_NUM_OF_DSC
#   define HID_NUM_OF_DSC       1
#endif
#ifndef HID_RPT01_SIZE
#   define HID_RPT01_SIZE       47
#endif

/* Class-Specific Requests */
#define GET_REPORT              0x01
#define GET_IDLE                0x02
#define GET_PROTOCOL            0x03
#define SET_REPORT              0x09
#define SET_IDLE                0x0A
#define SET_PROTOCOL            0x0B

/* Class Descriptor Types */
#define DSC_HID                 0x21
#define DSC_RPT                 0x22
#define DSC_PHY                 0x23

/* Protocol Selection */
#define BOOT_PROTOCOL           0x00
#define RPT_PROTOCOL            0x01

/* HID Interface Class Code */
#define HID_INTF                0x03

/* HID Interface Class SubClass Codes */
#define BOOT_INTF_SUBCLASS      0x01

/* HID Interface Class Protocol Codes */
#define HID_PROTOCOL_NONE       0x00
#define HID_PROTOCOL_KEYBOARD   0x01
#define HID_PROTOCOL_MOUSE      0x02

extern const unsigned char hid_rpt01 [HID_RPT01_SIZE];
extern volatile unsigned char hid_report_out[HID_INT_OUT_EP_SIZE];
extern volatile unsigned char hid_report_in[HID_INT_IN_EP_SIZE];
extern volatile unsigned char hid_report_feature[HID_FEATURE_REPORT_BYTES];

//
// USB HID Descriptor header as detailed in section
// "6.2.1 HID Descriptor" of the HID class definition specification
//
typedef struct _USB_HID_DSC_HEADER
{
    unsigned char bDescriptorType;      // offset 9
    unsigned short wDscLength;          // offset 10

} USB_HID_DSC_HEADER;

//
// USB HID Descriptor header as detailed in section
// "6.2.1 HID Descriptor" of the HID class definition specification
//
typedef struct _USB_HID_DSC
{
    unsigned char bLength;              // offset 0
    unsigned char bDescriptorType;      // offset 1
    unsigned short bcdHID;              // offset 2
    unsigned char bCountryCode;         // offset 4
    unsigned char bNumDsc;              // offset 5

} USB_HID_DSC;

void hid_check_request(void);

#endif // HID_H
