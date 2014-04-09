/*
 * Chip configuration.
 */
#include "machine/pic32mx.h"

PIC32_DEVCFG (
    DEVCFG0_DEBUG_ENABLED,      /* ICE debugger enabled */

    DEVCFG1_FNOSC_FRCDIVPLL |   /* Primary oscillator with PLL */
    DEVCFG1_POSCMOD_DISABLE |        /* HS oscillator */
    DEVCFG1_OSCIOFNC |          /* CLKO output active */
    DEVCFG1_FPBDIV_1 |          /* Peripheral bus clock = SYSCLK/1 */
    DEVCFG1_FCKM_DISABLE |      /* Fail-safe clock monitor disable */
    DEVCFG1_FCKS_DISABLE |      /* Clock switching disable */
    DEVCFG1_WDTPS_1024,         /* Watchdog postscale = 1/1024 */

    DEVCFG2_FPLLIDIV_2 |        /* PLL divider = 1/2 */
    DEVCFG2_FPLLMUL_20 |        /* PLL multiplier = 20x */
    DEVCFG2_UPLLIDIV_2 |        /* USB PLL divider = 1/2 */
    DEVCFG2_UPLLDIS |           /* Disable USB PLL */
    DEVCFG2_FPLLODIV_1,         /* PLL postscaler = 1/1 */

    DEVCFG3_USERID(0xffff) |    /* User-defined ID */
    DEVCFG3_FSRSSEL_7 |         /* Assign irq priority 7 to shadow set */
    DEVCFG3_FETHIO);            /* Default Ethernet i/o pins */
