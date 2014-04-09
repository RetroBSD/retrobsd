/*
 * USB Hardware Abstraction Layer (HAL)  (Header File)
 *
 * This file abstracts the hardware interface.  The USB stack firmware can be
 * compiled to work on different USB microcontrollers, such as PIC18 and PIC24.
 * The USB related special function registers and bit names are generally very
 * similar between the device families, but small differences in naming exist.
 *
 * In order to make the same set of firmware work accross the device families,
 * when modifying SFR contents, a slightly abstracted name is used, which is
 * then "mapped" to the appropriate real name in the usb_hal_picxx.h header.
 *
 * Make sure to include the correct version of the usb_hal_picxx.h file for
 * the microcontroller family which will be used.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PICmicro� Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
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
#ifndef USB_HAL_PIC32_H
#define USB_HAL_PIC32_H

/* Buffer Descriptor Status Register Initialization Parameters */

//The _BSTALL definition is changed from 0x04 to 0x00 to
// fix a difference in the PIC18 and PIC24 definitions of this
// bit.  This should be changed back once the definitions are
// synced.
#define _BSTALL     0x04        //Buffer Stall enable
#define _DTSEN      0x08        //Data Toggle Synch enable
#define _DAT0       0x00        //DATA0 packet expected next
#define _DAT1       0x40        //DATA1 packet expected next
#define _DTSMASK    0x40        //DTS Mask
#define _USIE       0x80        //SIE owns buffer
#define _UCPU       0x00        //CPU owns buffer

#define _STAT_MASK  0xFC

// Buffer Descriptor Status Register layout.
typedef union __attribute__ ((packed)) _BD_STAT
{
    struct __attribute__ ((packed)){
        unsigned            :2;
        unsigned    BSTALL  :1;     //Buffer Stall Enable
        unsigned    DTSEN   :1;     //Data Toggle Synch Enable
        unsigned            :2;     //Reserved - write as 00
        unsigned    DTS     :1;     //Data Toggle Synch Value
        unsigned    UOWN    :1;     //USB Ownership
    };
    struct __attribute__ ((packed)){
        unsigned            :2;
        unsigned    PID0    :1;
        unsigned    PID1    :1;
        unsigned    PID2    :1;
        unsigned    PID3    :1;

    };
    struct __attribute__ ((packed)){
        unsigned            :2;
        unsigned    PID     :4;         //Packet Identifier
    };
    unsigned short  Val;
} BD_STAT;

// BDT Entry Layout
typedef union __attribute__ ((packed))__BDT
{
    struct __attribute__ ((packed))
    {
        BD_STAT     STAT;
        unsigned    CNT:10;
        unsigned char *ADR;		//Buffer Address
    };
    struct __attribute__ ((packed))
    {
        unsigned    res  :16;
        unsigned    count:10;
    };
    unsigned int    w[2];
    unsigned short  v[4];
    unsigned long long Val;
} BDT_ENTRY;

#define USTAT_EP0_PP_MASK   ~0x04
#define USTAT_EP_MASK       0xFC
#define USTAT_EP0_OUT       0x00
#define USTAT_EP0_OUT_EVEN  0x00
#define USTAT_EP0_OUT_ODD   0x04

#define USTAT_EP0_IN        0x08
#define USTAT_EP0_IN_EVEN   0x08
#define USTAT_EP0_IN_ODD    0x0C

#define UEP_STALL 0x0002

//#define USB_PING_PONG__NO_PING_PONG	0x00
//#define USB_PING_PONG__EP0_OUT_ONLY	0x01
#define USB_PING_PONG__FULL_PING_PONG	0x02
//#define USB_PING_PONG__ALL_BUT_EP0	0x03

/*
 * Translate virtual address to physical one.
 * Only for fixed mapping.
 */
static inline void *ConvertToPhysicalAddress (volatile void *addr)
{
    unsigned virt = (unsigned) addr;
    unsigned phys;

    if (virt & 0x80000000) {
        if (virt & 0x40000000) {
            // kseg2 or kseg3 - no mapping
            phys = virt;
        } else {
            // kseg0 или kseg1, cut bits A[31:29]
            phys = virt & 0x1fffffff;
        }
    } else {
	// kuseg
        phys = virt + 0x40000000;
    }
    return (void*) phys;
}

/*
 * This macro is used to disable the USB module
 */
#define usb_module_disable() {\
	U1CON = 0;\
	U1IE = 0;\
	U1OTGIE = 0;\
	U1PWR |= PIC32_U1PWR_USBPWR;\
	usb_device_state = DETACHED_STATE;\
}
#endif
