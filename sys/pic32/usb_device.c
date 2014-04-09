/*
 * This file contains functions, macros, definitions, variables,
 * datatypes, etc. that are required for usage with the MCHPFSUSB device
 * stack. This file should be included in projects that use the device stack.
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

#if (USB_PING_PONG_MODE != USB_PING_PONG__FULL_PING_PONG)
    #error "PIC32 only supports full ping pong mode."
#endif

unsigned usb_device_state;
unsigned usb_active_configuration;
static unsigned char usb_alternate_interface[USB_MAX_NUM_INT];
static volatile BDT_ENTRY *pBDTEntryEP0OutCurrent;
static volatile BDT_ENTRY *pBDTEntryEP0OutNext;
static volatile BDT_ENTRY *pBDTEntryOut[USB_MAX_EP_NUMBER+1];
static volatile BDT_ENTRY *pBDTEntryIn[USB_MAX_EP_NUMBER+1];
static unsigned short_packet_status;
static unsigned control_transfer_state;
static unsigned ustat_saved;
IN_PIPE usb_in_pipe[1];
OUT_PIPE usb_out_pipe[1];
int usb_remote_wakeup;

/*
 * Section A: Buffer Descriptor Table
 * - 0x400 - 0x4FF(max)
 * - USB_MAX_EP_NUMBER is defined in target.cfg
 */
volatile BDT_ENTRY usb_buffer [(USB_MAX_EP_NUMBER + 1) * 4] __attribute__ ((aligned (512)));

/*
 * Section B: EP0 Buffer Space
 */
volatile CTRL_TRF_SETUP usb_setup_pkt;           // 8-byte only

// Buffer for control transfer data
static volatile unsigned char ctrl_trf_data [USB_EP0_BUFF_SIZE];

/*
 * This function initializes the device stack
 * it in the default state
 *
 * The USB module will be completely reset including
 * all of the internal variables, registers, and
 * interrupt flags.
 */
void usb_device_init(void)
{
    unsigned i;

    // Clear all USB error flags
    U1EIR = 0xFF;

    // Clears all USB interrupts
    U1IR = 0xFF;

    U1EIE = 0x9F;                   // Unmask all USB error interrupts
    U1IE = PIC32_U1I_URST |         // Unmask Reset interrupt
           PIC32_U1I_IDLE |         // Unmask Idle interrupt
           PIC32_U1I_UERR |         // Unmask Error interrupt
           PIC32_U1I_TRN;           // Transaction Complete Interrupt

    // Power up the module
    U1PWRC |= PIC32_U1PWRC_USBPWR;

    // Set the address of the BDT (if applicable)
    U1BDTP1 = (unsigned) usb_buffer >> 8;

    // Reset all of the Ping Pong buffers
    U1CON |= PIC32_U1CON_PPBRST;
    U1CON &= ~PIC32_U1CON_PPBRST;

    // Reset to default address
    U1ADDR = 0x00;

    // Clear all of the endpoint control registers
    for (i=1; i<USB_MAX_EP_NUMBER; i++)
        U1EP(i) = 0;

    // Clear all of the BDT entries
    for (i=0; i<(sizeof(usb_buffer)/sizeof(BDT_ENTRY)); i++) {
       usb_buffer[i].Val = 0x00;
    }

    // Initialize EP0 as a Ctrl EP
    U1EP(0) = EP_CTRL | USB_HANDSHAKE_ENABLED;

    // Flush any pending transactions
    while (U1IR & PIC32_U1I_TRN) {
        U1IR = PIC32_U1I_TRN;
    }

    //clear all of the internal pipe information
    usb_in_pipe[0].info.Val = 0;
    usb_out_pipe[0].info.Val = 0;
    usb_out_pipe[0].wCount = 0;

    // Make sure packet processing is enabled
    U1CON &= ~PIC32_U1CON_PKTDIS;

    // Get ready for the first packet
    pBDTEntryIn[0] = (volatile BDT_ENTRY*) &usb_buffer[EP0_IN_EVEN];

    // Clear active configuration
    usb_active_configuration = 0;

    // Indicate that we are now in the detached state
    usb_device_state = DETACHED_STATE;
}

/*
 * This function is the main state machine of the
 * USB device side stack.  This function should be
 * called periodically to receive and transmit
 * packets through the stack.  This function should
 * be called  preferably once every 100us
 * during the enumeration process.  After the
 * enumeration process this function still needs to
 * be called periodically to respond to various
 * situations on the bus but is more relaxed in its
 * time requirements.  This function should also
 * be called at least as fast as the OUT data
 * expected from the PC.
 */
void usb_device_tasks(void)
{
    unsigned i;

#ifdef USB_SUPPORT_OTG
    // SRP Time Out Check
    if (USBOTGSRPIsReady())
    {
        if (USBT1MSECIF && USBT1MSECIE)
        {
            if (USBOTGGetSRPTimeOutFlag())
            {
                if (USBOTGIsSRPTimeOutExpired())
                {
                    USB_OTGEventHandler(0,OTG_EVENT_SRP_FAILED,0,0);
                }
            }
            // Clear Interrupt Flag
            *USBT1MSECIFReg = 1 << USBT1MSECIFBitNum;
        }
    }
    // If Session Is Started Then
    else {
        // If SRP Is Ready
        if (USBOTGSRPIsReady())
        {
            // Clear SRPReady
            USBOTGClearSRPReady();

            // Clear SRP Timeout Flag
            USBOTGClearSRPTimeOutFlag();

            // Indicate Session Started
            UART2PrintString( "\r\n***** USB OTG B Event - Session Started  *****\r\n" );
        }
    }
#endif

    // if we are in the detached state
    if (usb_device_state == DETACHED_STATE)
    {
	// Disable module & detach from bus
        U1CON = 0;

        // Mask all USB interrupts
        U1IE = 0;

        // Enable module & attach to bus
        while (! (U1CON & PIC32_U1CON_USBEN)) {
		U1CON |= PIC32_U1CON_USBEN;
	}

        // moved to the attached state
        usb_device_state = ATTACHED_STATE;

        // Enable/set things like: pull ups, full/low-speed mode,
        // set the ping pong mode, and set internal transceiver
        SetConfigurationOptions();

#ifdef USB_SUPPORT_OTG
	U1OTGCON = USB_OTG_DPLUS_ENABLE | USB_OTG_ENABLE;
#endif
    }

    if (usb_device_state == ATTACHED_STATE) {
        /*
         * After enabling the USB module, it takes some time for the
         * voltage on the D+ or D- line to rise high enough to get out
         * of the SE0 condition. The USB Reset interrupt should not be
         * unmasked until the SE0 condition is cleared. This helps
         * prevent the firmware from misinterpreting this unique event
         * as a USB bus reset from the USB host.
         */
        U1IR = 0;			// Clear all USB interrupts
        U1IE = 0;			// Mask all USB interrupts
	U1IE = PIC32_U1I_URST |         // Unmask RESET interrupt
               PIC32_U1I_IDLE;		// Unmask IDLE interrupt
	usb_device_state = POWERED_STATE;
    }

#ifdef USB_SUPPORT_OTG
    // If ID Pin Changed State
    if (USBIDIF && USBIDIE)
    {
        // Re-detect & Initialize
        USBOTGInitialize();

        *USBIDIFReg = 1 << USBIDIFBitNum;
    }
#endif

    /*
     * Task A: Service USB Activity Interrupt
     */
    if ((U1OTGIR & PIC32_U1OTGI_ACTV) && (U1OTGIE & PIC32_U1OTGI_ACTV))
    {
#if defined(USB_SUPPORT_OTG)
        U1OTGIR = PIC32_U1OTGI_ACTV;
#else
        usb_wake_from_suspend();
#endif
    }

    /*
     * Pointless to continue servicing if the device is in suspend mode.
     */
    if (U1PWRC & PIC32_U1PWRC_USUSPEND) {
        return;
    }

    /*
     * Task B: Service USB Bus Reset Interrupt.
     * When bus reset is received during suspend, ACTVIF will be set first,
     * once the UCON_SUSPND is clear, then the URSTIF bit will be asserted.
     * This is why URSTIF is checked after ACTVIF.
     *
     * The USB reset flag is masked when the USB state is in
     * DETACHED_STATE or ATTACHED_STATE, and therefore cannot
     * cause a USB reset event during these two states.
     */
    if ((U1IR & PIC32_U1I_URST) && (U1IE & PIC32_U1I_URST))
    {
        usb_device_init();
        usb_device_state = DEFAULT_STATE;

        /*
         * Bug Fix: Feb 26, 2007 v2.1 (#F1)
         *********************************************************************
         * In the original firmware, if an OUT token is sent by the host
         * before a SETUP token is sent, the firmware would respond with an ACK.
         * This is not a correct response, the firmware should have sent a STALL.
         * This is a minor non-compliance since a compliant host should not
         * send an OUT before sending a SETUP token. The fix allows a SETUP
         * transaction to be accepted while stalling OUT transactions.
         * */
        usb_buffer[EP0_OUT_EVEN].ADR = ConvertToPhysicalAddress (&usb_setup_pkt);
        usb_buffer[EP0_OUT_EVEN].CNT = USB_EP0_BUFF_SIZE;
        usb_buffer[EP0_OUT_EVEN].STAT.Val &= ~_STAT_MASK;
        usb_buffer[EP0_OUT_EVEN].STAT.Val |= _USIE|_DAT0|_DTSEN|_BSTALL;

#ifdef USB_SUPPORT_OTG
         // Disable HNP
         USBOTGDisableHnp();

         // Deactivate HNP
         USBOTGDeactivateHnp();
#endif
    }

    /*
     * Task C: Service other USB interrupts
     */
    if ((U1IR & PIC32_U1I_IDLE) && (U1IE & PIC32_U1I_IDLE))
    {
#ifdef USB_SUPPORT_OTG
        // If Suspended, Try to switch to Host
        USBOTGSelectRole(ROLE_HOST);
#else
        usb_suspend();
#endif
        U1IR = PIC32_U1I_IDLE;
    }

    if (U1IR & PIC32_U1I_SOF)
    {
        if (U1IE & PIC32_U1I_SOF)
            usbcb_sof_handler();    // Required callback, see usbcallbacks.c
        U1IR = PIC32_U1I_SOF;
    }

    if ((U1IR & PIC32_U1I_STALL) && (U1IE & PIC32_U1I_STALL))
    {
        usb_stall_handler();
    }

    if ((U1IR & PIC32_U1I_UERR) && (U1IE & PIC32_U1I_UERR))
    {
        usbcb_error_handler();  // Required callback, see usbcallbacks.c
        U1EIR = 0xFF;           // This clears UERRIF
    }

    /*
     * Pointless to continue servicing if the host has not sent a bus reset.
     * Once bus reset is received, the device transitions into the DEFAULT
     * state and is ready for communication.
     */
    if (usb_device_state < DEFAULT_STATE)
        return;

    /*
     * Task D: Servicing USB Transaction Complete Interrupt
     */
    if (U1IE & PIC32_U1I_TRN)
    {
        // Drain or deplete the USAT FIFO entries.
        // If the USB FIFO ever gets full, USB bandwidth
        // utilization can be compromised, and the device
        // won't be able to receive SETUP packets.
	for (i = 0; i < 4; i++) {
	    if (! (U1IR & PIC32_U1I_TRN))
	    	break;                      // USTAT FIFO must be empty.

            ustat_saved = U1STAT;
            U1IR = PIC32_U1I_TRN;

            /*
             * usb_ctrl_ep_service only services transactions over EP0.
             * It ignores all other EP transactions.
             */
            usb_ctrl_ep_service();
        }
    }
}

/*
 * This function handles the event of a STALL occuring on the bus
 */
void usb_stall_handler(void)
{
    /*
     * Does not really have to do anything here,
     * even for the control endpoint.
     * All BDs of Endpoint 0 are owned by SIE right now,
     * but once a Setup Transaction is received, the ownership
     * for EP0_OUT will be returned to CPU.
     * When the Setup Transaction is serviced, the ownership
     * for EP0_IN will then be forced back to CPU by firmware.
     */

    /* v2b fix */
    if (U1EP(0) & PIC32_U1EP_EPSTALL)
    {
        // UOWN - if 0, owned by CPU, if 1, owned by SIE
        if (pBDTEntryEP0OutCurrent->STAT.Val == _USIE &&
            pBDTEntryIn[0]->STAT.Val == (_USIE | _BSTALL))
        {
            // Set ep0Bo to stall also
            pBDTEntryEP0OutCurrent->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;
        }
	// Clear stall status
	U1EP(0) &= ~PIC32_U1EP_EPSTALL;
    }

    U1IR = PIC32_U1I_STALL;
}

/*
 * This function handles if the host tries to suspend the device
 */
void usb_suspend(void)
{
    /*
     * NOTE: Do not clear UIR_ACTVIF here!
     * Reason:
     * ACTVIF is only generated once an IDLEIF has been generated.
     * This is a 1:1 ratio interrupt generation.
     * For every IDLEIF, there will be only one ACTVIF regardless of
     * the number of subsequent bus transitions.
     *
     * If the ACTIF is cleared here, a problem could occur when:
     * [       IDLE       ][bus activity ->
     * <--- 3 ms ----->     ^
     *                ^     ACTVIF=1
     *                IDLEIF=1
     *  #           #           #           #   (#=Program polling flags)
     *                          ^
     *                          This polling loop will see both
     *                          IDLEIF=1 and ACTVIF=1.
     *                          However, the program services IDLEIF first
     *                          because ACTIVIE=0.
     *                          If this routine clears the only ACTIVIF,
     *                          then it can never get out of the suspend
     *                          mode.
     */

    U1OTGIE |= PIC32_U1OTGI_ACTV;	// Enable bus activity interrupt
    U1IR = PIC32_U1I_IDLE;

    /*
     * At this point the PIC can go into sleep,idle, or
     * switch to a slower clock, etc.  This should be done in the
     * usbcb_suspend() if necessary.
     */
    usbcb_suspend();             // Required callback, see usbcallbacks.c
}

/*
 * Wake up the USB module from suspend.
 */
void usb_wake_from_suspend(void)
{
    /*
     * If using clock switching, the place to restore the original
     * microcontroller core clock frequency is in the usbcb_wake_from_suspend() callback
     */
    usbcb_wake_from_suspend(); // Required callback, see usbcallbacks.c

    U1OTGIE &= ~PIC32_U1OTGI_ACTV;

    /*
    Bug Fix: Feb 26, 2007 v2.1
    *********************************************************************
    The ACTVIF bit cannot be cleared immediately after the USB module wakes
    up from Suspend or while the USB module is suspended. A few clock cycles
    are required to synchronize the internal hardware state machine before
    the ACTIVIF bit can be cleared by firmware. Clearing the ACTVIF bit
    before the internal hardware is synchronized may not have an effect on
    the value of ACTVIF. Additonally, if the USB module uses the clock from
    the 96 MHz PLL source, then after clearing the SUSPND bit, the USB
    module may not be immediately operational while waiting for the 96 MHz
    PLL to lock.
    */
    U1OTGIR = PIC32_U1OTGI_ACTV;
}

/*
 * usb_ctrl_ep_service checks for three transaction
 * types that it knows how to service and services them:
 * 1. EP0 SETUP
 * 2. EP0 OUT
 * 3. EP0 IN
 * It ignores all other types (i.e. EP1, EP2, etc.)
 *
 * PreCondition: USTAT is loaded with a valid endpoint address.
 */
void usb_ctrl_ep_service(void)
{
    // If the last packet was a EP0 OUT packet
    if ((ustat_saved & USTAT_EP0_PP_MASK) == USTAT_EP0_OUT_EVEN)
    {
	// Point to the EP0 OUT buffer of the buffer that arrived
        pBDTEntryEP0OutCurrent = (volatile BDT_ENTRY*)
            &usb_buffer [(ustat_saved & USTAT_EP_MASK) >> 2];

	// Set the next out to the current out packet
        pBDTEntryEP0OutNext = pBDTEntryEP0OutCurrent;

	// Toggle it to the next ping pong buffer (if applicable)
        *(unsigned char*)&pBDTEntryEP0OutNext ^= USB_NEXT_EP0_OUT_PING_PONG;

	// If the current EP0 OUT buffer has a SETUP token
        if (pBDTEntryEP0OutCurrent->STAT.PID == SETUP_TOKEN)
        {
            // Handle the control transfer
            usb_ctrl_trf_setup_handler();
        } else {
            // Handle the DATA transfer
            usb_ctrl_trf_out_handler();
        }
    } else if ((ustat_saved & USTAT_EP0_PP_MASK) == USTAT_EP0_IN)
    {
	// Otherwise the transmission was and EP0 IN
	// so take care of the IN transfer
        usb_ctrl_trf_in_handler();
    }
}

/*
 * This routine is a task dispatcher and has 3 stages.
 * 1. It initializes the control transfer state machine.
 * 2. It calls on each of the module that may know how to
 *    service the Setup Request from the host.
 *    Module Example: USBD, HID, CDC, MSD, ...
 *    A callback function, usbcb_check_other_req(),
 *    is required to call other module handlers.
 * 3. Once each of the modules has had a chance to check if
 *    it is responsible for servicing the request, stage 3
 *    then checks direction of the transfer to determine how
 *    to prepare EP0 for the control transfer.
 *    Refer to usb_ctrl_ep_service_complete() for more details.
 *
 * PreCondition: usb_setup_pkt buffer is loaded with valid USB Setup Data
 *
 * Microchip USB Firmware has three different states for
 * the control transfer state machine:
 * 1. WAIT_SETUP
 * 2. CTRL_TRF_TX
 * 3. CTRL_TRF_RX
 * Refer to firmware manual to find out how one state
 * is transitioned to another.
 *
 * A Control Transfer is composed of many USB transactions.
 * When transferring data over multiple transactions,
 * it is important to keep track of data source, data
 * destination, and data count. These three parameters are
 * stored in pSrc, pDst, and wCount. A flag is used to
 * note if the data source is from ROM or RAM.
 */
void usb_ctrl_trf_setup_handler(void)
{
    //if the SIE currently owns the buffer
    if (pBDTEntryIn[0]->STAT.UOWN != 0)
    {
	// give control back to the CPU
	// Compensate for after a STALL
        pBDTEntryIn[0]->STAT.Val = _UCPU;
    }

    // Keep track of if a short packet has been sent yet or not
    short_packet_status = SHORT_PKT_NOT_USED;

    /* Stage 1 */
    control_transfer_state = WAIT_SETUP;

    usb_in_pipe[0].wCount = 0;
    usb_in_pipe[0].info.Val = 0;

    /* Stage 2 */
    usb_check_std_request();
    usbcb_check_other_req();   // Required callback, see usbcallbacks.c

    /* Stage 3 */
    usb_ctrl_ep_service_complete();
}

/*
 * This routine handles an OUT transaction according to
 * which control transfer state is currently active.
 *
 * Note that if the the control transfer was from
 * host to device, the session owner should be notified
 * at the end of each OUT transaction to service the
 * received data.
 */
void usb_ctrl_trf_out_handler(void)
{
    if (control_transfer_state == CTRL_TRF_RX)
    {
        usb_ctrl_trf_rx_service();
    }
    else    // CTRL_TRF_TX
    {
        usb_prepare_for_next_setup_trf();
    }
}

/*
 * This routine handles an IN transaction according to
 * which control transfer state is currently active.
 *
 * A Set Address Request must not change the acutal address
 * of the device until the completion of the control
 * transfer. The end of the control transfer for Set Address
 * Request is an IN transaction. Therefore it is necessary
 * to service this unique situation when the condition is
 * right. Macro mUSBCheckAdrPendingState is defined in
 * usb9.h and its function is to specifically service this event.
 */
void usb_ctrl_trf_in_handler(void)
{
    unsigned lastDTS;

    lastDTS = pBDTEntryIn[0]->STAT.DTS;

    //switch to the next ping pong buffer
    *(unsigned char*)&pBDTEntryIn[0] ^= USB_NEXT_EP0_IN_PING_PONG;

    //mUSBCheckAdrPendingState();       // Must check if in ADR_PENDING_STATE
    if (usb_device_state == ADR_PENDING_STATE)
    {
        U1ADDR = usb_setup_pkt.bDevADR;
        if (U1ADDR > 0)
        {
            usb_device_state = ADDRESS_STATE;
        }
        else
        {
            usb_device_state = DEFAULT_STATE;
        }
    }//end if


    if (control_transfer_state == CTRL_TRF_TX)
    {
        pBDTEntryIn[0]->ADR = ConvertToPhysicalAddress (ctrl_trf_data);
        usb_ctrl_trf_tx_service();

        /* v2b fix */
        if (short_packet_status == SHORT_PKT_SENT)
        {
            // If a short packet has been sent, don't want to send any more,
            // stall next time if host is still trying to read.
            pBDTEntryIn[0]->STAT.Val = _USIE|_BSTALL;
        } else {
            if (lastDTS == 0) {
                pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;
            } else {
                pBDTEntryIn[0]->STAT.Val = _USIE|_DAT0|_DTSEN;
            }
        }
    } else {
        // CTRL_TRF_RX
        usb_prepare_for_next_setup_trf();
    }
}

/*
 * The routine forces EP0 OUT to be ready for a new
 * Setup transaction, and forces EP0 IN to be owned by CPU.
 */
void usb_prepare_for_next_setup_trf(void)
{
    /*
    Bug Fix: Feb 26, 2007 v2.1
    *********************************************************************
    Facts:
    A Setup Packet should never be stalled. (USB 2.0 Section 8.5.3)
    If a Setup PID is detected by the SIE, the DTSEN setting is ignored.
    This causes a problem at the end of a control write transaction.
    In usb_ctrl_ep_service_complete(), during a control write (Host to Device),
    the EP0_OUT is setup to write any data to the ctrl_trf_data buffer.
    If <SETUP[0]><IN[1]> is completed and usb_ctrl_trf_in_handler() is not
    called before the next <SETUP[0]> is received, then the latest Setup
    data will be written to the ctrl_trf_data buffer instead of the usb_setup_pkt
    buffer.

    If usb_ctrl_trf_in_handler() was called before the latest <SETUP[0]> is
    received, then there would be no problem,
    because usb_prepare_for_next_setup_trf() would have been called and updated
    ep0Bo.ADR to point to the usb_setup_pkt buffer.

    Work around:
    Check for the problem as described above and copy the Setup data from
    ctrl_trf_data to usb_setup_pkt.
    */
    if ((control_transfer_state == CTRL_TRF_RX) &&
       (U1CON & PIC32_U1CON_PKTDIS) &&
       (pBDTEntryEP0OutCurrent->CNT == sizeof(CTRL_TRF_SETUP)) &&
       (pBDTEntryEP0OutCurrent->STAT.PID == SETUP_TOKEN) &&
       (pBDTEntryEP0OutNext->STAT.UOWN == 0))
    {
        unsigned setup_cnt;

        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&usb_setup_pkt);

        // The Setup data was written to the ctrl_trf_data buffer, must copy
        // it back to the usb_setup_pkt buffer so that it can be processed correctly
        // by usb_ctrl_trf_setup_handler().
        for(setup_cnt = 0; setup_cnt < sizeof(CTRL_TRF_SETUP); setup_cnt++)
        {
            *(((unsigned char*) &usb_setup_pkt) + setup_cnt) =
                *(((unsigned char*) &ctrl_trf_data) + setup_cnt);
        }
    /* End v3b fix */
    } else {
        control_transfer_state = WAIT_SETUP;
        pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;      // Defined in target.cfg
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&usb_setup_pkt);

        /*
        Bug Fix: Feb 26, 2007 v2.1 (#F1)
        *********************************************************************
        In the original firmware, if an OUT token is sent by the host
        before a SETUP token is sent, the firmware would respond with an ACK.
        This is not a correct response, the firmware should have sent a STALL.
        This is a minor non-compliance since a compliant host should not
        send an OUT before sending a SETUP token. The fix allows a SETUP
        transaction to be accepted while stalling OUT transactions.
        */
        //ep0Bo.Stat.Val = _USIE|_DAT0|_DTSEN;        // Removed
        pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;  //Added #F1

        /*
        Bug Fix: Feb 26, 2007 v2.1 (#F3)
        *********************************************************************
        In the original firmware, if an IN token is sent by the host
        before a SETUP token is sent, the firmware would respond with an ACK.
        This is not a correct response, the firmware should have sent a STALL.
        This is a minor non-compliance since a compliant host should not
        send an IN before sending a SETUP token.

        Comment why this fix (#F3) is interfering with fix (#AF1).
        */
        pBDTEntryIn[0]->STAT.Val = _UCPU;             // Should be removed

        {
            BDT_ENTRY* p;

            p = (BDT_ENTRY*)(((unsigned int)pBDTEntryIn[0])^USB_NEXT_EP0_IN_PING_PONG);
            p->STAT.Val = _UCPU;
        }

        //ep0Bi.Stat.Val = _USIE|_BSTALL;   // Should be added #F3
    }

    //if someone is still expecting data from the control transfer
    //  then make sure to terminate that request and let them know that
    //  they are done
    if (usb_out_pipe[0].info.bits.busy == 1) {
        if (usb_out_pipe[0].pFunc != 0) {
            usb_out_pipe[0].pFunc();
        }
        usb_out_pipe[0].info.bits.busy = 0;
    }

}//end usb_prepare_for_next_setup_trf

/*
 * This routine checks the setup data packet to see
 * if it knows how to handle it
 */
void usb_check_std_request(void)
{
    if (usb_setup_pkt.RequestType != STANDARD) return;

    switch (usb_setup_pkt.bRequest) {
    case SET_ADR:
        usb_in_pipe[0].info.bits.busy = 1;            // This will generate a zero length packet
        usb_device_state = ADR_PENDING_STATE;     // Update state only
        /* See usb_ctrl_trf_in_handler() for the next step */
        break;
    case GET_DSC:
        usb_std_get_dsc_handler();
        break;
    case SET_CFG:
        usb_std_set_cfg_handler();
        break;
    case GET_CFG:
        usb_in_pipe[0].pSrc.bRam = (unsigned char*)&usb_active_configuration;	// Set Source
        usb_in_pipe[0].info.bits.ctrl_trf_mem = _RAM;			// Set memory type
        usb_in_pipe[0].wCount |= 0xff;					// Set data count
        usb_in_pipe[0].info.bits.busy = 1;
        break;
    case GET_STATUS:
        usb_std_get_status_handler();
        break;
    case CLR_FEATURE:
    case SET_FEATURE:
        usb_std_feature_req_handler();
        break;
    case GET_INTF:
        usb_in_pipe[0].pSrc.bRam = (unsigned char*)&usb_alternate_interface +
            usb_setup_pkt.bIntfID;				// Set source
        usb_in_pipe[0].info.bits.ctrl_trf_mem = _RAM;		// Set memory type
        usb_in_pipe[0].wCount |= 0xff;				// Set data count
        usb_in_pipe[0].info.bits.busy = 1;
        break;
    case SET_INTF:
        usb_in_pipe[0].info.bits.busy = 1;
        usb_alternate_interface[usb_setup_pkt.bIntfID] = usb_setup_pkt.bAltID;
        break;
    case SET_DSC:
        usbcb_std_set_dsc_handler();
        break;
    case SYNCH_FRAME:
    default:
        break;
    }
}

/*
 * This routine handles the standard SET & CLEAR
 * FEATURES requests
 */
void usb_std_feature_req_handler(void)
{
    BDT_ENTRY *p;
    unsigned int *pUEP;

#ifdef	USB_SUPPORT_OTG
    if ((usb_setup_pkt.bFeature == OTG_FEATURE_B_HNP_ENABLE)&&
        (usb_setup_pkt.Recipient == RCPT_DEV))
    {
        usb_in_pipe[0].info.bits.busy = 1;
        if (usb_setup_pkt.bRequest == SET_FEATURE)
            USBOTGEnableHnp();
        else
            USBOTGDisableHnp();
    }

    if ((usb_setup_pkt.bFeature == OTG_FEATURE_A_HNP_SUPPORT)&&
        (usb_setup_pkt.Recipient == RCPT_DEV))
    {
        usb_in_pipe[0].info.bits.busy = 1;
        if (usb_setup_pkt.bRequest == SET_FEATURE)
            USBOTGEnableSupportHnp();
        else
            USBOTGDisableSupportHnp();
    }


    if ((usb_setup_pkt.bFeature == OTG_FEATURE_A_ALT_HNP_SUPPORT)&&
        (usb_setup_pkt.Recipient == RCPT_DEV))
    {
        usb_in_pipe[0].info.bits.busy = 1;
        if (usb_setup_pkt.bRequest == SET_FEATURE)
            USBOTGEnableAltHnp();
        else
            USBOTGDisableAltHnp();
    }
#endif
    if ((usb_setup_pkt.bFeature == DEVICE_REMOTE_WAKEUP)&&
       (usb_setup_pkt.Recipient == RCPT_DEV))
    {
        usb_in_pipe[0].info.bits.busy = 1;
        if (usb_setup_pkt.bRequest == SET_FEATURE)
            usb_remote_wakeup = 1;
        else
            usb_remote_wakeup = 0;
    }

    if ((usb_setup_pkt.bFeature == ENDPOINT_HALT)&&
       (usb_setup_pkt.Recipient == RCPT_EP)&&
       (usb_setup_pkt.EPNum != 0))
    {
        usb_in_pipe[0].info.bits.busy = 1;
        /* Must do address calculation here */

        if (usb_setup_pkt.EPDir == 0)
        {
            p = (BDT_ENTRY*)pBDTEntryOut[usb_setup_pkt.EPNum];
        } else {
            p = (BDT_ENTRY*)pBDTEntryIn[usb_setup_pkt.EPNum];
        }

		//if it was a SET_FEATURE request
        if (usb_setup_pkt.bRequest == SET_FEATURE)
        {
            // Then STALL the endpoint
            p->STAT.Val = _USIE|_BSTALL;
        } else {
            // If it was not a SET_FEATURE
            // point to the appropriate UEP register
            pUEP = (unsigned int*) &U1EP(0);
            pUEP += usb_setup_pkt.EPNum * 4;

	    //Clear the STALL bit in the UEP register
            *pUEP &= ~UEP_STALL;

            if (usb_setup_pkt.EPDir == 1) // IN
            {
		// If the endpoint is an IN endpoint then we
		// need to return it to the CPU and reset the
		// DTS bit so that the next transfer is correct
#if (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0) || \
    (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
                p->STAT.Val = _UCPU | _DAT0;
                // toggle over the to the next buffer
                *(unsigned char*)&p ^= USB_NEXT_PING_PONG;
                p->STAT.Val = _UCPU | _DAT1;
#else
                p->STAT.Val = _UCPU | _DAT1;
#endif
            } else {
		// If the endpoint was an OUT endpoint then we
		// need to give control of the endpoint back to
		// the SIE so that the function driver can
		// receive the data as they expected.  Also need
		// to set the DTS bit so the next packet will be
		// correct
#if (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0) || \
    (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
                p->STAT.Val = _USIE|_DAT0|_DTSEN;
                //toggle over the to the next buffer
                *(unsigned char*)&p ^= USB_NEXT_PING_PONG;
                p->STAT.Val = _USIE|_DAT1|_DTSEN;
#else
                p->STAT.Val = _USIE|_DAT1|_DTSEN;
#endif
            }
        }
    }
}

/*
 * This routine handles the standard GET_DESCRIPTOR request.
 */
void usb_std_get_dsc_handler(void)
{
    if (usb_setup_pkt.bmRequestType == 0x80)
    {
        usb_in_pipe[0].info.Val = USB_INPIPES_ROM | USB_INPIPES_BUSY | USB_INPIPES_INCLUDE_ZERO;

        switch(usb_setup_pkt.bDescriptorType)
        {
            case USB_DESCRIPTOR_DEVICE:
                usb_in_pipe[0].pSrc.bRom = (const unsigned char*) &usb_device;
                usb_in_pipe[0].wCount = sizeof(usb_device);
                break;
            case USB_DESCRIPTOR_CONFIGURATION:
                usb_in_pipe[0].pSrc.bRom = usb_config [usb_setup_pkt.bDscIndex];
                usb_in_pipe[0].wCount = *(usb_in_pipe[0].pSrc.wRom+1);                // Set data count
                break;
            case USB_DESCRIPTOR_STRING:
#if defined(USB_NUM_STRING_DESCRIPTORS)
		if (usb_setup_pkt.bDscIndex < USB_NUM_STRING_DESCRIPTORS)
#else
                if (1)
#endif
                {
                    //Get a pointer to the String descriptor requested
                    usb_in_pipe[0].pSrc.bRom = usb_string [usb_setup_pkt.bDscIndex];
                    // Set data count
                    usb_in_pipe[0].wCount = *usb_in_pipe[0].pSrc.bRom;
                }
                else
                {
                    usb_in_pipe[0].info.Val = 0;
                }
                break;
            default:
                usb_in_pipe[0].info.Val = 0;
                break;
        }//end switch
    }//end if
}//end usb_std_get_dsc_handler

/*
 * This routine handles the standard GET_STATUS request
 */
void usb_std_get_status_handler(void)
{
    ctrl_trf_data[0] = 0;                 // Initialize content
    ctrl_trf_data[1] = 0;

    switch(usb_setup_pkt.Recipient)
    {
        case RCPT_DEV:
            usb_in_pipe[0].info.bits.busy = 1;
            /*
             * [0]: bit0: Self-Powered Status [0] Bus-Powered [1] Self-Powered
             *      bit1: RemoteWakeup        [0] Disabled    [1] Enabled
             */
	    ctrl_trf_data[0] |= 1;		// self powered

            if (usb_remote_wakeup == 1) {
                ctrl_trf_data[0] |= 2;
            }
            break;
        case RCPT_INTF:
            usb_in_pipe[0].info.bits.busy = 1;  // No data to update
            break;
        case RCPT_EP:
            usb_in_pipe[0].info.bits.busy = 1;
            /*
             * [0]: bit0: Halt Status [0] Not Halted [1] Halted
             */
            {
                BDT_ENTRY *p;

                if (usb_setup_pkt.EPDir == 0)
                {
                    p = (BDT_ENTRY*)pBDTEntryOut[usb_setup_pkt.EPNum];
                } else {
                    p = (BDT_ENTRY*)pBDTEntryIn[usb_setup_pkt.EPNum];
                }

                if (p->STAT.Val & _BSTALL)      // Use _BSTALL as a bit mask
                    ctrl_trf_data[0] = 1;       // Set bit0
                break;
            }
    }//end switch

    if (usb_in_pipe[0].info.bits.busy == 1)
    {
        usb_in_pipe[0].pSrc.bRam = (unsigned char*) &ctrl_trf_data; // Set Source
        usb_in_pipe[0].info.bits.ctrl_trf_mem = _RAM;               // Set memory type
        usb_in_pipe[0].wCount &= ~0xff;
        usb_in_pipe[0].wCount |= 2;                                 // Set data count
    }
}

/*
 * This routine wrap up the ramaining tasks in servicing
 * a Setup Request. Its main task is to set the endpoint
 * controls appropriately for a given situation. See code
 * below.
 * There are three main scenarios:
 * a) There was no handler for the Request, in this case
 *    a STALL should be sent out.
 * b) The host has requested a read control transfer,
 *    endpoints are required to be setup in a specific way.
 * c) The host has requested a write control transfer, or
 *    a control data stage is not required, endpoints are
 *    required to be setup in a specific way.
 *
 * Packet processing is resumed by clearing PKTDIS bit.
 */
void usb_ctrl_ep_service_complete(void)
{
    /*
     * PKTDIS bit is set when a Setup Transaction is received.
     * Clear to resume packet processing.
     */
    U1CON &= ~PIC32_U1CON_PKTDIS;

    if (usb_in_pipe[0].info.bits.busy == 0)
    {
        if (usb_out_pipe[0].info.bits.busy == 1)
        {
            control_transfer_state = CTRL_TRF_RX;
            /*
             * Control Write:
             * <SETUP[0]><OUT[1]><OUT[0]>...<IN[1]> | <SETUP[0]>
             *
             * 1. Prepare IN EP to respond to early termination
             *
             *    This is the same as a Zero Length Packet Response
             *    for control transfer without a data stage
             */
            pBDTEntryIn[0]->CNT = 0;
            pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;

            /*
             * 2. Prepare OUT EP to receive data.
             */
            pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
            pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress (&ctrl_trf_data);
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT1|_DTSEN;
        } else {
            /*
             * If no one knows how to service this request then stall.
             * Must also prepare EP0 to receive the next SETUP transaction.
             */
            pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
            pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&usb_setup_pkt);

            /* v2b fix */
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;
            pBDTEntryIn[0]->STAT.Val = _USIE|_BSTALL;
        }
    } else {
        // A module has claimed ownership of the control transfer session.
        if (usb_out_pipe[0].info.bits.busy == 0)
        {
            if (usb_setup_pkt.DataDir == DEV_TO_HOST)
            {
                if (usb_setup_pkt.wLength < usb_in_pipe[0].wCount)
                {
                        usb_in_pipe[0].wCount = usb_setup_pkt.wLength;
                }
                usb_ctrl_trf_tx_service();
                control_transfer_state = CTRL_TRF_TX;
                /*
                 * Control Read:
                 * <SETUP[0]><IN[1]><IN[0]>...<OUT[1]> | <SETUP[0]>
                 * 1. Prepare OUT EP to respond to early termination
                 *
                 * NOTE:
                 * If something went wrong during the control transfer,
                 * the last status stage may not be sent by the host.
                 * When this happens, two different things could happen
                 * depending on the host.
                 * a) The host could send out a RESET.
                 * b) The host could send out a new SETUP transaction
                 *    without sending a RESET first.
                 * To properly handle case (b), the OUT EP must be setup
                 * to receive either a zero length OUT transaction, or a
                 * new SETUP transaction.
                 *
                 * Furthermore, the Cnt byte should be set to prepare for
                 * the SETUP data (8-byte or more), and the buffer address
                 * should be pointed to usb_setup_pkt.
                 */
                pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
                pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&usb_setup_pkt);
                pBDTEntryEP0OutNext->STAT.Val = _USIE;           // Note: DTSEN is 0!

                pBDTEntryEP0OutCurrent->CNT = USB_EP0_BUFF_SIZE;
                pBDTEntryEP0OutCurrent->ADR = (unsigned char*)&usb_setup_pkt;
                pBDTEntryEP0OutCurrent->STAT.Val = _USIE;           // Note: DTSEN is 0!

                /*
                 * 2. Prepare IN EP to transfer data, Cnt should have
                 *    been initialized by responsible request owner.
                 */
                pBDTEntryIn[0]->ADR = ConvertToPhysicalAddress (&ctrl_trf_data);
                pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;

            } else {  // (usb_setup_pkt.DataDir == HOST_TO_DEVICE)

                control_transfer_state = CTRL_TRF_RX;
                /*
                 * Control Write:
                 * <SETUP[0]><OUT[1]><OUT[0]>...<IN[1]> | <SETUP[0]>
                 *
                 * 1. Prepare IN EP to respond to early termination
                 *
                 *    This is the same as a Zero Length Packet Response
                 *    for control transfer without a data stage
                 */
                pBDTEntryIn[0]->CNT = 0;
                pBDTEntryIn[0]->STAT.Val = _USIE|_DAT1|_DTSEN;

                /*
                 * 2. Prepare OUT EP to receive data.
                 */
                pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
                pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress (&ctrl_trf_data);
                pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT1|_DTSEN;
            }
        }
    }
}


/*
 * This routine should be called from only two places.
 * One from usb_ctrl_ep_service_complete() and one from
 * usb_ctrl_trf_in_handler(). It takes care of managing a
 * transfer over multiple USB transactions.
 *
 * This routine works with isochronous endpoint larger than
 * 256 bytes and is shown here as an example of how to deal
 * with BC9 and BC8. In reality, a control endpoint can never
 * be larger than 64 bytes.
 *
 * PreCondition: pSrc, wCount, and usb_stat.ctrl_trf_mem are setup properly.
 */
void usb_ctrl_trf_tx_service(void)
{
    unsigned byteToSend;
    unsigned char *dst;

    /*
     * First, have to figure out how many byte of data to send.
     */
    if (usb_in_pipe[0].wCount < USB_EP0_BUFF_SIZE)
    {
        byteToSend = usb_in_pipe[0].wCount;

        /* v2b fix */
        if (short_packet_status == SHORT_PKT_NOT_USED) {
            short_packet_status = SHORT_PKT_PENDING;

        } else if (short_packet_status == SHORT_PKT_PENDING) {
            short_packet_status = SHORT_PKT_SENT;
        }
        /* end v2b fix for this section */
    }
    else
    {
        byteToSend = USB_EP0_BUFF_SIZE;
    }

    /*
     * Next, load the number of bytes to send to BC9..0 in buffer descriptor
     */
    pBDTEntryIn[0]->CNT = byteToSend;

    /*
     * Subtract the number of bytes just about to be sent from the total.
     */
    usb_in_pipe[0].wCount = usb_in_pipe[0].wCount - byteToSend;

    // Set destination pointer
    dst = (unsigned char*) ctrl_trf_data;

    // Determine type of memory source
    if (usb_in_pipe[0].info.bits.ctrl_trf_mem == USB_INPIPES_ROM)
    {
        while (byteToSend)
        {
            *dst++ = *usb_in_pipe[0].pSrc.bRom++;
            byteToSend--;
        }
    } else { // RAM
        while (byteToSend)
        {
            *dst++ = *usb_in_pipe[0].pSrc.bRam++;
            byteToSend--;
        }
    }
}

/*
 * *** This routine is only partially complete. Check for
 * new version of the firmware.
 *
 * PreCondition: pDst and wCount are setup properly.
 *               pSrc is always &ctrl_trf_data
 *               usb_stat.ctrl_trf_mem is always _RAM.
 *               wCount should be set to 0 at the start of each control transfer.
 */
void usb_ctrl_trf_rx_service(void)
{
    unsigned byteToRead, i;

    byteToRead = pBDTEntryEP0OutCurrent->CNT;

    /*
     * Accumulate total number of bytes read
     */
    if (byteToRead > usb_out_pipe[0].wCount)
    {
        byteToRead = usb_out_pipe[0].wCount;
    } else {
        usb_out_pipe[0].wCount = usb_out_pipe[0].wCount - byteToRead;
    }

    for(i=0;i<byteToRead;i++)
    {
        *usb_out_pipe[0].pDst.bRam++ = ctrl_trf_data[i];
    }//end while(byteToRead)

    //If there is more data to read
    if (usb_out_pipe[0].wCount > 0)
    {
        /*
         * Don't have to worry about overwriting _KEEP bit
         * because if _KEEP was set, TRNIF would not have been
         * generated in the first place.
         */
        pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress (&ctrl_trf_data);
        if (pBDTEntryEP0OutCurrent->STAT.DTS == 0)
        {
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT1|_DTSEN;
        } else {
            pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN;
        }
    } else {
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&usb_setup_pkt);
        if (usb_out_pipe[0].pFunc != 0) {
            usb_out_pipe[0].pFunc();
        }
        usb_out_pipe[0].info.bits.busy = 0;
    }

    // reset ep0Bo.Cnt to USB_EP0_BUFF_SIZE

}//end usb_ctrl_trf_rx_service

/*
 * This routine first disables all endpoints by
 * clearing UEP registers. It then configures
 * (initializes) endpoints by calling the callback
 * function usbcb_init_ep().
 */
void usb_std_set_cfg_handler(void)
{
    unsigned i;

    // This will generate a zero length packet
    usb_in_pipe[0].info.bits.busy = 1;

    // disable all endpoints except endpoint 0
    for (i=1; i<USB_MAX_EP_NUMBER; i++)
        U1EP(i) = 0;

    // clear the alternate interface settings
    for (i=0; i<USB_MAX_NUM_INT; i++)
        usb_alternate_interface[i] = 0;

    // set the current configuration
    usb_active_configuration = usb_setup_pkt.bConfigurationValue;

    // if the configuration value == 0
    if (usb_setup_pkt.bConfigurationValue == 0)
    {
        // Go back to the addressed state
        usb_device_state = ADDRESS_STATE;
    } else {
        // Otherwise go to the configured state
        usb_device_state = CONFIGURED_STATE;

        // initialize the required endpoints
        usb_init_ep ((const unsigned char*) usb_config [usb_active_configuration - 1]);
        usbcb_init_ep();
    }
}

/*
 * This function will configure the specified endpoint.
 *
 * Input: unsigned EPNum - the endpoint to be configured
 *        unsigned direction - the direction to be configured
 */
void usb_configure_endpoint (unsigned epnum, unsigned direction)
{
    volatile BDT_ENTRY* handle;

    handle = (volatile BDT_ENTRY*) &usb_buffer[EP0_OUT_EVEN];
    handle += BD(epnum, direction, 0) / sizeof(BDT_ENTRY);

    handle->STAT.UOWN = 0;

    if (direction == 0) {
        pBDTEntryOut[epnum] = handle;
    } else {
        pBDTEntryIn[epnum] = handle;
    }

#if (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
    handle->STAT.DTS = 0;
    (handle+1)->STAT.DTS = 1;
#elif (USB_PING_PONG_MODE == USB_PING_PONG__NO_PING_PONG)
    //Set DTS to one because the first thing we will do
    //when transmitting is toggle the bit
    handle->STAT.DTS = 1;
#elif (USB_PING_PONG_MODE == USB_PING_PONG__EP0_OUT_ONLY)
    if (epnum != 0)
    {
        handle->STAT.DTS = 1;
    }
#elif (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0)
    if (epnum != 0)
    {
        handle->STAT.DTS = 0;
        (handle+1)->STAT.DTS = 1;
    }
#endif
}

/*
 * This function will enable the specified endpoint with the specified
 * options.
 *
 * Typical Usage:
 * <code>
 * void usbcb_init_ep(void)
 * {
 *     usb_enable_endpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
 *     USBMSDInit();
 * }
 * </code>
 *
 * In the above example endpoint number MSD_DATA_IN_EP is being configured
 * for both IN and OUT traffic with handshaking enabled. Also since
 * MSD_DATA_IN_EP is not endpoint 0 (MSD does not allow this), then we can
 * explicitly disable SETUP packets on this endpoint.
 *
 * Input:
 *   unsigned ep -       the endpoint to be configured
 *   unsigned options -  optional settings for the endpoint. The options should
 *                   be ORed together to form a single options string. The
 *                   available optional settings for the endpoint. The
 *                   options should be ORed together to form a single options
 *                   string. The available options are the following\:
 *                   * USB_HANDSHAKE_ENABLED enables USB handshaking (ACK,
 *                     NAK)
 *                   * USB_HANDSHAKE_DISABLED disables USB handshaking (ACK,
 *                     NAK)
 *                   * USB_OUT_ENABLED enables the out direction
 *                   * USB_OUT_DISABLED disables the out direction
 *                   * USB_IN_ENABLED enables the in direction
 *                   * USB_IN_DISABLED disables the in direction
 *                   * USB_ALLOW_SETUP enables control transfers
 *                   * USB_DISALLOW_SETUP disables control transfers
 *                   * USB_STALL_ENDPOINT STALLs this endpoint
 */
void usb_enable_endpoint (unsigned ep, unsigned options)
{
    // Set the options to the appropriate endpoint control register
    unsigned int *p = (unsigned int*) (&U1EP(0) + (4 * ep));

    *p = options;

    if (options & USB_OUT_ENABLED) {
        usb_configure_endpoint(ep, 0);
    }
    if (options & USB_IN_ENABLED) {
        usb_configure_endpoint(ep, 1);
    }
}

/*
 * STALLs the specified endpoint
 *
 * Input:
 *   unsigned ep - the endpoint the data will be transmitted on
 *   unsigned dir - the direction of the transfer
 */
void usb_stall_endpoint (unsigned ep, unsigned dir)
{
    BDT_ENTRY *p;

    if (ep == 0) {
        /*
         * If no one knows how to service this request then stall.
         * Must also prepare EP0 to receive the next SETUP transaction.
         */
        pBDTEntryEP0OutNext->CNT = USB_EP0_BUFF_SIZE;
        pBDTEntryEP0OutNext->ADR = ConvertToPhysicalAddress(&usb_setup_pkt);

        /* v2b fix */
        pBDTEntryEP0OutNext->STAT.Val = _USIE|_DAT0|_DTSEN|_BSTALL;
        pBDTEntryIn[0]->STAT.Val = _USIE|_BSTALL;
    } else {
        p = (BDT_ENTRY*) &usb_buffer[EP(ep, dir, 0)];
        p->STAT.Val |= _BSTALL | _USIE;

        //If the device is in FULL or ALL_BUT_EP0 ping pong modes
        //then stall that entry as well
#if (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG) || \
    (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0)

        p = (BDT_ENTRY*) &usb_buffer[EP(ep, dir, 1)];
        p->STAT.Val |= _BSTALL | _USIE;
#endif
    }
}

/*
 * Transfers one packet over the USB.
 *
 * Input:
 *   unsigned ep - the endpoint the data will be transmitted on
 *   unsigned dir - the direction of the transfer
 *                  This value is either OUT_FROM_HOST or IN_TO_HOST
 *   unsigned char* data - pointer to the data to be sent
 *   unsigned len - length of the data needing to be sent
 */
USB_HANDLE usb_transfer_one_packet (unsigned ep, unsigned dir,
    unsigned char* data, unsigned len)
{
    USB_HANDLE handle;

    // If the direction is IN
    if (dir != 0) {
        // point to the IN BDT of the specified endpoint
        handle = pBDTEntryIn[ep];
    } else {
        // else point to the OUT BDT of the specified endpoint
        handle = pBDTEntryOut[ep];
    }

    //Toggle the DTS bit if required
#if (USB_PING_PONG_MODE == USB_PING_PONG__NO_PING_PONG)
    handle->STAT.Val ^= _DTSMASK;
#elif (USB_PING_PONG_MODE == USB_PING_PONG__EP0_OUT_ONLY)
    if (ep != 0) {
        handle->STAT.Val ^= _DTSMASK;
    }
#endif

    //Set the data pointer, data length, and enable the endpoint
    handle->ADR = ConvertToPhysicalAddress(data);
    handle->CNT = len;
    handle->STAT.Val &= _DTSMASK;
    handle->STAT.Val |= _USIE | _DTSEN;

    // Point to the next buffer for ping pong purposes.
    if (dir != 0) {
        // toggle over the to the next buffer for an IN endpoint
        *(unsigned char*)&pBDTEntryIn[ep] ^= USB_NEXT_PING_PONG;
    } else {
        // toggle over the to the next buffer for an OUT endpoint
        *(unsigned char*)&pBDTEntryOut[ep] ^= USB_NEXT_PING_PONG;
    }
    return handle;
}

/*
 * USB Callback Functions
 */
/*
 * Call back that is invoked when a USB suspend is detected.
 */
void __attribute__((weak))
usbcb_suspend()
{
    /* Empty. */
}

/*
 * This call back is invoked when a wakeup from USB suspend is detected.
 */
void __attribute__((weak))
usbcb_wake_from_suspend()
{
    /* Empty. */
}

/*
 * Called when start-of-frame packet arrives, every 1 ms.
 */
void __attribute__((weak))
usbcb_sof_handler()
{
    /* Empty. */
}

/*
 * Called on any USB error interrupt, for debugging purposes.
 */
void __attribute__((weak))
usbcb_error_handler()
{
    /* Empty. */
}

/*
 * Handle a SETUP SET_DESCRIPTOR request (optional).
 */
void __attribute__((weak))
usbcb_std_set_dsc_handler()
{
    /* Empty. */
}
