/*
 * USB Chapter 9 Protocol (Header File)
 *
 * This file defines data structures, constants, and macros that are used to
 * to support the USB Device Framework protocol described in Chapter 9 of the
 * USB 2.0 specification.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
#ifndef _USB_CH9_H_
#define _USB_CH9_H_

//
// Section: USB Descriptors
//
#define USB_DESCRIPTOR_DEVICE		0x01	// bDescriptorType for a Device Descriptor.
#define USB_DESCRIPTOR_CONFIGURATION	0x02	// bDescriptorType for a Configuration Descriptor.
#define USB_DESCRIPTOR_STRING		0x03	// bDescriptorType for a String Descriptor.
#define USB_DESCRIPTOR_INTERFACE	0x04	// bDescriptorType for an Interface Descriptor.
#define USB_DESCRIPTOR_ENDPOINT		0x05	// bDescriptorType for an Endpoint Descriptor.
#define USB_DESCRIPTOR_DEVICE_QUALIFIER	0x06	// bDescriptorType for a Device Qualifier.
#define USB_DESCRIPTOR_OTHER_SPEED	0x07	// bDescriptorType for a Other Speed Configuration.
#define USB_DESCRIPTOR_INTERFACE_POWER	0x08	// bDescriptorType for Interface Power.
#define USB_DESCRIPTOR_OTG		0x09	// bDescriptorType for an OTG Descriptor.

/*
 * USB Device Descriptor Structure
 *
 * This struct defines the structure of a USB Device Descriptor.  Note that this
 * structure may need to be packed, or even accessed as bytes, to properly access
 * the correct fields when used on some device architectures.
 */
typedef struct __attribute__ ((packed)) _USB_DEVICE_DESCRIPTOR
{
	unsigned char bLength;			// Length of this descriptor.
	unsigned char bDescriptorType;		// DEVICE descriptor type (USB_DESCRIPTOR_DEVICE).
	unsigned short bcdUSB;			// USB Spec Release Number (BCD).
	unsigned char bDeviceClass;		// Class code (assigned by the USB-IF). 0xFF-Vendor specific.
	unsigned char bDeviceSubClass;		// Subclass code (assigned by the USB-IF).
	unsigned char bDeviceProtocol;		// Protocol code (assigned by the USB-IF). 0xFF-Vendor specific.
	unsigned char bMaxPacketSize0;		// Maximum packet size for endpoint 0.
	unsigned short idVendor;		// Vendor ID (assigned by the USB-IF).
	unsigned short idProduct;		// Product ID (assigned by the manufacturer).
	unsigned short bcdDevice;		// Device release number (BCD).
	unsigned char iManufacturer;		// Index of String Descriptor describing the manufacturer.
	unsigned char iProduct;			// Index of String Descriptor describing the product.
	unsigned char iSerialNumber;		// Index of String Descriptor with the device's serial number.
	unsigned char bNumConfigurations;	// Number of possible configurations.
} USB_DEVICE_DESCRIPTOR;


/*
 * USB Configuration Descriptor Structure
 *
 * This struct defines the structure of a USB Configuration Descriptor.  Note that this
 * structure may need to be packed, or even accessed as bytes, to properly access
 * the correct fields when used on some device architectures.
 */
typedef struct __attribute__ ((packed)) _USB_CONFIGURATION_DESCRIPTOR
{
	unsigned char bLength;			// Length of this descriptor.
	unsigned char bDescriptorType;		// CONFIGURATION descriptor type (USB_DESCRIPTOR_CONFIGURATION).
	unsigned short wTotalLength;		// Total length of all descriptors for this configuration.
	unsigned char bNumInterfaces;		// Number of interfaces in this configuration.
	unsigned char bConfigurationValue;	// Value of this configuration (1 based).
	unsigned char iConfiguration;		// Index of String Descriptor describing the configuration.
	unsigned char bmAttributes;		// Configuration characteristics.
	unsigned char bMaxPower;		// Maximum power consumed by this configuration.
} USB_CONFIGURATION_DESCRIPTOR;

// Attributes bits
#define USB_CFG_DSC_REQUIRED	0x80				// Required attribute
#define USB_CFG_DSC_SELF_PWR	(0x40 | USB_CFG_DSC_REQUIRED)	// Device is self powered.
#define USB_CFG_DSC_REM_WAKE	(0x20 | USB_CFG_DSC_REQUIRED)	// Device can request remote wakup


/*
 * USB Interface Descriptor Structure
 *
 * This struct defines the structure of a USB Interface Descriptor.  Note that this
 * structure may need to be packed, or even accessed as bytes, to properly access
 * the correct fields when used on some device architectures.
 */
typedef struct __attribute__ ((packed)) _USB_INTERFACE_DESCRIPTOR
{
	unsigned char bLength;			// Length of this descriptor.
	unsigned char bDescriptorType;		// INTERFACE descriptor type (USB_DESCRIPTOR_INTERFACE).
	unsigned char bInterfaceNumber;		// Number of this interface (0 based).
	unsigned char bAlternateSetting;	// Value of this alternate interface setting.
	unsigned char bNumEndpoints;		// Number of endpoints in this interface.
	unsigned char bInterfaceClass;		// Class code (assigned by the USB-IF).  0xFF-Vendor specific.
	unsigned char bInterfaceSubClass;	// Subclass code (assigned by the USB-IF).
	unsigned char bInterfaceProtocol;	// Protocol code (assigned by the USB-IF).  0xFF-Vendor specific.
	unsigned char iInterface;		// Index of String Descriptor describing the interface.
} USB_INTERFACE_DESCRIPTOR;


// *****************************************************************************
/* USB Endpoint Descriptor Structure

This struct defines the structure of a USB Endpoint Descriptor.  Note that this
structure may need to be packed, or even accessed as bytes, to properly access
the correct fields when used on some device architectures.
*/
typedef struct __attribute__ ((packed)) _USB_ENDPOINT_DESCRIPTOR
{
	unsigned char bLength;		// Length of this descriptor.
	unsigned char bDescriptorType;	// ENDPOINT descriptor type (USB_DESCRIPTOR_ENDPOINT).
	unsigned char bEndpointAddress;	// Endpoint address. Bit 7 indicates direction (0=OUT, 1=IN).
	unsigned char bmAttributes;	// Endpoint transfer type.
	unsigned short wMaxPacketSize;  // Maximum packet size.
	unsigned char bInterval;	// Polling interval in frames.
} USB_ENDPOINT_DESCRIPTOR;


// Endpoint Direction
#define EP_DIR_IN           0x80    // Data flows from device to host
#define EP_DIR_OUT          0x00    // Data flows from host to device


// ******************************************************************
// USB Endpoint Attributes
// ******************************************************************

// Section: Transfer Types
#define EP_ATTR_CONTROL     (0<<0)  // Endoint used for control transfers
#define EP_ATTR_ISOCH       (1<<0)  // Endpoint used for isochronous transfers
#define EP_ATTR_BULK        (2<<0)  // Endpoint used for bulk transfers
#define EP_ATTR_INTR        (3<<0)  // Endpoint used for interrupt transfers

// Section: Synchronization Types (for isochronous enpoints)
#define EP_ATTR_NO_SYNC     (0<<2)  // No Synchronization
#define EP_ATTR_ASYNC       (1<<2)  // Asynchronous
#define EP_ATTR_ADAPT       (2<<2)  // Adaptive synchronization
#define EP_ATTR_SYNC        (3<<2)  // Synchronous

// Section: Usage Types (for isochronous endpoints)
#define EP_ATTR_DATA        (0<<4)  // Data Endpoint
#define EP_ATTR_FEEDBACK    (1<<4)  // Feedback endpoint
#define EP_ATTR_IMP_FB      (2<<4)  // Implicit Feedback data EP

// Section: Max Packet Sizes
#define EP_MAX_PKT_INTR_LS  8       // Max low-speed interrupt packet
#define EP_MAX_PKT_INTR_FS  64      // Max full-speed interrupt packet
#define EP_MAX_PKT_ISOCH_FS 1023    // Max full-speed isochronous packet
#define EP_MAX_PKT_BULK_FS  64      // Max full-speed bulk packet
#define EP_LG_PKT_BULK_FS   32      // Large full-speed bulk packet
#define EP_MED_PKT_BULK_FS  16      // Medium full-speed bulk packet
#define EP_SM_PKT_BULK_FS   8       // Small full-speed bulk packet


// *****************************************************************************
/* USB OTG Descriptor Structure

This struct defines the structure of a USB OTG Descriptor.  Note that this
structure may need to be packed, or even accessed as bytes, to properly access
the correct fields when used on some device architectures.
*/
typedef struct __attribute__ ((packed)) _USB_OTG_DESCRIPTOR
{
	unsigned char bLength;		// Length of this descriptor.
	unsigned char bDescriptorType;	// OTG descriptor type (USB_DESCRIPTOR_OTG).
	unsigned char bmAttributes;	// OTG attributes.
} USB_OTG_DESCRIPTOR;


// ******************************************************************
// Section: USB String Descriptor Structure
// ******************************************************************
// This structure describes the USB string descriptor.  The string
// descriptor provides user-readable information about various aspects of
// the device.  The first string desriptor (string descriptor zero (0)),
// provides a list of the number of languages supported by the set of
// string descriptors for this device instead of an actual string.
//
// Note: The strings are in 2-byte-per-character unicode, not ASCII.
//
// Note: This structure only describes the "header" of the string
// descriptor.  The actual data (either the language ID array or the
// array of unicode characters making up the string, must be allocated
// immediately following this header with no padding between them.

typedef struct __attribute__ ((packed)) _USB_STRING_DSC
{
	unsigned char bLength;		// Size of this descriptor
	unsigned char bDescriptorType;	// Type, USB_DSC_STRING

} USB_STRING_DESCRIPTOR;

#define USB_STRING_INIT(nchars) struct {\
	unsigned char bLength;          \
	unsigned char bDescriptorType;	\
        unsigned short string[nchars];  \
}


// ******************************************************************
// Section: USB Device Qualifier Descriptor Structure
// ******************************************************************
// This structure describes the device qualifier descriptor.  The device
// qualifier descriptor provides overall device information if the device
// supports "other" speeds.
//
// Note: A high-speed device may support "other" speeds (ie. full or low).
// If so, it may need to implement the the device qualifier and other
// speed descriptors.

typedef struct __attribute__ ((packed)) _USB_DEVICE_QUALIFIER_DESCRIPTOR
{
	unsigned char bLength;			// Size of this descriptor
	unsigned char bType;			// Type, always USB_DESCRIPTOR_DEVICE_QUALIFIER
	unsigned short bcdUSB;			// USB spec version, in BCD
	unsigned char bDeviceClass;		// Device class code
	unsigned char bDeviceSubClass;		// Device sub-class code
	unsigned char bDeviceProtocol;		// Device protocol
	unsigned char bMaxPacketSize0;		// EP0, max packet size
	unsigned char bNumConfigurations;	// Number of "other-speed" configurations
	unsigned char bReserved;		// Always zero (0)

} USB_DEVICE_QUALIFIER_DESCRIPTOR;


// ******************************************************************
// Section: USB Setup Packet Structure
// ******************************************************************
// This structure describes the data contained in a USB standard device
// request's setup packet.  It is the data packet sent from the host to
// the device to control and configure the device.
//
// Note: Refer to the USB 2.0 specification for additional details on the
// usage of the setup packet and standard device requests.

typedef struct __attribute__ ((packed))
{
	union					// offset   description
	{					// ------   ------------------------
		unsigned char bmRequestType;	//   0      Bit-map of request type
		struct {
			unsigned recipient:  5;	//          Recipient of the request
			unsigned type:       2;	//          Type of request
			unsigned direction:  1;	//          Direction of data X-fer
		};
	} requestInfo;

	unsigned char bRequest;			//   1      Request type
	unsigned short wValue;			//   2      Depends on bRequest
	unsigned short wIndex;			//   4      Depends on bRequest
	unsigned short wLength;			//   6      Depends on bRequest

} SETUP_PKT, *PSETUP_PKT;


//
// Section: USB Specification Constants
//

// Section: Valid PID Values
//DOM-IGNORE-BEGIN
#define PID_OUT                                 0x1     // PID for an OUT token
#define PID_ACK                                 0x2     // PID for an ACK handshake
#define PID_DATA0                               0x3     // PID for DATA0 data
#define PID_PING                                0x4     // Special PID PING
#define PID_SOF                                 0x5     // PID for a SOF token
#define PID_NYET                                0x6     // PID for a NYET handshake
#define PID_DATA2                               0x7     // PID for DATA2 data
#define PID_SPLIT                               0x8     // Special PID SPLIT
#define PID_IN                                  0x9     // PID for a IN token
#define PID_NAK                                 0xA     // PID for a NAK handshake
#define PID_DATA1                               0xB     // PID for DATA1 data
#define PID_PRE                                 0xC     // Special PID PRE (Same as PID_ERR)
#define PID_ERR                                 0xC     // Special PID ERR (Same as PID_PRE)
#define PID_SETUP                               0xD     // PID for a SETUP token
#define PID_STALL                               0xE     // PID for a STALL handshake
#define PID_MDATA                               0xF     // PID for MDATA data

#define PID_MASK_DATA                           0x03    // Data PID mask
#define PID_MASK_DATA_SHIFTED                  (PID_MASK_DATA << 2) // Data PID shift to proper position
//DOM-IGNORE-END

// Section: USB Token Types
//DOM-IGNORE-BEGIN
#define USB_TOKEN_OUT                           0x01    // U1TOK - OUT token
#define USB_TOKEN_IN                            0x09    // U1TOK - IN token
#define USB_TOKEN_SETUP                         0x0D    // U1TOK - SETUP token
//DOM-IGNORE-END

// Section: OTG Descriptor Constants

#define OTG_HNP_SUPPORT                         0x02    // OTG Descriptor bmAttributes - HNP support flag
#define OTG_SRP_SUPPORT                         0x01    // OTG Descriptor bmAttributes - SRP support flag

// Section: Endpoint Directions

#define USB_IN_EP                               0x80    // IN endpoint mask
#define USB_OUT_EP                              0x00    // OUT endpoint mask

// Section: Standard Device Requests

#define USB_REQUEST_GET_STATUS                  0       // Standard Device Request - GET STATUS
#define USB_REQUEST_CLEAR_FEATURE               1       // Standard Device Request - CLEAR FEATURE
#define USB_REQUEST_SET_FEATURE                 3       // Standard Device Request - SET FEATURE
#define USB_REQUEST_SET_ADDRESS                 5       // Standard Device Request - SET ADDRESS
#define USB_REQUEST_GET_DESCRIPTOR              6       // Standard Device Request - GET DESCRIPTOR
#define USB_REQUEST_SET_DESCRIPTOR              7       // Standard Device Request - SET DESCRIPTOR
#define USB_REQUEST_GET_CONFIGURATION           8       // Standard Device Request - GET CONFIGURATION
#define USB_REQUEST_SET_CONFIGURATION           9       // Standard Device Request - SET CONFIGURATION
#define USB_REQUEST_GET_INTERFACE               10      // Standard Device Request - GET INTERFACE
#define USB_REQUEST_SET_INTERFACE               11      // Standard Device Request - SET INTERFACE
#define USB_REQUEST_SYNCH_FRAME                 12      // Standard Device Request - SYNCH FRAME

#define USB_FEATURE_ENDPOINT_HALT               0       // CLEAR/SET FEATURE - Endpoint Halt
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP        1       // CLEAR/SET FEATURE - Device remote wake-up
#define USB_FEATURE_TEST_MODE                   2       // CLEAR/SET FEATURE - Test mode

// Section: Setup Data Constants

#define USB_SETUP_HOST_TO_DEVICE                0x00    // Device Request bmRequestType transfer direction - host to device transfer
#define USB_SETUP_DEVICE_TO_HOST                0x80    // Device Request bmRequestType transfer direction - device to host transfer
#define USB_SETUP_TYPE_STANDARD                 0x00    // Device Request bmRequestType type - standard
#define USB_SETUP_TYPE_CLASS                    0x20    // Device Request bmRequestType type - class
#define USB_SETUP_TYPE_VENDOR                   0x40    // Device Request bmRequestType type - vendor
#define USB_SETUP_RECIPIENT_DEVICE              0x00    // Device Request bmRequestType recipient - device
#define USB_SETUP_RECIPIENT_INTERFACE           0x01    // Device Request bmRequestType recipient - interface
#define USB_SETUP_RECIPIENT_ENDPOINT            0x02    // Device Request bmRequestType recipient - endpoint
#define USB_SETUP_RECIPIENT_OTHER               0x03    // Device Request bmRequestType recipient - other

// Section: OTG SET FEATURE Constants

#define OTG_FEATURE_B_HNP_ENABLE                3       // SET FEATURE OTG - Enable B device to perform HNP
#define OTG_FEATURE_A_HNP_SUPPORT               4       // SET FEATURE OTG - A device supports HNP
#define OTG_FEATURE_A_ALT_HNP_SUPPORT           5       // SET FEATURE OTG - Another port on the A device supports HNP

// Section: USB Endpoint Transfer Types

#define USB_TRANSFER_TYPE_CONTROL               0x00    // Endpoint is a control endpoint.
#define USB_TRANSFER_TYPE_ISOCHRONOUS           0x01    // Endpoint is an isochronous endpoint.
#define USB_TRANSFER_TYPE_BULK                  0x02    // Endpoint is a bulk endpoint.
#define USB_TRANSFER_TYPE_INTERRUPT             0x03    // Endpoint is an interrupt endpoint.

// Section: Standard Feature Selectors for CLEAR_FEATURE Requests
#define USB_FEATURE_ENDPOINT_STALL              0       // Endpoint recipient
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP        1       // Device recipient
#define USB_FEATURE_TEST_MODE                   2       // Device recipient


// Section: USB Class Code Definitions
#define USB_HUB_CLASSCODE                       0x09    //  Class code for a hub.

#endif  // _USB_CH9_H_
