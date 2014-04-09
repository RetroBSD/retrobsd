#ifndef RD_SDRAMP_CONFIG_H_

#include "pic32mx.h"

/* TODO: better support for different sized sdram chips, 16 bit support */

/*
 * Number of physical address lines on sdram chip
 * one of 11, 12, 13
 */
#define SDR_ADDRESS_LINES 12

/*
 * Ram data width in bytes - 1 (8 bit) or 2 (16 bit)
 * 
 * NOT USED YET
 */
#define SDR_DATA_BYTES 2

/*
 * Upper/Lower Byte selection
 */
#define SDR_DQM_PORT TRISA

#if SDR_DATA_BYTES == 2
#define SDR_DQM_UDQM_BIT 6
#endif
#define SDR_DQM_LDQM_BIT 7

/*
 * Bank Selection
 * BA0 is connected to SDR_BANK_0_BIT
 * BA1 is connected to SDR_BANK_0_BIT + 1
 *
 * So, if SDR_BANK_0_BIT is 4, then bit 5
 * must be connected to BA1.
 */

#define SDR_BANK_PORT 	TRISG
#define SDR_BANK_0_BIT 	0

/*
 * Clock Enable
 *
 * Connect to CKE on sdram
 */
#define SDR_CKE_PORT 	TRISD
#define SDR_CKE_BIT 	11

/*
 * Control Lines
 *
 * Connect to /WE, /CAS, /CS and /RAS pins on sdram
 */
#define SDR_CONTROL_PORT 	TRISG
#define SDR_CONTROL_WE_BIT 	15
#define SDR_CONTROL_CAS_BIT 	13
#define SDR_CONTROL_CS_BIT 	14
#define SDR_CONTROL_RAS_BIT 	12

/*
 * Address Lines
 *
 * At present, the port can be changed, but
 * changing the address line bits is unsupported.
 */

#define SDR_ADDRESS_LB_PORT 	TRISF
#define SDR_ADDRESS_PORT 	TRISD

/***** WARNING - DO NOT CHANGE WITHOUT ALSO CHANGING CODE TO MATCH *****/
#define SDR_ADDRESS_LB_A0_BIT 	0
#define SDR_ADDRESS_LB_A1_BIT 	1
#define SDR_ADDRESS_LB_A2_BIT 	2

#define SDR_ADDRESS_A3_BIT 	1
#define SDR_ADDRESS_A4_BIT 	2
#define SDR_ADDRESS_A5_BIT 	3
#define SDR_ADDRESS_A6_BIT 	4
#define SDR_ADDRESS_A7_BIT 	5
#define SDR_ADDRESS_A8_BIT 	6
#define SDR_ADDRESS_A9_BIT 	7

#if SDR_ADDRESS_LINES >= 11
#define SDR_ADDRESS_A10_BIT 	8
#endif

#if SDR_ADDRESS_LINES >= 12
#define SDR_ADDRESS_A11_BIT 	9
#endif

#if SDR_ADDRESS_LINES >= 13
#define SDR_ADDRESS_A12_BIT 	10
#endif

/***** END WARNING *****/

/*
 * Data Lines
 *
 * The low 8 bits (bits 0-7) must be used
 * and connected to the data lines on the sdram.
 * The specific order in which the 8 pins are
 * connected to the data pins of the sdram is
 * not significant, unless you wish for a neat
 * and tidy design that is easy connect to a
 * logic analyzer for debugging purposes.
 */

#define SDR_DATA_PORT 	TRISE

/*
 * Output Compare
 *
 * Currently supporting OC1CON or OC4CON
 * Timer2 is used in all cases.
 * The appropriate pin should be connected to CLK on the sdram.
 * OC1CON - RD0
 * OC4CON - RD3
 */

#define SDR_OCR		OC1CON

/* 
 * Additional sdram connections
 *
 * Power and ground as appropriate.
 */


/***************************************************************/

/*
 * Anthing following should not normally need to be modified.
 * There are here in order to share definitions between C and ASM.
 */

#ifdef SDR_ADDRESS_A10_BIT
#define SDR_ADDRESS_A10_BITMASK (1<<SDR_ADDRESS_A10_BIT)
#else
#define SDR_ADDRESS_A10_BITMASK 0
#endif

#ifdef SDR_ADDRESS_A11_BIT
#define SDR_ADDRESS_A11_BITMASK (1<<SDR_ADDRESS_A11_BIT)
#else
#define SDR_ADDRESS_A11_BITMASK 0
#endif

#ifdef SDR_ADDRESS_A12_BIT
#define SDR_ADDRESS_A12_BITMASK (1<<SDR_ADDRESS_A12_BIT)
#else
#define SDR_ADDRESS_A12_BITMASK 0
#endif

#define ADDRESS_LB_MASK									\
	((1<<SDR_ADDRESS_LB_A0_BIT)|(1<<SDR_ADDRESS_LB_A1_BIT)|(1<<SDR_ADDRESS_LB_A2_BIT))

#define ADDRESS_MASK 									\
	((1<<SDR_ADDRESS_A3_BIT)|(1<<SDR_ADDRESS_A4_BIT)|(1<<SDR_ADDRESS_A5_BIT)|	\
	 (1<<SDR_ADDRESS_A6_BIT)|(1<<SDR_ADDRESS_A7_BIT)|(1<<SDR_ADDRESS_A8_BIT)|	\
	 (1<<SDR_ADDRESS_A9_BIT)| \
	 SDR_ADDRESS_A10_BITMASK| \
         SDR_ADDRESS_A11_BITMASK| \
         SDR_ADDRESS_A12_BITMASK)

#define CONTROL_ALL_MASK 					\
	((1<<SDR_CONTROL_CS_BIT)|(1<<SDR_CONTROL_RAS_BIT)| 	\
         (1<<SDR_CONTROL_CAS_BIT)|(1<<SDR_CONTROL_WE_BIT))

#define BANK_BITMASK 3
#define BANK_ALL_MASK (BANK_BITMASK << SDR_BANK_0_BIT)

#ifdef SDR_DQM_UDQM_BIT
#define SDR_DQM_MASK \
	((1<<SDR_DQM_LDQM_BIT)|(1<<SDR_DQM_UDQM_BIT))
#else
#define SDR_DQM_MASK \
	(1<<SDR_DQM_LDQM_BIT)
#endif

#endif
