/*
 * USB Device header file
 *
 * This file, with its associated C source file, provides the main substance of
 * the USB device side stack.  These files will receive, transmit, and process
 * various USB commands as well as take action when required for various events
 * that occur on the bus.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC® Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PIC Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
#ifndef USBDEVICE_H
#define USBDEVICE_H

#include <machine/usb_ch9.h>
#include <machine/usb_hal_pic32.h>

/*
 * USB Endpoint Definitions
 * USB Standard EP Address Format: DIR:X:X:X:EP3:EP2:EP1:EP0
 * This is used in the descriptors. See usbcfg.c
 *
 * NOTE: Do not use these values for checking against USTAT.
 * To check against USTAT, use values defined in usbd.h.
 */
#define _EP_IN		0x80
#define _EP_OUT		0x00
#define _EP01_OUT	0x01
#define _EP01_IN	0x81
#define _EP02_OUT	0x02
#define _EP02_IN	0x82
#define _EP03_OUT	0x03
#define _EP03_IN	0x83
#define _EP04_OUT	0x04
#define _EP04_IN	0x84
#define _EP05_OUT	0x05
#define _EP05_IN	0x85
#define _EP06_OUT	0x06
#define _EP06_IN	0x86
#define _EP07_OUT	0x07
#define _EP07_IN	0x87
#define _EP08_OUT	0x08
#define _EP08_IN	0x88
#define _EP09_OUT	0x09
#define _EP09_IN	0x89
#define _EP10_OUT	0x0A
#define _EP10_IN	0x8A
#define _EP11_OUT	0x0B
#define _EP11_IN	0x8B
#define _EP12_OUT	0x0C
#define _EP12_IN	0x8C
#define _EP13_OUT	0x0D
#define _EP13_IN	0x8D
#define _EP14_OUT	0x0E
#define _EP14_IN	0x8E
#define _EP15_OUT	0x0F
#define _EP15_IN	0x8F

/* Configuration Attributes */
#define _DEFAULT	(0x01 << 7)	// Default Value (Bit 7 is set)
#define _SELF		(0x01 << 6)	// Self-powered (Supports if set)
#define _RWU		(0x01 << 5)	// Remote Wakeup (Supports if set)
#define _HNP		(0x01 << 1)	// HNP (Supports if set)
#define _SRP		(0x01)		// SRP (Supports if set)

/* Endpoint Transfer Type */
#define _CTRL		0x00		// Control Transfer
#define _ISO		0x01		// Isochronous Transfer
#define _BULK		0x02		// Bulk Transfer
#define _INTERRUPT	0x03		// Interrupt Transfer

/* Isochronous Endpoint Synchronization Type */
#define _NS		(0x00 << 2)	// No Synchronization
#define _AS		(0x01 << 2)	// Asynchronous
#define _AD		(0x02 << 2)	// Adaptive
#define _SY		(0x03 << 2)	// Synchronous

/* Isochronous Endpoint Usage Type */
#define _DE		(0x00 << 4)	// Data endpoint
#define _FE		(0x01 << 4)	// Feedback endpoint
#define _IE		(0x02 << 4)	// Implicit feedback Data endpoint

#define _ROM		USB_INPIPES_ROM
#define _RAM		USB_INPIPES_RAM

//These are the directional indicators used for the usb_transfer_one_packet()
//  function.
#define OUT_FROM_HOST	0
#define IN_TO_HOST	1

/*
 * CTRL_TRF_SETUP: Every setup packet has 8 bytes.  This structure
 * allows direct access to the various members of the control
 * transfer.
 */
typedef union __attribute__ ((packed)) _CTRL_TRF_SETUP
{
    /* Standard Device Requests */
    struct __attribute__ ((packed))
    {
        unsigned char bmRequestType; //from table 9-2 of USB2.0 spec
        unsigned char bRequest; //from table 9-2 of USB2.0 spec
        unsigned short wValue; //from table 9-2 of USB2.0 spec
        unsigned short wIndex; //from table 9-2 of USB2.0 spec
        unsigned short wLength; //from table 9-2 of USB2.0 spec
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned short W_Value; //from table 9-2 of USB2.0 spec, allows byte/bitwise access
        unsigned short W_Index; //from table 9-2 of USB2.0 spec, allows byte/bitwise access
        unsigned short W_Length; //from table 9-2 of USB2.0 spec, allows byte/bitwise access
    };
    struct __attribute__ ((packed))
    {
        unsigned Recipient:5;   //Device,Interface,Endpoint,Other
        unsigned RequestType:2; //Standard,Class,Vendor,Reserved
        unsigned DataDir:1;     //Host-to-device,Device-to-host
        unsigned :8;
        unsigned char bFeature;          //DEVICE_REMOTE_WAKEUP,ENDPOINT_HALT
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned char bDscIndex;        //For Configuration and String DSC Only
        unsigned char bDescriptorType;  //Device,Configuration,String
        unsigned short wLangID;         //Language ID
        unsigned :8;
        unsigned :8;
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned char bDevADR;		//Device Address 0-127
        unsigned char bDevADRH;         //Must equal zero
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned char bConfigurationValue;         //Configuration Value 0-255
        unsigned char bCfgRSD;           //Must equal zero (Reserved)
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned char bAltID;            //Alternate Setting Value 0-255
        unsigned char bAltID_H;          //Must equal zero
        unsigned char bIntfID;           //Interface Number Value 0-255
        unsigned char bIntfID_H;         //Must equal zero
        unsigned :8;
        unsigned :8;
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned char bEPID;             //Endpoint ID (Number & Direction)
        unsigned char bEPID_H;           //Must equal zero
        unsigned :8;
        unsigned :8;
    };
    struct __attribute__ ((packed))
    {
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned EPNum:4;       //Endpoint Number 0-15
        unsigned :3;
        unsigned EPDir:1;       //Endpoint Direction: 0-OUT, 1-IN
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };

    /* End: Standard Device Requests */

} CTRL_TRF_SETUP;

// Defintion of the PIPE structure
//  This structure is used to keep track of data that is sent out
//  of the stack automatically.
typedef struct __attribute__ ((packed))
{
    union __attribute__ ((packed))
    {
        //Various options of pointers that are available to
        // get the data from
        unsigned char *bRam;
        const unsigned char *bRom;
        unsigned short *wRam;
        const unsigned short *wRom;
    } pSrc;
    union __attribute__ ((packed))
    {
        struct __attribute__ ((packed))
        {
            //is this transfer from RAM or ROM?
            unsigned ctrl_trf_mem          :1;
            unsigned reserved              :5;
            //include a zero length packet after
            //data is done if data_size%ep_size = 0?
            unsigned includeZero           :1;
            //is this PIPE currently in use
            unsigned busy                  :1;
        } bits;
        unsigned char Val;
    } info;
    unsigned short wCount;
} IN_PIPE;

#define CTRL_TRF_RETURN void
#define CTRL_TRF_PARAMS void
typedef struct __attribute__ ((packed))
{
    union __attribute__ ((packed))
    {
        //Various options of pointers that are available to
        // get the data from
        unsigned char *bRam;
        unsigned short *wRam;
    } pDst;
    union __attribute__ ((packed))
    {
        struct __attribute__ ((packed))
        {
            unsigned reserved              :7;
            //is this PIPE currently in use
            unsigned busy                  :1;
        } bits;
        unsigned char Val;
    } info;
    unsigned short wCount;
    CTRL_TRF_RETURN (*pFunc)(CTRL_TRF_PARAMS);
} OUT_PIPE;

//Various options for setting the PIPES
#define USB_INPIPES_ROM            0x00     //Data comes from RAM
#define USB_INPIPES_RAM            0x01     //Data comes from ROM
#define USB_INPIPES_BUSY           0x80     //The PIPE is busy
#define USB_INPIPES_INCLUDE_ZERO   0x40     //include a trailing zero packet
#define USB_INPIPES_NO_DATA        0x00     //no data to send
#define USB_INPIPES_NO_OPTIONS     0x00     //no options set

#define USB_EP0_ROM            USB_INPIPES_ROM
#define USB_EP0_RAM            USB_INPIPES_RAM
#define USB_EP0_BUSY           USB_INPIPES_BUSY
#define USB_EP0_INCLUDE_ZERO   USB_INPIPES_INCLUDE_ZERO
#define USB_EP0_NO_DATA        USB_INPIPES_NO_DATA
#define USB_EP0_NO_OPTIONS     USB_INPIPES_NO_OPTIONS

/*
 * Standard Request Codes
 * USB 2.0 Spec Ref Table 9-4
 */
#define GET_STATUS  0
#define CLR_FEATURE 1
#define SET_FEATURE 3
#define SET_ADR     5
#define GET_DSC     6
#define SET_DSC     7
#define GET_CFG     8
#define SET_CFG     9
#define GET_INTF    10
#define SET_INTF    11
#define SYNCH_FRAME 12

/* Section: Standard Feature Selectors */
#define DEVICE_REMOTE_WAKEUP    0x01
#define ENDPOINT_HALT           0x00

/* Section: USB Device States - To be used with [BYTE usb_device_state] */

/* Detached is the state in which the device is not attached to the bus.  When
 * in the detached state a device should not have any pull-ups attached to either
 * the D+ or D- line.  This defintions is a return value of the
 * function usb_get_device_state()
 */
#define DETACHED_STATE          0x00

/* Attached is the state in which the device is attached ot the bus but the
 * hub/port that it is attached to is not yet configured. This defintions is a
 * return value of the function usb_get_device_state()
 */
#define ATTACHED_STATE          0x01

/* Powered is the state in which the device is attached to the bus and the
 * hub/port that it is attached to is configured. This defintions is a return
 * value of the function usb_get_device_state()
 */
#define POWERED_STATE           0x02

/* Default state is the state after the device receives a RESET command from
 * the host. This defintions is a return value of the function usb_get_device_state()
 */
#define DEFAULT_STATE           0x04

/* Address pending state is not an official state of the USB defined states.
 * This state is internally used to indicate that the device has received a
 * SET_ADDRESS command but has not received the STATUS stage of the transfer yet.
 * The device is should not switch addresses until after the STATUS stage is
 * complete.  This defintions is a return value of the function
 * usb_get_device_state()
 */
#define ADR_PENDING_STATE       0x08

/* Address is the state in which the device has its own specific address on the
 * bus. This defintions is a return value of the function usb_get_device_state().
 */
#define ADDRESS_STATE           0x10

/* Configured is the state where the device has been fully enumerated and is
 * operating on the bus.  The device is now allowed to excute its application
 * specific tasks.  It is also allowed to increase its current consumption to the
 * value specified in the configuration descriptor of the current configuration.
 * This defintions is a return value of the function usb_get_device_state().
 */
#define CONFIGURED_STATE        0x20

/* UCFG Initialization Parameters */
#define SetConfigurationOptions()   {U1CNFG1 = 0;}

/* UEPn Initialization Parameters */
#define EP_CTRL		0x0C	// Cfg Control pipe for this ep
#define EP_OUT		0x18	// Cfg OUT only pipe for this ep
#define EP_IN		0x14	// Cfg IN only pipe for this ep
#define EP_OUT_IN	0x1C	// Cfg both OUT & IN pipes for this ep
#define HSHK_EN		0x01	// Enable handshake packet
				// Handshake should be disable for isoch

#define USB_HANDSHAKE_ENABLED   0x01
#define USB_HANDSHAKE_DISABLED  0x00

#define USB_OUT_ENABLED         0x08
#define USB_OUT_DISABLED        0x00

#define USB_IN_ENABLED          0x04
#define USB_IN_DISABLED         0x00

#define USB_ALLOW_SETUP         0x00
#define USB_DISALLOW_SETUP      0x10

#define USB_STALL_ENDPOINT      0x02

// USB_HANDLE is a pointer to an entry in the BDT.  This pointer can be used
// to read the length of the last transfer, the status of the last transfer,
// and various other information.  Insure to initialize USB_HANDLE objects
// to NULL so that they are in a known state during their first usage.
#define USB_HANDLE volatile BDT_ENTRY*

// PIC32 supports only full ping-pong mode.
#define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG

/* Size of buffer for end-point EP0.
 * Valid Options: 8, 16, 32, or 64 bytes.
 * Using larger options take more SRAM, but
 * does not provide much advantage in most types
 * of applications.  Exceptions to this, are applications
 * that use EP0 IN or OUT for sending large amounts of
 * application related data.
 */
#ifndef USB_EP0_BUFF_SIZE
#   define USB_EP0_BUFF_SIZE	8
#endif

/*
 * Only one interface by default.
 */
#ifndef USB_MAX_NUM_INT
#   define USB_MAX_NUM_INT	1
#endif

// Definitions for the BDT
extern volatile BDT_ENTRY usb_buffer[(USB_MAX_EP_NUMBER + 1) * 4];

// Device descriptor
extern const USB_DEVICE_DESCRIPTOR usb_device;

// Array of configuration descriptors
extern const unsigned char *const usb_config[];
extern const unsigned char usb_config1_descriptor[];

// Array of string descriptors
extern const unsigned char *const usb_string[];

// Buffer for control transfers
extern volatile CTRL_TRF_SETUP usb_setup_pkt;           // 8-byte only

/* Control Transfer States */
#define WAIT_SETUP		0
#define CTRL_TRF_TX		1
#define CTRL_TRF_RX		2

/* v2.1 fix - Short Packet States - Used by Control Transfer Read  - CTRL_TRF_TX */
#define SHORT_PKT_NOT_USED	0
#define SHORT_PKT_PENDING	1
#define SHORT_PKT_SENT		2

/* USB PID: Token Types - See chapter 8 in the USB specification */
#define SETUP_TOKEN		0x0D    // 0b00001101
#define OUT_TOKEN		0x01    // 0b00000001
#define IN_TOKEN		0x09    // 0b00001001

/* bmRequestType Definitions */
#define HOST_TO_DEV		0
#define DEV_TO_HOST		1

#define STANDARD		0x00
#define CLASS			0x01
#define VENDOR			0x02

#define RCPT_DEV		0
#define RCPT_INTF		1
#define RCPT_EP			2
#define RCPT_OTH		3

extern unsigned usb_device_state;
extern unsigned usb_active_configuration;
extern IN_PIPE usb_in_pipe[1];
extern OUT_PIPE usb_out_pipe[1];

/*
  Function:
        void usb_device_tasks(void)

  Summary:
    This function is the main state machine of the USB device side stack.
    This function should be called periodically to receive and transmit
    packets through the stack. This function should be called preferably
    once every 100us during the enumeration process. After the enumeration
    process this function still needs to be called periodically to respond
    to various situations on the bus but is more relaxed in its time
    requirements. This function should also be called at least as fast as
    the OUT data expected from the PC.

  Description:
    This function is the main state machine of the USB device side stack.
    This function should be called periodically to receive and transmit
    packets through the stack. This function should be called preferably
    once every 100us during the enumeration process. After the enumeration
    process this function still needs to be called periodically to respond
    to various situations on the bus but is more relaxed in its time
    requirements. This function should also be called at least as fast as
    the OUT data expected from the PC.

    Typical usage:
    <code>
    void main(void)
    {
        usb_device_init();
        while (1)
        {
            usb_device_tasks();
            if ((usb_get_device_state() \< CONFIGURED_STATE) || usb_is_device_suspended())
            {
                // Either the device is not configured or we are suspended
                // so we don't want to do execute any application code
                continue;   //go back to the top of the while loop
            } else {
                // Otherwise we are free to run user application code.
                UserApplication();
            }
        }
    }
    </code>

  Conditions:
    None
  Remarks:
    This function should be called preferably once every 100us during the
    enumeration process. After the enumeration process this function still
    needs to be called periodically to respond to various situations on the
    bus but is more relaxed in its time requirements.
  */
void usb_device_tasks(void);

/*
    Function:
        void usb_device_init(void)

    Description:
        This function initializes the device stack it in the default state. The
        USB module will be completely reset including all of the internal
        variables, registers, and interrupt flags.

    Precondition:
        This function must be called before any of the other USB Device
        functions can be called, including usb_device_tasks().

    Parameters:
        None

    Return Values:
        None

    Remarks:
        None

  */
void usb_device_init(void);

/*
  Function:
        int usb_get_remote_wakeup_status(void)

  Summary:
    This function indicates if remote wakeup has been enabled by the host.
    Devices that support remote wakeup should use this function to
    determine if it should send a remote wakeup.

  Description:
    This function indicates if remote wakeup has been enabled by the host.
    Devices that support remote wakeup should use this function to
    determine if it should send a remote wakeup.

    If a device does not support remote wakeup (the Remote wakeup bit, bit
    5, of the bmAttributes field of the Configuration descriptor is set to
    1), then it should not send a remote wakeup command to the PC and this
    function is not of any use to the device. If a device does support
    remote wakeup then it should use this function as described below.

    If this function returns FALSE and the device is suspended, it should
    not issue a remote wakeup (resume).

    If this function returns TRUE and the device is suspended, it should
    issue a remote wakeup (resume).

    A device can add remote wakeup support by having the _RWU symbol added
    in the configuration descriptor (located in the usb_descriptors.c file
    in the project). This done in the 8th byte of the configuration
    descriptor. For example:

    <code lang="c">
    const unsigned char configDescriptor1[]={
        0x09,                           // Size
        USB_DESCRIPTOR_CONFIGURATION,   // descriptor type
        DESC_CONFIG_WORD(0x0022),       // Total length
        1,                              // Number of interfaces
        1,                              // Index value of this cfg
        0,                              // Configuration string index
        _DEFAULT | _SELF | _RWU,        // Attributes, see usb_device.h
        50,                             // Max power consumption in 2X mA(100mA)

        //The rest of the configuration descriptor should follow
    </code>

    For more information about remote wakeup, see the following section of
    the USB v2.0 specification available at www.usb.org:
        * Section 9.2.5.2
        * Table 9-10
        * Section 7.1.7.7
        * Section 9.4.5

  Conditions:
    None

  Return Values:
    TRUE -   Remote Wakeup has been enabled by the host
    FALSE -  Remote Wakeup is not currently enabled

  Remarks:
    None

  */
#define usb_get_remote_wakeup_status() usb_remote_wakeup

/*
  Function:
        unsigned usb_get_device_state(void)

  Summary:
    This function will return the current state of the device on the USB.
    This function should return CONFIGURED_STATE before an application
    tries to send information on the bus.
  Description:
    This function returns the current state of the device on the USB. This
    \function is used to determine when the device is ready to communicate
    on the bus. Applications should not try to send or receive data until
    this function returns CONFIGURED_STATE.

    It is also important that applications yield as much time as possible
    to the usb_device_tasks() function as possible while the this function
    \returns any value between ATTACHED_STATE through CONFIGURED_STATE.

    For more information about the various device states, please refer to
    the USB specification section 9.1 available from www.usb.org.

    Typical usage:
    <code>
    void main(void)
    {
        usb_device_init();
        while(1)
        {
            usb_device_tasks();
            if ((usb_get_device_state() \< CONFIGURED_STATE) || usb_is_device_suspended())
            {
                //Either the device is not configured or we are suspended
                //  so we don't want to do execute any application code
                continue;   //go back to the top of the while loop
            }
            else
            {
                //Otherwise we are free to run user application code.
                UserApplication();
            }
        }
    }
    </code>
  Conditions:
    None
  Return Values:
    DETACHED_STATE -     The device is not attached to the bus
    ATTACHED_STATE -     The device is attached to the bus but
    POWERED_STATE -      The device is not officially in the powered state
    DEFAULT_STATE -      The device has received a RESET from the host
    ADR_PENDING_STATE -  The device has received the SET_ADDRESS command but
                         hasn't received the STATUS stage of the command so
                         it is still operating on address 0.
    ADDRESS_STATE -      The device has an address assigned but has not
                         received a SET_CONFIGURATION command yet or has
                         received a SET_CONFIGURATION with a configuration
                         number of 0 (deconfigured)
    CONFIGURED_STATE -   the device has received a non\-zero
                         SET_CONFIGURATION command is now ready for
                         communication on the bus.
  Remarks:
    None
  */
#define usb_get_device_state() usb_device_state

/*
  Function:
        int usb_is_device_suspended(void)

  Summary:
    This function indicates if this device is currently suspended. When a
    device is suspended it will not be able to transfer data over the bus.
  Description:
    This function indicates if this device is currently suspended. When a
    device is suspended it will not be able to transfer data over the bus.
    This function can be used by the application to skip over section of
    code that do not need to exectute if the device is unable to send data
    over the bus.

    Typical usage:
    <code>
       void main(void)
       {
           usb_device_init();
           while(1)
           {
               usb_device_tasks();
               if ((usb_get_device_state() \< CONFIGURED_STATE) || usb_is_device_suspended())
               {
                   //Either the device is not configured or we are suspended
                   //  so we don't want to do execute any application code
                   continue;   //go back to the top of the while loop
               }
               else
               {
                   //Otherwise we are free to run user application code.
                   UserApplication();
               }
           }
       }
    </code>
  Conditions:
    None
  Return Values:
    TRUE -   this device is suspended.
    FALSE -  this device is not suspended.
  Remarks:
    None
 */
#define usb_is_device_suspended() (U1PWRC & PIC32_U1PWRC_USUSPEND)

void usb_ctrl_ep_service (void);
void usb_ctrl_trf_setup_handler (void);
void usb_ctrl_trf_in_handler (void);
void usb_check_std_request (void);
void usb_std_get_dsc_handler (void);
void usb_ctrl_ep_service_complete (void);
void usb_ctrl_trf_tx_service (void);
void usb_prepare_for_next_setup_trf (void);
void usb_ctrl_trf_rx_service (void);
void usb_std_set_cfg_handler (void);
void usb_std_get_status_handler (void);
void usb_std_feature_req_handler (void);
void usb_ctrl_trf_out_handler (void);

void usb_wake_from_suspend (void);
void usb_suspend (void);
void usb_stall_handler (void);
volatile USB_HANDLE usb_transfer_one_packet (unsigned ep, unsigned dir, unsigned char* data, unsigned len);
void usb_enable_endpoint (unsigned ep, unsigned options);
void usb_configure_endpoint (unsigned EPNum, unsigned direction);

#if defined(USB_DYNAMIC_EP_CONFIG)
    void usb_init_ep(unsigned char const* pConfig);
#else
    #define usb_init_ep(a)
#endif

/* Section: CALLBACKS */

/*
  Function:
      void usbcb_suspend(void)

  Summary:
    Call back that is invoked when a USB suspend is detected.
  Description:
    Call back that is invoked when a USB suspend is detected.

    \Example power saving code. Insert appropriate code here for the
    desired application behavior. If the microcontroller will be put to
    sleep, a process similar to that shown below may be used:

    \Example Psuedo Code:
    <code>
    ConfigureIOPinsForLowPower();
    SaveStateOfAllInterruptEnableBits();
    DisableAllInterruptEnableBits();

    //should enable at least USBActivityIF as a wake source
    EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();

    Sleep();

    //Preferrably, this should be done in the
    //  usbcb_wake_from_suspend() function instead.
    RestoreStateOfAllPreviouslySavedInterruptEnableBits();

    //Preferrably, this should be done in the
    //  usbcb_wake_from_suspend() function instead.
    RestoreIOPinsToNormal();
    </code>

    IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here. This
    bit is cleared inside the usb_device.c file. Clearing USBActivityIF
    here will cause things to not work as intended.
  Conditions:
    None

    Paramters: None
  Side Effects:
    None

    Remark: None
  */
void usbcb_suspend (void);

/*
 Function:
   void usbcb_wake_from_suspend (void)

 Summary:
   This call back is invoked when a wakeup from USB suspend
   is detected.

 Description:
   The host may put USB peripheral devices in low power
   suspend mode (by "sending" 3+ms of idle).  Once in suspend
   mode, the host may wake the device back up by sending non-
   idle state signalling.

   This call back is invoked when a wakeup from USB suspend
   is detected.

   If clock switching or other power savings measures were taken when
   executing the usbcb_suspend() function, now would be a good time to
   switch back to normal full power run mode conditions.  The host allows
   a few milliseconds of wakeup time, after which the device must be
   fully back to normal, and capable of receiving and processing USB
   packets.  In order to do this, the USB module must receive proper
   clocking (IE: 48MHz clock must be available to SIE for full speed USB
   operation).

 PreCondition:  None

 Parameters:    None

 Return Values: None

 Remarks:       None
 */
void usbcb_wake_from_suspend (void);

/*
  Function:
    void usbcb_sof_handler (void)

  Summary:
    This callback is called when a SOF packet is received by the host.
    (optional)

  Description:
    This callback is called when a SOF packet is received by the host.
    (optional)

    The USB host sends out a SOF packet to full-speed
    devices every 1 ms. This interrupt may be useful
    for isochronous pipes. End designers should
    implement callback routine as necessary.

  PreCondition:
    None

  Parameters:
    None

  Return Values:
    None

  Remarks:
    None
 */
void usbcb_sof_handler (void);

/*
  Function:
    void usbcb_error_handler (void)

  Summary:
    This callback is called whenever a USB error occurs. (optional)

  Description:
    This callback is called whenever a USB error occurs. (optional)

    The purpose of this callback is mainly for
    debugging during development. Check UEIR to see
    which error causes the interrupt.

  PreCondition:
    None

  Parameters:
    None

  Return Values:
    None

  Remarks:
    No need to clear UEIR to 0 here.
    Callback caller is already doing that.

    Typically, user firmware does not need to do anything special
    if a USB error occurs.  For example, if the host sends an OUT
    packet to your device, but the packet gets corrupted (ex:
    because of a bad connection, or the user unplugs the
    USB cable during the transmission) this will typically set
    one or more USB error interrupt flags.  Nothing specific
    needs to be done however, since the SIE will automatically
    send a "NAK" packet to the host.  In response to this, the
    host will normally retry to send the packet again, and no
    data loss occurs.  The system will typically recover
    automatically, without the need for application firmware
    intervention.

    Nevertheless, this callback function is provided, such as
    for debugging purposes.
 */
void usbcb_error_handler (void);

/*
  Function:
      void usbcb_check_other_req (void)

  Summary:
    This function is called whenever a request comes over endpoint 0 (the
    control endpoint) that the stack does not know how to handle.
  Description:
    When SETUP packets arrive from the host, some firmware must process the
    request and respond appropriately to fulfill the request. Some of the
    SETUP packets will be for standard USB "chapter 9" (as in, fulfilling
    chapter 9 of the official USB specifications) requests, while others
    may be specific to the USB device class that is being implemented. For
    \example, a HID class device needs to be able to respond to "GET
    REPORT" type of requests. This is not a standard USB chapter 9 request,
    and therefore not handled by usb_device.c. Instead this request should
    be handled by class specific firmware, such as that contained in
    usb_function_hid.c.

    Typical Usage:
    <code>
    void usbcb_check_other_req (void)
    {
        //Since the stack didn't handle the request I need to check
        //  my class drivers to see if it is for them
        USBCheckMSDRequest();
    }
    </code>
  Conditions:
    None
  Remarks:
    None
  */
void usbcb_check_other_req (void);

/*
  Function:
    void usbcb_std_set_dsc_handler (void)

  Summary:
    This callback is called when a SET_DESCRIPTOR request is received (optional)

  Description:
    The usbcb_std_set_dsc_handler() callback function is
    called when a SETUP, bRequest: SET_DESCRIPTOR request
    arrives.  Typically SET_DESCRIPTOR requests are
    not used in most applications, and it is
    optional to support this type of request.

  PreCondition:
    None

  Parameters:
    None

  Return Values:
    None

  Remark:            None
 */
void usbcb_std_set_dsc_handler (void);

/*
  Function:
      void usbcb_init_ep (void)

  Summary:
    This function is called whenever the device receives a
    SET_CONFIGURATION request.
  Description:
    This function is called when the device becomes initialized, which
    occurs after the host sends a SET_CONFIGURATION (wValue not = 0)
    request. This callback function should initialize the endpoints for the
    device's usage according to the current configuration.

    Typical Usage:
    <code>
    void usbcb_init_ep (void)
    {
        usb_enable_endpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        USBMSDInit();
    }
    </code>
  Conditions:
    None
  Remarks:
    None
  */
void usbcb_init_ep (void);

/*
  Function:
        void usbcb_send_resume (void)

  Summary:
    This function should be called to initiate a remote wakeup. (optional)
  Description:
    The USB specifications allow some types of USB peripheral devices to
    wake up a host PC (such as if it is in a low power suspend to RAM
    state). This can be a very useful feature in some USB applications,
    such as an Infrared remote control receiver. If a user presses the
    "power" button on a remote control, it is nice that the IR receiver can
    detect this signalling, and then send a USB "command" to the PC to wake
    up.

    The usbcb_send_resume() "callback" function is used to send this special
    USB signalling which wakes up the PC. This function may be called by
    application firmware to wake up the PC. This function should only be
    called when:

      1. The USB driver used on the host PC supports the remote wakeup
         capability.
      2. The USB configuration descriptor indicates the device is remote
         wakeup capable in the bmAttributes field. (see usb_descriptors.c and
         _RWU)
      3. The USB host PC is currently sleeping, and has previously sent
         your device a SET FEATURE setup packet which "armed" the remote wakeup
         capability. (see usb_get_remote_wakeup_status())

    This callback should send a RESUME signal that has the period of
    1-15ms.

    Typical Usage:
    <code>
    if ((usb_device_state == CONFIGURED_STATE)
        &amp;&amp; usb_is_device_suspended()
        &amp;&amp; (usb_get_remote_wakeup_status() == TRUE))
    {
        if (ButtonPressed)
        {
            // Wake up the USB module from suspend
            usb_wake_from_suspend();

            // Issue a remote wakeup command on the bus
            usbcb_send_resume();
        }
    }
    </code>
  Conditions:
    None
  Remarks:
    A user can switch to primary first by calling usbcb_wake_from_suspend() if
    required/desired.

    The modifiable section in this routine should be changed to meet the
    application needs. Current implementation temporary blocks other
    functions from executing for a period of 1-13 ms depending on the core
    frequency.

    According to USB 2.0 specification section 7.1.7.7, "The remote wakeup
    device must hold the resume signaling for at lest 1 ms but for no more
    than 15 ms." The idea here is to use a delay counter loop, using a
    common value that would work over a wide range of core frequencies.
    That value selected is 1800. See table below:
    <table>
    Core Freq(MHz)   MIP (for PIC18)   RESUME Signal Period (ms)
    ---------------  ----------------  --------------------------
    48               12                1.05
    4                1                 12.6
    </table>
      * These timing could be incorrect when using code optimization or
        extended instruction mode, or when having other interrupts enabled.
        Make sure to verify using the MPLAB SIM's Stopwatch and verify the
        actual signal on an oscilloscope.
      * These timing numbers should be recalculated when using PIC24 or
        PIC32 as they have different clocking structures.
      * A timer can be used in place of the blocking loop if desired.

*/
void usbcb_send_resume (void);

/*
  Function:
    void usbcb_ep0_data_received(void)

  Summary:
    This function is called whenever a EP0 data
    packet is received. (optional)

  Description:
    This function is called whenever a EP0 data
    packet is received.  This gives the user (and
    thus the various class examples a way to get
    data that is received via the control endpoint.
    This function needs to be used in conjunction
    with the usbcb_check_other_req() function since
    the usbcb_check_other_req() function is the apps
    method for getting the initial control transfer
    before the data arrives.

  PreCondition:
    ENABLE_EP0_DATA_RECEIVED_CALLBACK must be
    defined already (in target.cfg)

  Parameters:
    None

  Return Values:
    None

  Remarks:
    None
*/
void usbcb_ep0_data_received (void);




/* Section: MACROS */

#define DESC_CONFIG_BYTE(a) (a)
#define DESC_CONFIG_WORD(a) (a&0xFF),((a>>8)&0xFF)
#define DESC_CONFIG_DWORD(a) (a&0xFF),((a>>8)&0xFF),((a>>16)&0xFF),((a>>24)&0xFF)

/*
  Function:
    int usb_handle_busy(USB_HANDLE handle)

  Summary:
    Checks to see if the input handle is busy

  Description:
    Checks to see if the input handle is busy

    Typical Usage
    <code>
    //make sure that the last transfer isn't busy by checking the handle
    if (! usb_handle_busy(handle))
    {
        //Send the data contained in the INPacket[] array out on
        //  endpoint USBGEN_EP_NUM
        handle = USBGenWrite (USBGEN_EP_NUM, (unsigned char*) &INPacket[0],  sizeof(INPacket));
    }
    </code>

  Conditions:
    None
  Input:
    USB_HANDLE handle -  handle of the transfer that you want to check the
                         status of
  Return Values:
    TRUE -   The specified handle is busy
    FALSE -  The specified handle is free and available for a transfer
  Remarks:
    None
  */
#define usb_handle_busy(handle) (handle != 0 && handle->STAT.UOWN)

/*
    Function:
        unsigned short usb_handle_get_length(USB_HANDLE handle)

    Summary:
        Retrieves the length of the destination buffer of the input
        handle

    Description:
        Retrieves the length of the destination buffer of the input
        handle

    PreCondition:
        None

    Parameters:
        USB_HANDLE handle - the handle to the transfer you want the
        address for.

    Return Values:
        unsigned short - length of the current buffer that the input handle
        points to.  If the transfer is complete then this is the
        length of the data transmitted or the length of data
        actually received.

    Remarks:
        None

 */
#define usb_handle_get_length(handle) (handle->CNT)

/*
    Function:
        unsigned short usb_handle_get_addr(USB_HANDLE)

    Summary:
        Retrieves the address of the destination buffer of the input
        handle

    Description:
        Retrieves the address of the destination buffer of the input
        handle

    PreCondition:
        None

    Parameters:
        USB_HANDLE handle - the handle to the transfer you want the
        address for.

    Return Values:
        unsigned short - address of the current buffer that the input handle
        points to.

    Remarks:
        None

 */
#define usb_handle_get_addr(handle) (handle->ADR)

/*
    Function:
        void usb_ep0_set_source_ram(unsigned char* src)

    Summary:
        Sets the address of the data to send over the
        control endpoint

    PreCondition:
        None

    Paramters:
        src - address of the data to send

    Return Values:
        None

    Remarks:
        None

 */
#define usb_ep0_set_source_ram(src) usb_in_pipe[0].pSrc.bRam = src

/*
    Function:
        void usb_ep0_set_source_rom(unsigned char* src)

    Summary:
        Sets the address of the data to send over the
        control endpoint

    PreCondition:
        None

    Parameters:
        src - address of the data to send

    Return Values:
        None

    Remarks:
        None

 */
#define usb_ep0_set_source_rom(src) usb_in_pipe[0].pSrc.bRom = src

/*
    Function:
        void usb_ep0_transmit(unsigned char options)

    Summary:
        Sets the address of the data to send over the
        control endpoint

    PreCondition:
        None

    Paramters:
        options - the various options that you want
                  when sending the control data. Options are:
                       USB_INPIPES_ROM
                       USB_INPIPES_RAM
                       USB_INPIPES_BUSY
                       USB_INPIPES_INCLUDE_ZERO
                       USB_INPIPES_NO_DATA
                       USB_INPIPES_NO_OPTIONS

    Return Values:
        None

    Remarks:
        None

 */
#define usb_ep0_transmit(options) usb_in_pipe[0].info.Val = options | USB_INPIPES_BUSY

/*
    Function:
        void usb_ep0_set_size(unsigned short size)

    Summary:
        Sets the size of the data to send over the
        control endpoint

    PreCondition:
        None

    Parameters:
        size - the size of the data needing to be transmitted

    Return Values:
        None

    Remarks:
        None

 */
#define usb_ep0_set_size(size) usb_in_pipe[0].wCount = size

/*
    Function:
        void usb_ep0_send_ram_ptr(unsigned char* src, unsigned short size, unsigned char Options)

    Summary:
        Sets the source, size, and options of the data
        you wish to send from a RAM source

    PreCondition:
        None

    Parameters:
        src - address of the data to send
        size - the size of the data needing to be transmitted
        options - the various options that you want
        when sending the control data. Options are:
        USB_INPIPES_ROM
        USB_INPIPES_RAM
        USB_INPIPES_BUSY
        USB_INPIPES_INCLUDE_ZERO
        USB_INPIPES_NO_DATA
        USB_INPIPES_NO_OPTIONS

    Return Values:
        None

    Remarks:
        None

 */
#define usb_ep0_send_ram_ptr(src,size,options)  {usb_ep0_set_source_ram(src);usb_ep0_set_size(size);usb_ep0_transmit(options | USB_EP0_RAM);}

/*
    Function:
        void usb_ep0_send_rom_ptr(unsigned char* src, unsigned short size, unsigned char Options)

    Summary:
        Sets the source, size, and options of the data
        you wish to send from a ROM source

    PreCondition:
        None

    Parameters:
        src - address of the data to send
        size - the size of the data needing to be transmitted
        options - the various options that you want
        when sending the control data. Options are:
        USB_INPIPES_ROM
        USB_INPIPES_RAM
        USB_INPIPES_BUSY
        USB_INPIPES_INCLUDE_ZERO
        USB_INPIPES_NO_DATA
        USB_INPIPES_NO_OPTIONS

    Return Values:
        None

    Remarks:
        None

 */
#define usb_ep0_send_rom_ptr(src,size,options)  {usb_ep0_set_source_rom(src);usb_ep0_set_size(size);usb_ep0_transmit(options | USB_EP0_ROM);}

/*
    Function:
        USB_HANDLE usb_tx_one_packet(unsigned char ep, unsigned char* data, unsigned short len)

    Summary:
        Sends the specified data out the specified endpoint

    PreCondition:
        None

    Parameters:
        ep - the endpoint you want to send the data out of
        data - the data that you wish to send
        len - the length of the data that you wish to send

    Return Values:
        USB_HANDLE - a handle for the transfer.  This information
        should be kept to track the status of the transfer

    Remarks:
        None

 */
#define usb_tx_one_packet(ep, data, len)    usb_transfer_one_packet(ep, IN_TO_HOST, data, len)

/*
    Function:
        void usb_rx_one_packet(unsigned char ep, unsigned char* data, unsigned short len)

    Summary:
        Receives the specified data out the specified endpoint

    PreCondition:
        None

    Parameters:
        ep - the endpoint you want to receive the data into
        data - where the data will go when it arrives
        len - the length of the data that you wish to receive

    Return Values:
        None

    Remarks:
        None

 */
#define usb_rx_one_packet(ep, data, len)    usb_transfer_one_packet(ep, OUT_FROM_HOST, data, len)

/*
    Function:
        void usb_stall_endpoint(unsigned ep, unsigned dir)

    Summary:
         STALLs the specified endpoint

    PreCondition:
        None

    Parameters:
        unsigned char ep - the endpoint the data will be transmitted on
        unsigned char dir - the direction of the transfer

    Return Values:
        None

    Remarks:
        None
 */
void usb_stall_endpoint(unsigned ep, unsigned dir);

#if (USB_PING_PONG_MODE == USB_PING_PONG__NO_PING_PONG)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0000   // Used in USB Device Mode only
    #define USB_NEXT_EP0_IN_PING_PONG 0x0000    // Used in USB Device Mode only
    #define USB_NEXT_PING_PONG 0x0000           // Used in USB Device Mode only
    #define EP0_OUT_EVEN    0                   // Used in USB Device Mode only
    #define EP0_OUT_ODD     0                   // Used in USB Device Mode only
    #define EP0_IN_EVEN     1                   // Used in USB Device Mode only
    #define EP0_IN_ODD      1                   // Used in USB Device Mode only
    #define EP1_OUT_EVEN    2                   // Used in USB Device Mode only
    #define EP1_OUT_ODD     2                   // Used in USB Device Mode only
    #define EP1_IN_EVEN     3                   // Used in USB Device Mode only
    #define EP1_IN_ODD      3                   // Used in USB Device Mode only
    #define EP2_OUT_EVEN    4                   // Used in USB Device Mode only
    #define EP2_OUT_ODD     4                   // Used in USB Device Mode only
    #define EP2_IN_EVEN     5                   // Used in USB Device Mode only
    #define EP2_IN_ODD      5                   // Used in USB Device Mode only
    #define EP3_OUT_EVEN    6                   // Used in USB Device Mode only
    #define EP3_OUT_ODD     6                   // Used in USB Device Mode only
    #define EP3_IN_EVEN     7                   // Used in USB Device Mode only
    #define EP3_IN_ODD      7                   // Used in USB Device Mode only
    #define EP4_OUT_EVEN    8                   // Used in USB Device Mode only
    #define EP4_OUT_ODD     8                   // Used in USB Device Mode only
    #define EP4_IN_EVEN     9                   // Used in USB Device Mode only
    #define EP4_IN_ODD      9                   // Used in USB Device Mode only
    #define EP5_OUT_EVEN    10                  // Used in USB Device Mode only
    #define EP5_OUT_ODD     10                  // Used in USB Device Mode only
    #define EP5_IN_EVEN     11                  // Used in USB Device Mode only
    #define EP5_IN_ODD      11                  // Used in USB Device Mode only
    #define EP6_OUT_EVEN    12                  // Used in USB Device Mode only
    #define EP6_OUT_ODD     12                  // Used in USB Device Mode only
    #define EP6_IN_EVEN     13                  // Used in USB Device Mode only
    #define EP6_IN_ODD      13                  // Used in USB Device Mode only
    #define EP7_OUT_EVEN    14                  // Used in USB Device Mode only
    #define EP7_OUT_ODD     14                  // Used in USB Device Mode only
    #define EP7_IN_EVEN     15                  // Used in USB Device Mode only
    #define EP7_IN_ODD      15                  // Used in USB Device Mode only
    #define EP8_OUT_EVEN    16                  // Used in USB Device Mode only
    #define EP8_OUT_ODD     16                  // Used in USB Device Mode only
    #define EP8_IN_EVEN     17                  // Used in USB Device Mode only
    #define EP8_IN_ODD      17                  // Used in USB Device Mode only
    #define EP9_OUT_EVEN    18                  // Used in USB Device Mode only
    #define EP9_OUT_ODD     18                  // Used in USB Device Mode only
    #define EP9_IN_EVEN     19                  // Used in USB Device Mode only
    #define EP9_IN_ODD      19                  // Used in USB Device Mode only
    #define EP10_OUT_EVEN   20                  // Used in USB Device Mode only
    #define EP10_OUT_ODD    20                  // Used in USB Device Mode only
    #define EP10_IN_EVEN    21                  // Used in USB Device Mode only
    #define EP10_IN_ODD     21                  // Used in USB Device Mode only
    #define EP11_OUT_EVEN   22                  // Used in USB Device Mode only
    #define EP11_OUT_ODD    22                  // Used in USB Device Mode only
    #define EP11_IN_EVEN    23                  // Used in USB Device Mode only
    #define EP11_IN_ODD     23                  // Used in USB Device Mode only
    #define EP12_OUT_EVEN   24                  // Used in USB Device Mode only
    #define EP12_OUT_ODD    24                  // Used in USB Device Mode only
    #define EP12_IN_EVEN    25                  // Used in USB Device Mode only
    #define EP12_IN_ODD     25                  // Used in USB Device Mode only
    #define EP13_OUT_EVEN   26                  // Used in USB Device Mode only
    #define EP13_OUT_ODD    26                  // Used in USB Device Mode only
    #define EP13_IN_EVEN    27                  // Used in USB Device Mode only
    #define EP13_IN_ODD     27                  // Used in USB Device Mode only
    #define EP14_OUT_EVEN   28                  // Used in USB Device Mode only
    #define EP14_OUT_ODD    28                  // Used in USB Device Mode only
    #define EP14_IN_EVEN    29                  // Used in USB Device Mode only
    #define EP14_IN_ODD     29                  // Used in USB Device Mode only
    #define EP15_OUT_EVEN   30                  // Used in USB Device Mode only
    #define EP15_OUT_ODD    30                  // Used in USB Device Mode only
    #define EP15_IN_EVEN    31                  // Used in USB Device Mode only
    #define EP15_IN_ODD     31                  // Used in USB Device Mode only

    #define EP(ep,dir,pp) (2*ep+dir)            // Used in USB Device Mode only

    #define BD(ep,dir,pp)   ((8 * ep) + (4 * dir))      // Used in USB Device Mode only

#elif (USB_PING_PONG_MODE == USB_PING_PONG__EP0_OUT_ONLY)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0004
    #define USB_NEXT_EP0_IN_PING_PONG 0x0000
    #define USB_NEXT_PING_PONG 0x0000
    #define EP0_OUT_EVEN    0
    #define EP0_OUT_ODD     1
    #define EP0_IN_EVEN     2
    #define EP0_IN_ODD      2
    #define EP1_OUT_EVEN    3
    #define EP1_OUT_ODD     3
    #define EP1_IN_EVEN     4
    #define EP1_IN_ODD      4
    #define EP2_OUT_EVEN    5
    #define EP2_OUT_ODD     5
    #define EP2_IN_EVEN     6
    #define EP2_IN_ODD      6
    #define EP3_OUT_EVEN    7
    #define EP3_OUT_ODD     7
    #define EP3_IN_EVEN     8
    #define EP3_IN_ODD      8
    #define EP4_OUT_EVEN    9
    #define EP4_OUT_ODD     9
    #define EP4_IN_EVEN     10
    #define EP4_IN_ODD      10
    #define EP5_OUT_EVEN    11
    #define EP5_OUT_ODD     11
    #define EP5_IN_EVEN     12
    #define EP5_IN_ODD      12
    #define EP6_OUT_EVEN    13
    #define EP6_OUT_ODD     13
    #define EP6_IN_EVEN     14
    #define EP6_IN_ODD      14
    #define EP7_OUT_EVEN    15
    #define EP7_OUT_ODD     15
    #define EP7_IN_EVEN     16
    #define EP7_IN_ODD      16
    #define EP8_OUT_EVEN    17
    #define EP8_OUT_ODD     17
    #define EP8_IN_EVEN     18
    #define EP8_IN_ODD      18
    #define EP9_OUT_EVEN    19
    #define EP9_OUT_ODD     19
    #define EP9_IN_EVEN     20
    #define EP9_IN_ODD      20
    #define EP10_OUT_EVEN   21
    #define EP10_OUT_ODD    21
    #define EP10_IN_EVEN    22
    #define EP10_IN_ODD     22
    #define EP11_OUT_EVEN   23
    #define EP11_OUT_ODD    23
    #define EP11_IN_EVEN    24
    #define EP11_IN_ODD     24
    #define EP12_OUT_EVEN   25
    #define EP12_OUT_ODD    25
    #define EP12_IN_EVEN    26
    #define EP12_IN_ODD     26
    #define EP13_OUT_EVEN   27
    #define EP13_OUT_ODD    27
    #define EP13_IN_EVEN    28
    #define EP13_IN_ODD     28
    #define EP14_OUT_EVEN   29
    #define EP14_OUT_ODD    29
    #define EP14_IN_EVEN    30
    #define EP14_IN_ODD     30
    #define EP15_OUT_EVEN   31
    #define EP15_OUT_ODD    31
    #define EP15_IN_EVEN    32
    #define EP15_IN_ODD     32

    #define EP(ep,dir,pp) (2*ep+dir+(((ep==0)&&(dir==0))?pp:2))
    #define BD(ep,dir,pp) (4*(ep+dir+(((ep==0)&&(dir==0))?pp:2)))

#elif (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0008
    #define USB_NEXT_EP0_IN_PING_PONG 0x0008
    #define USB_NEXT_PING_PONG 0x0008

    #define EP0_OUT_EVEN    0
    #define EP0_OUT_ODD     1
    #define EP0_IN_EVEN     2
    #define EP0_IN_ODD      3
    #define EP1_OUT_EVEN    4
    #define EP1_OUT_ODD     5
    #define EP1_IN_EVEN     6
    #define EP1_IN_ODD      7
    #define EP2_OUT_EVEN    8
    #define EP2_OUT_ODD     9
    #define EP2_IN_EVEN     10
    #define EP2_IN_ODD      11
    #define EP3_OUT_EVEN    12
    #define EP3_OUT_ODD     13
    #define EP3_IN_EVEN     14
    #define EP3_IN_ODD      15
    #define EP4_OUT_EVEN    16
    #define EP4_OUT_ODD     17
    #define EP4_IN_EVEN     18
    #define EP4_IN_ODD      19
    #define EP5_OUT_EVEN    20
    #define EP5_OUT_ODD     21
    #define EP5_IN_EVEN     22
    #define EP5_IN_ODD      23
    #define EP6_OUT_EVEN    24
    #define EP6_OUT_ODD     25
    #define EP6_IN_EVEN     26
    #define EP6_IN_ODD      27
    #define EP7_OUT_EVEN    28
    #define EP7_OUT_ODD     29
    #define EP7_IN_EVEN     30
    #define EP7_IN_ODD      31
    #define EP8_OUT_EVEN    32
    #define EP8_OUT_ODD     33
    #define EP8_IN_EVEN     34
    #define EP8_IN_ODD      35
    #define EP9_OUT_EVEN    36
    #define EP9_OUT_ODD     37
    #define EP9_IN_EVEN     38
    #define EP9_IN_ODD      39
    #define EP10_OUT_EVEN   40
    #define EP10_OUT_ODD    41
    #define EP10_IN_EVEN    42
    #define EP10_IN_ODD     43
    #define EP11_OUT_EVEN   44
    #define EP11_OUT_ODD    45
    #define EP11_IN_EVEN    46
    #define EP11_IN_ODD     47
    #define EP12_OUT_EVEN   48
    #define EP12_OUT_ODD    49
    #define EP12_IN_EVEN    50
    #define EP12_IN_ODD     51
    #define EP13_OUT_EVEN   52
    #define EP13_OUT_ODD    53
    #define EP13_IN_EVEN    54
    #define EP13_IN_ODD     55
    #define EP14_OUT_EVEN   56
    #define EP14_OUT_ODD    57
    #define EP14_IN_EVEN    58
    #define EP14_IN_ODD     59
    #define EP15_OUT_EVEN   60
    #define EP15_OUT_ODD    61
    #define EP15_IN_EVEN    62
    #define EP15_IN_ODD     63

    #define EP(ep,dir,pp) (4*ep+2*dir+pp)

    #define BD(ep,dir,pp) (8*(4*ep+2*dir+pp))

#elif (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0000
    #define USB_NEXT_EP0_IN_PING_PONG 0x0000
    #define USB_NEXT_PING_PONG 0x0004
    #define EP0_OUT_EVEN    0
    #define EP0_OUT_ODD     0
    #define EP0_IN_EVEN     1
    #define EP0_IN_ODD      1
    #define EP1_OUT_EVEN    2
    #define EP1_OUT_ODD     3
    #define EP1_IN_EVEN     4
    #define EP1_IN_ODD      5
    #define EP2_OUT_EVEN    6
    #define EP2_OUT_ODD     7
    #define EP2_IN_EVEN     8
    #define EP2_IN_ODD      9
    #define EP3_OUT_EVEN    10
    #define EP3_OUT_ODD     11
    #define EP3_IN_EVEN     12
    #define EP3_IN_ODD      13
    #define EP4_OUT_EVEN    14
    #define EP4_OUT_ODD     15
    #define EP4_IN_EVEN     16
    #define EP4_IN_ODD      17
    #define EP5_OUT_EVEN    18
    #define EP5_OUT_ODD     19
    #define EP5_IN_EVEN     20
    #define EP5_IN_ODD      21
    #define EP6_OUT_EVEN    22
    #define EP6_OUT_ODD     23
    #define EP6_IN_EVEN     24
    #define EP6_IN_ODD      25
    #define EP7_OUT_EVEN    26
    #define EP7_OUT_ODD     27
    #define EP7_IN_EVEN     28
    #define EP7_IN_ODD      29
    #define EP8_OUT_EVEN    30
    #define EP8_OUT_ODD     31
    #define EP8_IN_EVEN     32
    #define EP8_IN_ODD      33
    #define EP9_OUT_EVEN    34
    #define EP9_OUT_ODD     35
    #define EP9_IN_EVEN     36
    #define EP9_IN_ODD      37
    #define EP10_OUT_EVEN   38
    #define EP10_OUT_ODD    39
    #define EP10_IN_EVEN    40
    #define EP10_IN_ODD     41
    #define EP11_OUT_EVEN   42
    #define EP11_OUT_ODD    43
    #define EP11_IN_EVEN    44
    #define EP11_IN_ODD     45
    #define EP12_OUT_EVEN   46
    #define EP12_OUT_ODD    47
    #define EP12_IN_EVEN    48
    #define EP12_IN_ODD     49
    #define EP13_OUT_EVEN   50
    #define EP13_OUT_ODD    51
    #define EP13_IN_EVEN    52
    #define EP13_IN_ODD     53
    #define EP14_OUT_EVEN   54
    #define EP14_OUT_ODD    55
    #define EP14_IN_EVEN    56
    #define EP14_IN_ODD     57
    #define EP15_OUT_EVEN   58
    #define EP15_OUT_ODD    59
    #define EP15_IN_EVEN    60
    #define EP15_IN_ODD     61

    #define EP(ep,dir,pp) (4*ep+2*dir+((ep==0)?0:(pp-2)))
    #define BD(ep,dir,pp) (4*(4*ep+2*dir+((ep==0)?0:(pp-2))))

#else
    #error "No ping pong mode defined."
#endif

extern int usb_remote_wakeup;

#endif //USBD_H
