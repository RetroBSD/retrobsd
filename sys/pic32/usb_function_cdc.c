/*
 * This file contains all of functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the CDC function
 * driver. This file should be included in projects that use the CDC
 * \function driver.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC® Microcontroller is intended and
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
#include <machine/pic32mx.h>
#include <machine/usb_device.h>
#include <machine/usb_function_cdc.h>

unsigned cdc_trf_state;         // States are defined cdc.h
unsigned cdc_tx_len;            // total tx length

LINE_CODING cdc_line_coding;	// Buffer to store line coding information

static USB_HANDLE data_out;
static USB_HANDLE data_in;

static CONTROL_SIGNAL_BITMAP control_signal_bitmap;

static volatile unsigned char cdc_data_rx [CDC_DATA_OUT_EP_SIZE];
static volatile unsigned char cdc_data_tx [CDC_DATA_IN_EP_SIZE];

/*
 * SEND_ENCAPSULATED_COMMAND and GET_ENCAPSULATED_RESPONSE are required
 * requests according to the CDC specification.
 * However, it is not really being used here, therefore a dummy buffer is
 * used for conformance.
 */
#define DUMMY_LENGTH    0x08

static unsigned char dummy_encapsulated_cmd_response [DUMMY_LENGTH];

/*
 * This routine checks the setup data packet to see if it
 * knows how to handle it.
 */
void cdc_check_request()
{
    /*
     * If request recipient is not an interface then return
     */
    if (usb_setup_pkt.Recipient != RCPT_INTF)
        return;

    /*
     * If request type is not class-specific then return
     */
    if (usb_setup_pkt.RequestType != CLASS)
        return;

    /*
     * Interface ID must match interface numbers associated with
     * CDC class, else return
     */
    if (usb_setup_pkt.bIntfID != CDC_COMM_INTF_ID &&
        usb_setup_pkt.bIntfID != CDC_DATA_INTF_ID)
        return;

    switch (usb_setup_pkt.bRequest)
    {
    case SEND_ENCAPSULATED_COMMAND:
        // send the packet
        usb_in_pipe[0].pSrc.bRam = (unsigned char*) &dummy_encapsulated_cmd_response;
        usb_in_pipe[0].wCount = DUMMY_LENGTH;
        usb_in_pipe[0].info.bits.ctrl_trf_mem = USB_INPIPES_RAM;
        usb_in_pipe[0].info.bits.busy = 1;
        break;

    case GET_ENCAPSULATED_RESPONSE:
        // Populate dummy_encapsulated_cmd_response first.
        usb_in_pipe[0].pSrc.bRam = (unsigned char*) &dummy_encapsulated_cmd_response;
        usb_in_pipe[0].info.bits.busy = 1;
        break;

    case SET_LINE_CODING:
        usb_out_pipe[0].wCount = usb_setup_pkt.wLength;
        usb_out_pipe[0].pDst.bRam = (unsigned char*) &cdc_line_coding._byte[0];
        usb_out_pipe[0].pFunc = 0;
        usb_out_pipe[0].info.bits.busy = 1;
        break;

    case GET_LINE_CODING:
        usb_ep0_send_ram_ptr ((unsigned char*) &cdc_line_coding,
            LINE_CODING_LENGTH, USB_EP0_INCLUDE_ZERO);
        break;

    case SET_CONTROL_LINE_STATE:
        control_signal_bitmap._byte = (unsigned char)usb_setup_pkt.W_Value;
        CONFIGURE_RTS(control_signal_bitmap.CARRIER_CONTROL);
        CONFIGURE_DTR(control_signal_bitmap.DTE_PRESENT);
        usb_in_pipe[0].info.bits.busy = 1;
        break;
    }
}

/*
 * This function initializes the CDC function driver. This function sets
 * the default line coding (baud rate, bit parity, number of data bits,
 * and format). This function also enables the endpoints and prepares for
 * the first transfer from the host.
 *
 * This function should be called after the SET_CONFIGURATION command.
 * This is most simply done by calling this function from the
 * usbcb_init_ep() function.
 *
 * Usage:
 *     void usbcb_init_ep()
 *     {
 *         cdc_init_ep();
 *     }
 */
void cdc_init_ep()
{
    // Abstract line coding information
    cdc_line_coding.dwDTERate = 115200;	// baud rate
    cdc_line_coding.bCharFormat = 0;	// 1 stop bit
    cdc_line_coding.bParityType = 0;	// None
    cdc_line_coding.bDataBits = 8;	// 5,6,7,8, or 16

    cdc_trf_state = CDC_TX_READY;
    cdc_tx_len = 0;

    /*
     * Do not have to init Cnt of IN pipes here.
     * Reason:  Number of BYTEs to send to the host
     *          varies from one transaction to
     *          another. Cnt should equal the exact
     *          number of BYTEs to transmit for
     *          a given IN transaction.
     *          This number of BYTEs will only
     *          be known right before the data is
     *          sent.
     */
    usb_enable_endpoint (CDC_COMM_EP, USB_IN_ENABLED |
        USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
    usb_enable_endpoint (CDC_DATA_EP, USB_IN_ENABLED | USB_OUT_ENABLED |
        USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);

    data_out = usb_rx_one_packet (CDC_DATA_EP,
        (unsigned char*) &cdc_data_rx, sizeof(cdc_data_rx));
    data_in = 0;
}

/*
 * Get received data.
 */
int cdc_consume (void (*func) (int))
{
    unsigned len;

    if (! data_out || usb_handle_busy (data_out))
        return 0;

    /*
     * Pass received data to user function.
     */
    len = usb_handle_get_length (data_out);
    if (func != 0) {
        unsigned n;
        for (n=0; n<len; n++)
            func (cdc_data_rx[n]);
    }

    /*
     * Prepare dual-ram buffer for next OUT transaction
     */
    data_out = usb_rx_one_packet (CDC_DATA_EP,
            (unsigned char*) &cdc_data_rx, sizeof(cdc_data_rx));
    return len;
}

/*
 * Send a symbol to the USB.
 * Return a number of free bytes in transmit buffer.
 *
 * Usage:
 *      if (cdc_is_tx_ready()) {
 *          do {
 *              space = cdc_putc (data[i++]);
 *          } while (space > 0);
 *      }
 * Conditions:
 *   cdc_is_tx_ready() must return TRUE. This indicates that the last
 *   transfer is complete and is ready to receive a new block of data.
 */
int cdc_putc (int c)
{
    if (cdc_trf_state != CDC_TX_READY || cdc_tx_len >= sizeof(cdc_data_tx))
        return 0;

    cdc_data_tx [cdc_tx_len++] = c;
    return sizeof(cdc_data_tx) - cdc_tx_len;
}

/*
 * cdc_tx_service handles device-to-host transaction(s). This function
 * should be called once per Main Program loop after the device reaches
 * the configured state.
 *
 * Usage:
 *   void main()
 *   {
 *       usb_device_init();
 *       while (1)
 *       {
 *           usb_device_tasks();
 *           if (USBGetDeviceState() < CONFIGURED_STATE || USBIsDeviceSuspended())
 *           {
 *               // Either the device is not configured or we are suspended
 *               // so we don't want to do execute any application code
 *               continue;   // go back to the top of the while loop
 *           } else {
 *               // Keep trying to send data to the PC as required
 *               cdc_tx_service();
 *
 *               // Run application code.
 *               UserApplication();
 *           }
 *       }
 *   }
 */
void cdc_tx_service()
{
    // Check that USB connection is established.
    if (usb_device_state < CONFIGURED_STATE ||
        (U1PWRC & PIC32_U1PWRC_USUSPEND))
        return;

    if (usb_handle_busy(data_in))
        return;
    /*
     * Completing stage is necessary while [ mCDCUSartTxIsBusy()==1 ].
     * By having this stage, user can always check cdc_trf_state,
     * and not having to call mCDCUsartTxIsBusy() directly.
     */
    if (cdc_trf_state == CDC_TX_COMPLETING) {
        cdc_trf_state = CDC_TX_READY;
        cdc_tx_len = 0;
    }

    /*
     * If CDC_TX_BUSY_ZLP state, send zero length packet
     */
    if (cdc_trf_state == CDC_TX_BUSY_ZLP)
    {
        data_in = usb_tx_one_packet (CDC_DATA_EP, 0, 0);
        cdc_trf_state = CDC_TX_COMPLETING;
        return;
    }

    /*
     * Send a next packet.
     */
    if (cdc_trf_state == CDC_TX_READY && cdc_tx_len > 0) {
        /*
         * Determine if a zero length packet state is necessary.
         * See explanation in USB Specification 2.0: Section 5.8.3
         */
        if (cdc_tx_len == CDC_DATA_IN_EP_SIZE)
            cdc_trf_state = CDC_TX_BUSY_ZLP;
        else
            cdc_trf_state = CDC_TX_COMPLETING;

        data_in = usb_tx_one_packet (CDC_DATA_EP, (unsigned char*)&cdc_data_tx, cdc_tx_len);
    }
}
