/*
 * Chip configuration.
 */
#include "machine/pic32mx.h"

PIC32_DEVCFG (
    DEVCFG0_DEBUG_DISABLED,     /* ICE debugger enabled */

    DEVCFG1_FNOSC_PRIPLL |      /* Primary oscillator with PLL */
    DEVCFG1_POSCMOD_HS |        /* HS oscillator */
    DEVCFG1_FPBDIV_1 |          /* Peripheral bus clock = SYSCLK/1 */
    DEVCFG1_IESO |              /* Internal-external switch over */
    DEVCFG1_WDTPS_1,            /* Watchdog postscale = 1/1 */

    DEVCFG2_FPLLIDIV_2 |        /* PLL divider = 1/2 */
    DEVCFG2_FPLLMUL_20 |        /* PLL multiplier = 20x */
    DEVCFG2_UPLLIDIV_2 |        /* USB PLL divider = 1/2 */
    DEVCFG2_FPLLODIV_1,         /* PLL postscaler = 1/1 */

    DEVCFG3_USERID(0xffff) |    /* User-defined ID */
    DEVCFG3_FSRSSEL_7 |         /* Assign irq priority 7 to shadow set */
    DEVCFG3_FUSBIDIO |          /* USBID pin: controlled by USB */
    DEVCFG3_FVBUSONIO |         /* VBuson pin: controlled by USB */
    DEVCFG3_FCANIO |            /* Default CAN pins */
    DEVCFG3_FETHIO);            /* Default Ethernet i/o pins */
