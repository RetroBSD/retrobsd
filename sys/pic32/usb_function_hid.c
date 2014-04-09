/*
 * USB HID Function Driver File
 *
 * This file contains all of functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the HID function
 * driver. This file should be included in projects that use the HID
 * function driver.
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
#include <machine/usb_device.h>
#include <machine/usb_function_hid.h>

unsigned char hid_idle_rate;
unsigned char hid_active_protocol;  // [0] Boot Protocol [1] Report Protocol

/*
 * Section C: non-EP0 Buffer Space
 */
volatile unsigned char hid_report_out[HID_INT_OUT_EP_SIZE];
volatile unsigned char hid_report_in[HID_INT_IN_EP_SIZE];
volatile unsigned char hid_report_feature [HID_FEATURE_REPORT_BYTES];

/*
 * Check to see if the HID supports a specific Output or Feature report.
 *
 * Return: 1 if it's a supported Input report
 *         2 if it's a supported Output report
 *         3 if it's a supported Feature report
 *         0 for all other cases
 */
static unsigned char report_supported (void)
{
    // Find out if an Output or Feature report has arrived on the control pipe.
    usb_device_tasks();

    switch (usb_setup_pkt.W_Value >> 8) {
    case 0x01:                  // Input report
        switch (usb_setup_pkt.W_Value & 0xff) {
        case 0x00: return 1;    // Report ID 0
        default:   return 0;    // Other report IDs not supported.
        }
    case 0x02:          // Output report
        switch (usb_setup_pkt.W_Value & 0xff) {
        case 0x00: return 2;    // Report ID 0
        default:   return 0;    // Other report IDs not supported.
        }
    case 0x03:                  // Feature report
        switch (usb_setup_pkt.W_Value & 0xff) {
        case 0x00: return 3;    // Report ID 0
        default:   return 0;    // Other report IDs not supported.
        }
    default:
        return 0;
    }
}

/*
 * Check to see if an Output or Feature report has arrived
 * on the control pipe. If yes, extract and use the data.
 */
static void report_handler (void)
{
    unsigned char count = 0;

    // Find out if an Output or Feature report has arrived on the control pipe.
    // Get the report type from the Setup packet.

    switch (usb_setup_pkt.W_Value >> 8) {
    case 0x02:                  // Output report
        switch (usb_setup_pkt.W_Value & 0xff) {
        case 0:                 // Report ID 0
            // This example application copies the Output report data
            // to hid_report_in.
            // (Assumes Input and Output reports are the same length.)
            // A "real" application would do something more useful with the data.
            // wCount holds the number of bytes read in the Data stage.
            // This example assumes the report fits in one transaction.

            for (count=0; count <= HID_OUTPUT_REPORT_BYTES - 1; count++) {
                hid_report_in[count] = hid_report_out[count];
            }
            break;
        }
        break;

    case 0x03:                  // Feature report
        // Get the report ID from the Setup packet.
        switch (usb_setup_pkt.W_Value & 0xff) {
        case 0:                 // Report ID 0
            // The Feature report data is in hid_report_feature.
            // This example application just sends the data back in the next
            // Get_Report request for a Feature report.
            // wCount holds the number of bytes read in the Data stage.
            // This example assumes the report fits in one transaction.
            // The Feature report uses a single buffer so to send the same data back
            // in the next IN Feature report, there is nothing to copy.
            // The data is in hid_report_feature[HID_FEATURE_REPORT_BYTES]
            break;
        }
        break;
    }
}

/*
 * This routine handles HID specific request that happen on EP0.  These
 * include, but are not limited to, requests for the HID report
 * descriptors.  This function should be called from the
 * USBCBCheckOtherReq() call back function whenever using an HID device.
 *
 * Typical Usage:
 *      void USBCBCheckOtherReq(void)
 *      {
 *          // Since the stack didn't handle the request I need
 *          // to check my class drivers to see if it is for them
 *          hid_check_request();
 *      }
 */
void hid_check_request (void)
{
    if (usb_setup_pkt.Recipient != RCPT_INTF)
        return;
    if (usb_setup_pkt.bIntfID != HID_INTF_ID)
        return;

    /*
     * There are two standard requests that hid.c may support.
     * 1. GET_DSC(DSC_HID,DSC_RPT,DSC_PHY);
     * 2. SET_DSC(DSC_HID,DSC_RPT,DSC_PHY);
     */
    if (usb_setup_pkt.bRequest == GET_DSC) {
        switch (usb_setup_pkt.bDescriptorType) {
        case DSC_HID:
            if (usb_active_configuration == 1) {
                usb_ep0_send_rom_ptr ((const unsigned char*)
                    &usb_config1_descriptor + 18,
                    sizeof(USB_HID_DSC)+3,
                    USB_EP0_INCLUDE_ZERO);
            }
            break;
        case DSC_RPT:
            if (usb_active_configuration == 1) {
                usb_ep0_send_rom_ptr ((const unsigned char*)
                    &hid_rpt01[0],
                    HID_RPT01_SIZE,     // See target.cfg
                    USB_EP0_INCLUDE_ZERO);
            }
            break;
        case DSC_PHY:
            usb_ep0_transmit (USB_EP0_NO_DATA);
            break;
        }
    }

    if (usb_setup_pkt.RequestType != CLASS)
        return;

    switch (usb_setup_pkt.bRequest) {
    case GET_REPORT:
        switch (report_supported()) {
        case 1:                 // Input Report
            usb_in_pipe[0].pSrc.bRam = (unsigned char*) &hid_report_in[0];
            usb_in_pipe[0].info.bits.ctrl_trf_mem = _RAM;       // Set memory type
            usb_in_pipe[0].wCount = HID_INPUT_REPORT_BYTES;     // Set data count
            usb_in_pipe[0].info.bits.busy = 1;
            break;
        case 3:                 // Feature Report
            usb_in_pipe[0].pSrc.bRam = (unsigned char*) &hid_report_feature[0];
            usb_in_pipe[0].info.bits.ctrl_trf_mem = _RAM;       // Set memory type
            usb_in_pipe[0].wCount = HID_FEATURE_REPORT_BYTES;   // Set data count
            usb_in_pipe[0].info.bits.busy = 1;
            break;
        }
        break;
    case SET_REPORT:
        switch (report_supported()) {
        case 2:                 // Output Report
            usb_out_pipe[0].wCount = usb_setup_pkt.wLength;
            usb_out_pipe[0].pFunc = report_handler;
            usb_out_pipe[0].pDst.bRam = (unsigned char*) &hid_report_out[0];
            usb_out_pipe[0].info.bits.busy = 1;
            break;
        case 3:                 // Feature Report
            usb_out_pipe[0].wCount = usb_setup_pkt.wLength;
            usb_out_pipe[0].pFunc = report_handler;
            usb_out_pipe[0].pDst.bRam = (unsigned char*) &hid_report_feature[0];
            usb_out_pipe[0].info.bits.busy = 1;
            break;
        }
        break;
    case GET_IDLE:
        usb_ep0_send_ram_ptr (&hid_idle_rate, 1, USB_EP0_INCLUDE_ZERO);
        break;
    case SET_IDLE:
        usb_ep0_transmit (USB_EP0_NO_DATA);
        hid_idle_rate = usb_setup_pkt.W_Value >> 8;
        break;
    case GET_PROTOCOL:
        usb_ep0_send_ram_ptr (&hid_active_protocol, 1, USB_EP0_NO_OPTIONS);
        break;
    case SET_PROTOCOL:
        usb_ep0_transmit (USB_EP0_NO_DATA);
        hid_active_protocol = usb_setup_pkt.W_Value & 0xff;
        break;
    }
}
