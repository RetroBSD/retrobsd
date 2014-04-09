/*
 * Console driver via USB.
 *
 * Copyright (C) 2011 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <machine/pic32mx.h>
#include <machine/usb_device.h>
#include <machine/usb_function_cdc.h>

struct tty cnttys [1];

void cnstart (struct tty *tp);

/*
 * Initialize USB module SFRs and firmware variables to known state.
 * Enable interrupts.
 */
void cninit()
{
    usb_device_init();
    IECSET(1) = 1 << (PIC32_IRQ_USB - 32);

    /* Wait for any user input. */
    while (! cdc_consume(0))
        usb_device_tasks();
}

void cnidentify()
{
    printf ("console: USB\n");
}

int cnopen (dev, flag, mode)
    dev_t dev;
{
    register struct tty *tp = &cnttys[0];

    tp->t_oproc = cnstart;
    if ((tp->t_state & TS_ISOPEN) == 0) {
        ttychars(tp);
        tp->t_state = TS_ISOPEN | TS_CARR_ON;
        tp->t_flags = ECHO | XTABS | CRMOD | CRTBS | CRTERA | CTLECH | CRTKIL;
    }
    if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
        return (EBUSY);

    return ttyopen (dev, tp);
}

int cnclose (dev, flag, mode)
    dev_t dev;
{
    register struct tty *tp = &cnttys[0];

    ttyclose (tp);
    return 0;
}

int cnread (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    register struct tty *tp = &cnttys[0];

    return ttread (tp, uio, flag);
}

int cnwrite (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    register struct tty *tp = &cnttys[0];

    return ttwrite (tp, uio, flag);
}

int cnioctl (dev, cmd, addr, flag)
    dev_t dev;
    register u_int cmd;
    caddr_t addr;
{
    register struct tty *tp = &cnttys[0];
    register int error;

    error = ttioctl (tp, cmd, addr, flag);
    if (error < 0)
            error = ENOTTY;
    return error;
}

int cnselect (dev, rw)
    register dev_t dev;
    int rw;
{
    register struct tty *tp = &cnttys[0];

    return ttyselect (tp, rw);
}

void cnstart (tp)
    register struct tty *tp;
{
    register int s;

    s = spltty();
    if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
out:	/* Disable transmit_interrupt. */
        led_control (LED_TTY, 0);
        splx (s);
        return;
    }
    ttyowake(tp);
    if (tp->t_outq.c_cc == 0)
        goto out;
    if (cdc_is_tx_ready()) {
        while (tp->t_outq.c_cc != 0) {
            int c = getc (&tp->t_outq);
            if (cdc_putc (c) == 0)
                break;
        }
        cdc_tx_service();
        tp->t_state |= TS_BUSY;
    }
    led_control (LED_TTY, 1);
    splx (s);
}

/*
 * Put a symbol on console terminal.
 */
void cnputc (c)
    char c;
{
    register int s;

    s = spltty();
    while (! cdc_is_tx_ready()) {
        usb_device_tasks();
        cdc_tx_service();
    }
    led_control (LED_TTY, 1);
again:
    cdc_putc (c);
    cdc_tx_service();

    while (! cdc_is_tx_ready()) {
        cdc_tx_service();
        usb_device_tasks();
    }
    if (c == '\n') {
        c = '\r';
        goto again;
    }

    led_control (LED_TTY, 0);
    splx (s);
}

static int getc_data;

/*
 * Receive a character for getc.
 */
static void store_char (int c)
{
    getc_data = (unsigned char) c;
}

/*
 * Receive a symbol from console terminal.
 */
int
cngetc ()
{
    register int s;

    s = spltty();
    for (getc_data = -1; getc_data < 0; ) {
        usb_device_tasks();
        cdc_consume (store_char);
        cdc_tx_service();
    }
    splx (s);
    return getc_data;
}

/*
 * Receive a character from CDC.
 */
static void cn_rx (int c)
{
    register struct tty *tp = &cnttys[0];

    if ((tp->t_state & TS_ISOPEN) == 0)
        return;
    ttinput (c, tp);
}

/*
 * Check bus status and service USB interrupts.
 */
void cnintr (int chan)
{
    register struct tty *tp = &cnttys[0];

    // Must call this function from interrupt or periodically.
    usb_device_tasks();

    // Check that USB connection is established.
    if (usb_device_state < CONFIGURED_STATE ||
        (U1PWRC & PIC32_U1PWRC_USUSPEND))
        return;

    // Receive data from user.
    cdc_consume (cn_rx);

    if (cdc_is_tx_ready()) {
        // Transmitter empty.
        led_control (LED_TTY, 0);

        if (tp->t_state & TS_BUSY) {
            tp->t_state &= ~TS_BUSY;
            ttstart (tp);
        }
    }

    // Transmit data to user.
    cdc_tx_service();
}

/*
 * USB Callback Functions
 */

/*
 * This function is called when the device becomes initialized.
 * It should initialize the endpoints for the device's usage
 * according to the current configuration.
 */
void usbcb_init_ep()
{
    cdc_init_ep();
}

/*
 * Process device-specific SETUP requests.
 */
void usbcb_check_other_req()
{
    cdc_check_request();
}

#if 0
/*
 * Wake up a host PC.
 */
void usb_send_resume (void)
{
    /* Start RESUME signaling. */
    U1CON |= PIC32_U1CON_RESUME;

    /* Set RESUME line for 1-13 ms. */
    udelay (5000);

    U1CON &= ~PIC32_U1CON_RESUME;
}
#endif

#ifndef CONSOLE_VID
#   define CONSOLE_VID 0x04D8   // Vendor ID: Microchip
#endif
#ifndef CONSOLE_PID
#   define CONSOLE_PID 0x000A   // Product ID: CDC RS-232 Emulation Demo
#endif

/*
 * Device Descriptor
 */
const USB_DEVICE_DESCRIPTOR usb_device = {
    sizeof(usb_device),     // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,  // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    CDC_DEVICE,             // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    CONSOLE_VID,            // Vendor ID
    CONSOLE_PID,            // Product ID
    0x0100,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/*
 * Configuration 1 Descriptor
 */
const unsigned char usb_config1_descriptor[] =
{
    /* Configuration Descriptor */
    9,                                  // sizeof(USB_CFG_DSC)
    USB_DESCRIPTOR_CONFIGURATION,	// CONFIGURATION descriptor type
    67, 0,				// Total length of data for this cfg
    2,                                  // Number of interfaces in this cfg
    1,                                  // Index value of this configuration
    0,                                  // Configuration string index
    _DEFAULT | _SELF,                   // Attributes, see usb_device.h
    150,                                // Max power consumption (2X mA)

    /* Interface Descriptor */
    9,                                  // sizeof(USB_INTF_DSC)
    USB_DESCRIPTOR_INTERFACE,           // INTERFACE descriptor type
    0,                                  // Interface Number
    0,                                  // Alternate Setting Number
    1,                                  // Number of endpoints in this intf
    COMM_INTF,                          // Class code
    ABSTRACT_CONTROL_MODEL,		// Subclass code
    V25TER,				// Protocol code
    0,                                  // Interface string index

    /* CDC Class-Specific Descriptors */
    sizeof(USB_CDC_HEADER_FN_DSC),
    CS_INTERFACE,
    DSC_FN_HEADER,
    0x10,0x01,

    sizeof(USB_CDC_ACM_FN_DSC),
    CS_INTERFACE,
    DSC_FN_ACM,
    USB_CDC_ACM_FN_DSC_VAL,

    sizeof(USB_CDC_UNION_FN_DSC),
    CS_INTERFACE,
    DSC_FN_UNION,
    CDC_COMM_INTF_ID,
    CDC_DATA_INTF_ID,

    sizeof(USB_CDC_CALL_MGT_FN_DSC),
    CS_INTERFACE,
    DSC_FN_CALL_MGT,
    0x00,
    CDC_DATA_INTF_ID,

    /* Endpoint Descriptor */
    7,                                  // sizeof(USB_EP_DSC)
    USB_DESCRIPTOR_ENDPOINT,	        // Endpoint Descriptor
    _EP02_IN,			        // EndpointAddress
    _INTERRUPT,			        // Attributes
    0x08, 0x00,			        // size
    0x02,				// Interval

    /* Interface Descriptor */
    9,				        // sizeof(USB_INTF_DSC)
    USB_DESCRIPTOR_INTERFACE,	        // INTERFACE descriptor type
    1,				        // Interface Number
    0,				        // Alternate Setting Number
    2,				        // Number of endpoints in this intf
    DATA_INTF,			        // Class code
    0,				        // Subclass code
    NO_PROTOCOL,			// Protocol code
    0,				        // Interface string index

    /* Endpoint Descriptor */
    7,                                  // sizeof(USB_EP_DSC)
    USB_DESCRIPTOR_ENDPOINT,	        // Endpoint Descriptor
    _EP03_OUT,			        // EndpointAddress
    _BULK,				// Attributes
    0x40, 0x00,			        // size
    0x00,				// Interval

    /* Endpoint Descriptor */
    7,                                  // sizeof(USB_EP_DSC)
    USB_DESCRIPTOR_ENDPOINT,	        // Endpoint Descriptor
    _EP03_IN,			        // EndpointAddress
    _BULK,				// Attributes
    0x40, 0x00,			        // size
    0x00,				// Interval
};


/*
 * Language code string descriptor.
 */
static const USB_STRING_INIT(1) string0_descriptor = {
    sizeof(string0_descriptor),
    USB_DESCRIPTOR_STRING,
    { 0x0409 }                          /* US English */
};

/*
 * Manufacturer string descriptor
 */
static const USB_STRING_INIT(25) string1_descriptor = {
    sizeof(string1_descriptor),
    USB_DESCRIPTOR_STRING,
    { 'M','i','c','r','o','c','h','i','p',' ',
      'T','e','c','h','n','o','l','o','g','y',
      ' ','I','n','c','.', },
};

/*
 * Product string descriptor
 */
static const USB_STRING_INIT(16) string2_descriptor = {
    sizeof(string2_descriptor),
    USB_DESCRIPTOR_STRING,
    { 'R','e','t','r','o','B','S','D',' ','C',
      'o','n','s','o','l','e', },
};

/*
 * Array of configuration descriptors
 */
const unsigned char *const usb_config[] = {
    (const unsigned char *const) &usb_config1_descriptor,
};

/*
 * Array of string descriptors
 */
const unsigned char *const usb_string[USB_NUM_STRING_DESCRIPTORS] = {
    (const unsigned char *const) &string0_descriptor,
    (const unsigned char *const) &string1_descriptor,
    (const unsigned char *const) &string2_descriptor,
};
