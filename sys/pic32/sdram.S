/*
 * SDRAM Access Routines for PIC32.
 * 
 * Retromaster - 10.05.2010
 * 
 * This file is in the public domain. You can use, modify, and distribute the source code
 * and executable programs based on the source code. This file is provided "as is" and 
 * without any express or implied warranties whatsoever. Use at your own risk!
 *
 * Changes by jmcgee for inclusion in the retrobsd project. 
 */

/* SDRAM Used: HY57V281620 */

/*
 * See rd_sdramp_config.h for sdramp port/pin configuration
 */
#include "rd_sdramp_config.h"

/* Offsets (from TRISA) for the various port control registers */
#define TRIS_OFFSET   0x0
#define PORT_OFFSET   (PORTA-TRISA)
#define LAT_OFFSET    (LATA-TRISA)
#define ODCF_OFFSET   (ODCA-TRISA)

/* Offsets (from TRISA) for the various io port bit manipulator registers */
#define NOP_OP_OFFSET 0x0
#define CLR_OP_OFFSET (TRISACLR-TRISA)
#define SET_OP_OFFSET (TRISASET-TRISA)
#define INV_OP_OFFSET (TRISAINV-TRISA)

	/* Global Symbols */
	.globl sdram_init
	.globl sdram_read
	.globl sdram_write
	.globl sdram_active
	.globl sdram_auto_refresh
	.globl sdram_precharge
	.globl sdram_precharge_all
	.globl sdram_sleep
	.globl sdram_wake
	.globl sdram_bank
	
	.type sdram_init, @function
	.type sdram_read, @function
	.type sdram_write, @function
	.type sdram_active, @function
	.type sdram_auto_refresh, @function
	.type sdram_precharge, @function
	.type sdram_precharge_all, @function
	.type sdram_sleep, @function
	.type sdram_wake, @function
	.type sdram_bank, @function

    /* 
     * This code MUST execute from ram and the ram MUST be configured
     * for zero wait states. Interrupts MUST disabled before
     * calling any of these functions, and any DMA MUST also be
     * disabled.
     *
     * Also, the peripheral bus divisor must be set to 1.
     */

	.section .ramfunc,"ax",@progbits

    /* No instruction reordering */

    	.set noreorder

#define clock4 \
    nop;nop;nop;nop

#define clock3 \
    nop;nop;nop

#define clock2 \
    nop;nop

#define clock1 \
    nop

/*
 * The SDRAM clock is output from the output compare unit. 
 * This macro synchronizes with that clock so that we are
 * sure to have at least two clock cycles to issue control 
 * line changes and access the data bus before the rising
 * edge.
 */
 

#define sync_clock \
    	la	$t8, TMR2;      \
    	li 	$v0, 2;	        \
    	lw 	$v1, ($t8);     \
    	bge 	$v1, $v0, 1f;   \
    	nop;	                \
    	nop;	                \
    	nop;	                \
    1:      	                \
    	nop;							 

/*
 * Initializes the SDRAM. 
 * Should be called once sometime after startup
 * C Prototype:
 * extern __attribute__((far)) void sdram_init();
 * This should only be called from sdram_init_c,
 * which does all of the preliminary setup.
 */

sdram_init:

	/* Initialize address lines */
	la	$t0, TRISA	/* base of io addresses */

	/* Get ready for the commands we are about to issue. */
	li 	$t4, (1<<SDR_CONTROL_CAS_BIT) 	
	li 	$t5, (1<<SDR_CONTROL_WE_BIT) 

	/* Mode Register: CL:2, BL:8 (0x23) */
	/*li 	$t6, 0x1810*/
	//li	$t3, (1<<SDR_ADDRESS_LB_A1_BIT)|(1<<SDR_ADDRESS_LB_A0_BIT)
	//li	$t6, (1<<SDR_ADDRESS_A5_BIT)

	li 	$t7, CONTROL_ALL_MASK 
	li 	$t8, (1<<SDR_ADDRESS_A10_BIT)	/* A10 */

	sw	$t8, (SDR_ADDRESS_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0) /* A10 = 1 for Precharge ALL */

	sync_clock
	
	.set nomacro

	/* Precharge All */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0) /* LLLL */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0) /* LLHL */
	clock2

	/* Auto Refresh 1 */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0) /* LLLL */
	sw	$t5, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0) /* LLLH */
	clock2

	/* Auto Refresh 2 */
	clock4

	/* Auto Refresh 3 */
	clock4

	/* Auto Refresh 4 */
	clock4

	/* Auto Refresh 5 */
	li	$t4, ADDRESS_LB_MASK
	li	$t5, ADDRESS_MASK
	clock2

	/* Auto Refresh 6 */
	sw	$t4, (SDR_ADDRESS_LB_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0)
	sw	$t5, (SDR_ADDRESS_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0)
	clock2

	/* Mode Register: CL:2, BL:8 (0x23) */

	/* Auto Refresh 7 */
	li	$t4, (1<<SDR_ADDRESS_LB_A1_BIT)|(1<<SDR_ADDRESS_LB_A0_BIT)
	sw	$t4, (SDR_ADDRESS_LB_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0)
	clock2

	/* Auto Refresh 8 */
	li	$t4, (1<<SDR_ADDRESS_A5_BIT)
	sw	$t4, (SDR_ADDRESS_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0)
	clock2

	/* Load Mode Register */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0)
	clock3

	/* Command Inhibit */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0)
	clock3 

	/* Command Inhibit */
	clock4
	
	.set macro

	jr	$ra
	nop

	/*.end sdram_init*/


/* 
 * Sends ACTIVE command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_active();
 * Responsiblity of caller to output row address before calling this function.
 * See sdram_active_c( unsigned );
 */
sdram_active:

	la 	$t0, TRISA		/* Port Base */
	li 	$t7, (1<<SDR_CONTROL_CS_BIT)|(1<<SDR_CONTROL_RAS_BIT) 

	sync_clock
	
	.set nomacro 
	
	/* Active */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0)
	clock3

	/* Command Inhibit */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0)
	clock3

	/* Command Inhibit */
	clock4

	.set macro
	
	jr	$ra
	nop
	

/* 
 * Sends WRITE command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_write(uint64_t val);
 * Each pseudo column contains 8 bytes of data (consists of 8 ram columns)
 * Responsiblity of caller to output pseudo column address before calling this function.
 * See sdram_write_c( unsigned, uint64_t );
 */

sdram_write:

	la 	$t0, TRISA		/* Port Base */
	li 	$t4, 0xFF
	li 	$t7, (1<<SDR_CONTROL_CS_BIT) | (1<<SDR_CONTROL_CAS_BIT) | (1<<SDR_CONTROL_WE_BIT) 


//#ifdef SDRAM_FPGA_DIR_SUPPORT
//	li	$t3, (1<<SDR_DATA_DIR_BIT)
//	sw	$t3, (SDR_DATA_DIR_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0)
//#endif

	/* Set data lines */
	srl 	$t5, $a0, 24
	sb	$t5, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)
	
    	sync_clock	
	
	.set nomacro 
	
	/* Write */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LHLL */
	sw	$t4, (SDR_DATA_PORT-TRISA) + TRIS_OFFSET + CLR_OP_OFFSET($t0)   /* 1 - enable data lines */
	srl 	$t5, $a0, 16
	clock1

	/* Command Inhibit */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* HHHH */
	sb	$t5, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 2 */
	srl 	$t5, $a0, 8
	clock1

	/* Command Inhibit */
	sb	$t5, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 3 */
	clock3

	/* Command Inhibit */
	sb	$a0, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 4 */
	srl 	$t5, $a1, 24
	clock2

	/* Command Inhibit */
	sb	$t5, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 5 */
	srl 	$t5, $a1, 16
	clock2

	/* Command Inhibit */
	sb	$t5, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 6 */
	srl 	$t5, $a1, 8
	clock2

	/* Command Inhibit */
	sb	$t5, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 7 */
	clock3

	/* Command Inhibit */
	sb	$a1, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 8 */
	clock3

	sw	$t4, (SDR_DATA_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0)   /* PIC32 errata - thanks Pito. */
	sw	$t4, (SDR_DATA_PORT-TRISA) + TRIS_OFFSET + SET_OP_OFFSET($t0)   /* Data lines input again */

	.set macro
	
//#ifdef SDRAM_FPGA_DIR_SUPPORT
//	clock4
//	li	$t3, (1<<SDR_DATA_DIR_BIT)
//	sw	$t3, (SDR_DATA_DIR_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0)
//#endif


	jr	$ra
	nop


/* 
 * Sends READ command
 * C Prototype:
 * extern __attribute__((far)) uint64_t sdram_read();
 * Each pseudo column contains 8 bytes of data (consists of 8 ram columns)
 * Responsiblity of caller to output pseudo column address before calling this function.
 * See sdram_read_c()
 */
sdram_read:

	la 	$t0, TRISA		/* Port Base */
	li 	$t7, (1<<SDR_CONTROL_CS_BIT) | (1<<SDR_CONTROL_CAS_BIT) 

	sync_clock

	.set nomacro 
	
	/* Read */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LHLH */
	clock3

	/* Command Inhibit */
	sw	$t7, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* HHHH */
	clock3

	/* Command Inhibit */
	clock3
	lbu	$v0, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 1 */

	/* Command Inhibit */
	clock3
	lbu		$t5, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 2 */

	/* Command Inhibit */
	sll	$v0, $v0, 8
	or	$v0, $v0, $t5
	clock1
	lbu	$t5, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 3 */

	/* Command Inhibit */
	sll	$v0, $v0, 8
	or	$v0, $v0, $t5
	clock1
	lbu	$t5, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 4 */

	/* Command Inhibit */
	sll	$v0, $v0, 8
	or	$v0, $v0, $t5
	clock1
	lbu	$v1, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 5 */	

	/* Command Inhibit */
	clock3
	lbu	$t5, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 6 */	

	/* Command Inhibit */
	sll	$v1, $v1, 8
	or	$v1, $v1, $t5
	clock1
	lbu	$t5, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 7 */	

	/* Command Inhibit */
	sll	$v1, $v1, 8
	or	$v1, $v1, $t5
	clock1
	lbu	$t5, (SDR_DATA_PORT-TRISA) + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 8 */		

	/* Command Inhibit */
	sll	$v1, $v1, 8
	or	$v1, $v1, $t5

	.set macro 
	
	jr	$ra
	nop


/* 
 * Sends PRECHARGE ALL command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_precharge_all(void);
 */
sdram_precharge_all:

	la 	$t0, TRISA			/* Port Base */
	li 	$t3, (1<<SDR_ADDRESS_A10_BIT)	/* A10 */
	li 	$t4, (1<<SDR_CONTROL_CS_BIT) | (1<<SDR_CONTROL_RAS_BIT) | (1<<SDR_CONTROL_WE_BIT)

	sw	$t3, (SDR_ADDRESS_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0) /* A10 = 1 for Precharge ALL */

	sync_clock
	
	.set nomacro
	
	/* Precharge All */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLHL */
	clock3

	/* Command Inhibit */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro 
	
	jr	$ra
	nop


/* 
 * Sends PRECHARGE command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_precharge(void);
 */
sdram_precharge:
    
	la 	$t0, TRISA			/* Port Base */
	li 	$t3, (1<<SDR_ADDRESS_A10_BIT)	/* A10 */
	li 	$t4, (1<<SDR_CONTROL_CS_BIT) | (1<<SDR_CONTROL_RAS_BIT) | (1<<SDR_CONTROL_WE_BIT) 

	sw	$t3, (SDR_ADDRESS_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0) /* A10 = 0 for Precharge */

	sync_clock
	
	.set nomacro
	
	/* Precharge All */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLHL */
	clock3

	/* Command Inhibit */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro
	
	jr	$ra
	nop


/* 
 * Sends AUTO REFRESH command 
 * All banks must be in PRECHARGEd state
 * C Prototype:
 * extern __attribute__((far)) void sdram_auto_refresh(void);
 */
sdram_auto_refresh:

	la 	$t0, TRISA		/* Port Base */
	li 	$t4, (1<<SDR_CONTROL_CS_BIT)|(1<<SDR_CONTROL_RAS_BIT)|(1<<SDR_CONTROL_CAS_BIT) 

	sync_clock
	
	.set nomacro 
	
	/* Auto Refresh */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLH */
	clock3

	/* Command Inhibit */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro
	
	jr	$ra
	nop

/* 
 * Puts the SDRAM into the self refresh mode.
 * SDRAM retains data in this state.
 * C Prototype:
 * extern __attribute__((far)) void sdram_sleep(void);
 */
sdram_sleep:

	la 	$t0, TRISA		/* Port Base */
	li 	$t1, (1<<SDR_CKE_BIT) 
	li 	$t4, (1<<SDR_CONTROL_CS_BIT)|(1<<SDR_CONTROL_RAS_BIT)|(1<<SDR_CONTROL_CAS_BIT) 

	sync_clock
	
	.set nomacro
	
	/* Auto Refresh */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLH */
	sw 	$t1, (SDR_CKE_PORT-TRISA) + LAT_OFFSET + CLR_OP_OFFSET($t0)     /* CKE low */
	clock2

	/* Command Inhibit */
	sw	$t4, (SDR_CONTROL_PORT-TRISA) + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro
	
	jr	$ra
	nop

/* 
 * Takes the SDRAM out of the self refresh mode.
 * Parameters: none
 * C Prototype:
 * extern __attribute__((far)) void sdram_wake(void);
 */
sdram_wake:

	la 	$t0, TRISA		/* Port Base */
	li 	$t1, (1<<SDR_CKE_BIT) 

	sync_clock
	
	.set nomacro
	
	/* Command Inhibit */
	sw 	$t1, (SDR_CKE_PORT-TRISA) + LAT_OFFSET + SET_OP_OFFSET($t0) /* CKE low */
	clock3

	/* Command Inhibit */
	clock4

	/* Command Inhibit */
	clock4

	.set macro
	
	jr	$ra
	nop


