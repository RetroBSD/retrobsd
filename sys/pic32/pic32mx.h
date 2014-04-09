/*
 * Hardware register defines for all Microchip PIC32MX microcontrollers.
 *
 * Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
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
#ifndef _IO_PIC32MX_H
#define _IO_PIC32MX_H

/*--------------------------------------
 * Coprocessor 0 registers.
 */
#define C0_HWRENA	7	/* Enable RDHWR in non-privileged mode */
#define C0_BADVADDR	8	/* Virtual address of last exception */
#define C0_COUNT	9	/* Processor cycle count */
#define C0_COMPARE	11	/* Timer interrupt control */
#define C0_STATUS	12	/* Processor status and control */
#define C0_INTCTL	12	/* Select 1: interrupt control */
#define C0_SRSCTL	12	/* Select 2: GPR shadow set control */
#define C0_SRSMAP	12	/* Select 3: vector to shadow set mapping */
#define C0_CAUSE	13	/* Cause of last exception */
#define C0_EPC		14	/* Program counter at last exception */
#define C0_PRID		15	/* Processor identification (read only) */
#define C0_EBASE	15	/* Select 1: exception base address */
#define C0_CONFIG	16	/* Configuration */
#define C0_CONFIG1	16	/* Select 1: configuration 1 */
#define C0_CONFIG2	16	/* Select 2: configuration 2 */
#define C0_CONFIG3	16	/* Select 3: configuration 3 */
#define C0_DEBUG	23	/* Debug control and status */
#define C0_DEPC		24	/* Program counter at last debug exception */
#define C0_ERROREPC	30	/* Program counter at last error */
#define C0_DESAVE	31	/* Debug handler scratchpad register */

/*
 * Status register.
 */
#define ST_CU0		0x10000000	/* Access to coprocessor 0 allowed (in user mode) */
#define ST_RP		0x08000000	/* Enable reduced power mode */
#define ST_RE		0x02000000	/* Reverse endianness (in user mode) */
#define ST_BEV		0x00400000	/* Exception vectors: bootstrap */
#define ST_SR		0x00100000	/* Soft reset */
#define ST_NMI		0x00080000	/* NMI reset */
#define ST_IPL(x)	((x) << 10)	/* Current interrupt priority level */
#define ST_UM		0x00000010	/* User mode */
#define ST_ERL		0x00000004	/* Error level */
#define ST_EXL		0x00000002	/* Exception level */
#define ST_IE		0x00000001	/* Interrupt enable */

/*
 * Сause register.
 */
#define CA_BD		0x80000000	/* Exception occured in delay slot */
#define CA_TI		0x40000000	/* Timer interrupt is pending */
#define CA_CE		0x30000000	/* Coprocessor exception */
#define CA_DC		0x08000000	/* Disable COUNT register */
#define CA_IV		0x00800000	/* Use special interrupt vector 0x200 */
#define CA_RIPL(r)	((r)>>10 & 63)	/* Requested interrupt priority level */
#define CA_IP1		0x00020000	/* Request software interrupt 1 */
#define CA_IP0		0x00010000	/* Request software interrupt 0 */
#define CA_EXC_CODE	0x0000007c	/* Exception code */

#define CA_Int		0		/* Interrupt */
#define CA_AdEL		(4 << 2)	/* Address error, load or instruction fetch */
#define CA_AdES		(5 << 2)	/* Address error, store */
#define CA_IBE		(6 << 2)	/* Bus error, instruction fetch */
#define CA_DBE		(7 << 2)	/* Bus error, load or store */
#define CA_Sys		(8 << 2)	/* Syscall */
#define CA_Bp		(9 << 2)	/* Breakpoint */
#define CA_RI		(10 << 2)	/* Reserved instruction */
#define CA_CPU		(11 << 2)	/* Coprocessor unusable */
#define CA_Ov		(12 << 2)	/* Arithmetic overflow */
#define CA_Tr		(13 << 2)	/* Trap */

#define DB_DBD          (1 << 31)       /* Debug exception in a branch delay slot */
#define DB_DM           (1 << 30)       /* Debug mode */
#define DB_NODCR        (1 << 29)       /* No dseg present */
#define DB_LSNM         (1 << 28)       /* Load/stores in dseg go to main memory */
#define DB_DOZE         (1 << 27)       /* Processor was in low-power mode */
#define DB_HALT         (1 << 26)       /* Internal system bus clock was running */
#define DB_COUNTDM      (1 << 25)       /* Count register is running in Debug mode */
#define DB_IBUSEP       (1 << 24)       /* Instruction fetch bus error exception */
#define DB_DBUSEP       (1 << 21)       /* Data access bus error exception */
#define DB_IEXI         (1 << 20)       /* Imprecise error exception */
#define DB_VER          (7 << 15)       /* EJTAG version number */
#define DB_DEXCCODE     (0x1f << 10)    /* Cause of exception in Debug mode */
#define DB_SST          (1 << 8)        /* Single step exception enabled */
#define DB_DIBImpr      (1 << 6)        /* Imprecise debug instruction break */
#define DB_DINT         (1 << 5)        /* Debug interrupt exception */
#define DB_DIB          (1 << 4)        /* Debug instruction break exception */
#define DB_DDBS         (1 << 3)        /* Debug data break exception on store */
#define DB_DDBL         (1 << 2)        /* Debug data break exception on load */
#define DB_DBP          (1 << 1)        /* Debug software breakpoint exception */
#define DB_DSS          (1 << 0)        /* Debug single-step exception */

/*--------------------------------------
 * Peripheral registers.
 */
#ifdef __ASSEMBLER__
#define PIC32_R(a)		(0xBF800000 + (a))
#else
#define PIC32_R(a)		*(volatile unsigned*)(0xBF800000 + (a))
#endif
/*--------------------------------------
 * UART registers.
 */
#define U1MODE		PIC32_R (0x6000) /* Mode */
#define U1MODECLR	PIC32_R (0x6004)
#define U1MODESET	PIC32_R (0x6008)
#define U1MODEINV	PIC32_R (0x600C)
#define U1STA		PIC32_R (0x6010) /* Status and control */
#define U1STACLR	PIC32_R (0x6014)
#define U1STASET	PIC32_R (0x6018)
#define U1STAINV	PIC32_R (0x601C)
#define U1TXREG		PIC32_R (0x6020) /* Transmit */
#define U1RXREG		PIC32_R (0x6030) /* Receive */
#define U1BRG		PIC32_R (0x6040) /* Baud rate */
#define U1BRGCLR	PIC32_R (0x6044)
#define U1BRGSET	PIC32_R (0x6048)
#define U1BRGINV	PIC32_R (0x604C)

#ifdef PIC32MX4
#define U2MODE		PIC32_R (0x6200) /* Mode */
#define U2MODECLR	PIC32_R (0x6204)
#define U2MODESET	PIC32_R (0x6208)
#define U2MODEINV	PIC32_R (0x620C)
#define U2STA		PIC32_R (0x6210) /* Status and control */
#define U2STACLR	PIC32_R (0x6214)
#define U2STASET	PIC32_R (0x6218)
#define U2STAINV	PIC32_R (0x621C)
#define U2TXREG		PIC32_R (0x6220) /* Transmit */
#define U2RXREG		PIC32_R (0x6230) /* Receive */
#define U2BRG		PIC32_R (0x6240) /* Baud rate */
#define U2BRGCLR	PIC32_R (0x6244)
#define U2BRGSET	PIC32_R (0x6248)
#define U2BRGINV	PIC32_R (0x624C)
#endif
#ifdef PIC32MX7
#define U4MODE		PIC32_R (0x6200) /* Mode */
#define U4MODECLR	PIC32_R (0x6204)
#define U4MODESET	PIC32_R (0x6208)
#define U4MODEINV	PIC32_R (0x620C)
#define U4STA		PIC32_R (0x6210) /* Status and control */
#define U4STACLR	PIC32_R (0x6214)
#define U4STASET	PIC32_R (0x6218)
#define U4STAINV	PIC32_R (0x621C)
#define U4TXREG		PIC32_R (0x6220) /* Transmit */
#define U4RXREG		PIC32_R (0x6230) /* Receive */
#define U4BRG		PIC32_R (0x6240) /* Baud rate */
#define U4BRGCLR	PIC32_R (0x6244)
#define U4BRGSET	PIC32_R (0x6248)
#define U4BRGINV	PIC32_R (0x624C)

#define U3MODE		PIC32_R (0x6400) /* Mode */
#define U3MODECLR	PIC32_R (0x6404)
#define U3MODESET	PIC32_R (0x6408)
#define U3MODEINV	PIC32_R (0x640C)
#define U3STA		PIC32_R (0x6410) /* Status and control */
#define U3STACLR	PIC32_R (0x6414)
#define U3STASET	PIC32_R (0x6418)
#define U3STAINV	PIC32_R (0x641C)
#define U3TXREG		PIC32_R (0x6420) /* Transmit */
#define U3RXREG		PIC32_R (0x6430) /* Receive */
#define U3BRG		PIC32_R (0x6440) /* Baud rate */
#define U3BRGCLR	PIC32_R (0x6444)
#define U3BRGSET	PIC32_R (0x6448)
#define U3BRGINV	PIC32_R (0x644C)

#define U6MODE		PIC32_R (0x6600) /* Mode */
#define U6MODECLR	PIC32_R (0x6604)
#define U6MODESET	PIC32_R (0x6608)
#define U6MODEINV	PIC32_R (0x660C)
#define U6STA		PIC32_R (0x6610) /* Status and control */
#define U6STACLR	PIC32_R (0x6614)
#define U6STASET	PIC32_R (0x6618)
#define U6STAINV	PIC32_R (0x661C)
#define U6TXREG		PIC32_R (0x6620) /* Transmit */
#define U6RXREG		PIC32_R (0x6630) /* Receive */
#define U6BRG		PIC32_R (0x6640) /* Baud rate */
#define U6BRGCLR	PIC32_R (0x6644)
#define U6BRGSET	PIC32_R (0x6648)
#define U6BRGINV	PIC32_R (0x664C)

#define U2MODE		PIC32_R (0x6800) /* Mode */
#define U2MODECLR	PIC32_R (0x6804)
#define U2MODESET	PIC32_R (0x6808)
#define U2MODEINV	PIC32_R (0x680C)
#define U2STA		PIC32_R (0x6810) /* Status and control */
#define U2STACLR	PIC32_R (0x6814)
#define U2STASET	PIC32_R (0x6818)
#define U2STAINV	PIC32_R (0x681C)
#define U2TXREG		PIC32_R (0x6820) /* Transmit */
#define U2RXREG		PIC32_R (0x6830) /* Receive */
#define U2BRG		PIC32_R (0x6840) /* Baud rate */
#define U2BRGCLR	PIC32_R (0x6844)
#define U2BRGSET	PIC32_R (0x6848)
#define U2BRGINV	PIC32_R (0x684C)

#define U5MODE		PIC32_R (0x6A00) /* Mode */
#define U5MODECLR	PIC32_R (0x6A04)
#define U5MODESET	PIC32_R (0x6A08)
#define U5MODEINV	PIC32_R (0x6A0C)
#define U5STA		PIC32_R (0x6A10) /* Status and control */
#define U5STACLR	PIC32_R (0x6A14)
#define U5STASET	PIC32_R (0x6A18)
#define U5STAINV	PIC32_R (0x6A1C)
#define U5TXREG		PIC32_R (0x6A20) /* Transmit */
#define U5RXREG		PIC32_R (0x6A30) /* Receive */
#define U5BRG		PIC32_R (0x6A40) /* Baud rate */
#define U5BRGCLR	PIC32_R (0x6A44)
#define U5BRGSET	PIC32_R (0x6A48)
#define U5BRGINV	PIC32_R (0x6A4C)
#endif

/*
 * UART Mode register.
 */
#define PIC32_UMODE_STSEL	0x0001	/* 2 Stop bits */
#define PIC32_UMODE_PDSEL	0x0006	/* Bitmask: */
#define PIC32_UMODE_PDSEL_8NPAR	0x0000	/* 8-bit data, no parity */
#define PIC32_UMODE_PDSEL_8EVEN	0x0002	/* 8-bit data, even parity */
#define PIC32_UMODE_PDSEL_8ODD	0x0004	/* 8-bit data, odd parity */
#define PIC32_UMODE_PDSEL_9NPAR	0x0006	/* 9-bit data, no parity */
#define PIC32_UMODE_BRGH	0x0008	/* High Baud Rate Enable */
#define PIC32_UMODE_RXINV	0x0010	/* Receive Polarity Inversion */
#define PIC32_UMODE_ABAUD	0x0020	/* Auto-Baud Enable */
#define PIC32_UMODE_LPBACK	0x0040	/* UARTx Loopback Mode */
#define PIC32_UMODE_WAKE	0x0080	/* Wake-up on start bit during Sleep Mode */
#define PIC32_UMODE_UEN		0x0300	/* Bitmask: */
#define PIC32_UMODE_UEN_RTS	0x0100	/* Using UxRTS pin */
#define PIC32_UMODE_UEN_RTSCTS	0x0200	/* Using UxCTS and UxRTS pins */
#define PIC32_UMODE_UEN_BCLK	0x0300	/* Using UxBCLK pin */
#define PIC32_UMODE_RTSMD	0x0800	/* UxRTS Pin Simplex mode */
#define PIC32_UMODE_IREN	0x1000	/* IrDA Encoder and Decoder Enable bit */
#define PIC32_UMODE_SIDL	0x2000	/* Stop in Idle Mode */
#define PIC32_UMODE_FRZ		0x4000	/* Freeze in Debug Exception Mode */
#define PIC32_UMODE_ON		0x8000	/* UART Enable */

/*
 * UART Control and status register.
 */
#define PIC32_USTA_URXDA	0x00000001 /* Receive Data Available (read-only) */
#define PIC32_USTA_OERR		0x00000002 /* Receive Buffer Overrun */
#define PIC32_USTA_FERR		0x00000004 /* Framing error detected (read-only) */
#define PIC32_USTA_PERR		0x00000008 /* Parity error detected (read-only) */
#define PIC32_USTA_RIDLE	0x00000010 /* Receiver is idle (read-only) */
#define PIC32_USTA_ADDEN	0x00000020 /* Address Detect mode */
#define PIC32_USTA_URXISEL	0x000000C0 /* Bitmask: receive interrupt is set when... */
#define PIC32_USTA_URXISEL_NEMP	0x00000000 /* ...receive buffer is not empty */
#define PIC32_USTA_URXISEL_HALF	0x00000040 /* ...receive buffer becomes 1/2 full */
#define PIC32_USTA_URXISEL_3_4	0x00000080 /* ...receive buffer becomes 3/4 full */
#define PIC32_USTA_TRMT		0x00000100 /* Transmit shift register is empty (read-only) */
#define PIC32_USTA_UTXBF	0x00000200 /* Transmit buffer is full (read-only) */
#define PIC32_USTA_UTXEN	0x00000400 /* Transmit Enable */
#define PIC32_USTA_UTXBRK	0x00000800 /* Transmit Break */
#define PIC32_USTA_URXEN	0x00001000 /* Receiver Enable */
#define PIC32_USTA_UTXINV	0x00002000 /* Transmit Polarity Inversion */
#define PIC32_USTA_UTXISEL	0x0000C000 /* Bitmask: TX interrupt is generated when... */
#define PIC32_USTA_UTXISEL_1	0x00000000 /* ...the transmit buffer contains at least one empty space */
#define PIC32_USTA_UTXISEL_ALL	0x00004000 /* ...all characters have been transmitted */
#define PIC32_USTA_UTXISEL_EMP	0x00008000 /* ...the transmit buffer becomes empty */
#define PIC32_USTA_ADDR		0x00FF0000 /* Automatic Address Mask */
#define PIC32_USTA_ADM_EN	0x01000000 /* Automatic Address Detect */

/*
 * Compute the 16-bit baud rate divisor, given
 * the bus frequency and baud rate.
 * Round to the nearest integer.
 */
#define PIC32_BRG_BAUD(fr,bd)	((((fr)/8 + (bd)) / (bd) / 2) - 1)

/*--------------------------------------
 * Port A-G registers.
 */
#define TRISA		PIC32_R (0x86000) /* Port A: mask of inputs */
#define TRISACLR	PIC32_R (0x86004)
#define TRISASET	PIC32_R (0x86008)
#define TRISAINV	PIC32_R (0x8600C)
#define PORTA		PIC32_R (0x86010) /* Port A: read inputs, write outputs */
#define PORTACLR	PIC32_R (0x86014)
#define PORTASET	PIC32_R (0x86018)
#define PORTAINV	PIC32_R (0x8601C)
#define LATA		PIC32_R (0x86020) /* Port A: read/write outputs */
#define LATACLR		PIC32_R (0x86024)
#define LATASET		PIC32_R (0x86028)
#define LATAINV		PIC32_R (0x8602C)
#define ODCA		PIC32_R (0x86030) /* Port A: open drain configuration */
#define ODCACLR		PIC32_R (0x86034)
#define ODCASET		PIC32_R (0x86038)
#define ODCAINV		PIC32_R (0x8603C)

#define TRISB		PIC32_R (0x86040) /* Port B: mask of inputs */
#define TRISBCLR	PIC32_R (0x86044)
#define TRISBSET	PIC32_R (0x86048)
#define TRISBINV	PIC32_R (0x8604C)
#define PORTB		PIC32_R (0x86050) /* Port B: read inputs, write outputs */
#define PORTBCLR	PIC32_R (0x86054)
#define PORTBSET	PIC32_R (0x86058)
#define PORTBINV	PIC32_R (0x8605C)
#define LATB		PIC32_R (0x86060) /* Port B: read/write outputs */
#define LATBCLR		PIC32_R (0x86064)
#define LATBSET		PIC32_R (0x86068)
#define LATBINV		PIC32_R (0x8606C)
#define ODCB		PIC32_R (0x86070) /* Port B: open drain configuration */
#define ODCBCLR		PIC32_R (0x86074)
#define ODCBSET		PIC32_R (0x86078)
#define ODCBINV		PIC32_R (0x8607C)

#define TRISC		PIC32_R (0x86080) /* Port C: mask of inputs */
#define TRISCCLR	PIC32_R (0x86084)
#define TRISCSET	PIC32_R (0x86088)
#define TRISCINV	PIC32_R (0x8608C)
#define PORTC		PIC32_R (0x86090) /* Port C: read inputs, write outputs */
#define PORTCCLR	PIC32_R (0x86094)
#define PORTCSET	PIC32_R (0x86098)
#define PORTCINV	PIC32_R (0x8609C)
#define LATC		PIC32_R (0x860A0) /* Port C: read/write outputs */
#define LATCCLR		PIC32_R (0x860A4)
#define LATCSET		PIC32_R (0x860A8)
#define LATCINV		PIC32_R (0x860AC)
#define ODCC		PIC32_R (0x860B0) /* Port C: open drain configuration */
#define ODCCCLR		PIC32_R (0x860B4)
#define ODCCSET		PIC32_R (0x860B8)
#define ODCCINV		PIC32_R (0x860BC)

#define TRISD		PIC32_R (0x860C0) /* Port D: mask of inputs */
#define TRISDCLR	PIC32_R (0x860C4)
#define TRISDSET	PIC32_R (0x860C8)
#define TRISDINV	PIC32_R (0x860CC)
#define PORTD		PIC32_R (0x860D0) /* Port D: read inputs, write outputs */
#define PORTDCLR	PIC32_R (0x860D4)
#define PORTDSET	PIC32_R (0x860D8)
#define PORTDINV	PIC32_R (0x860DC)
#define LATD		PIC32_R (0x860E0) /* Port D: read/write outputs */
#define LATDCLR		PIC32_R (0x860E4)
#define LATDSET		PIC32_R (0x860E8)
#define LATDINV		PIC32_R (0x860EC)
#define ODCD		PIC32_R (0x860F0) /* Port D: open drain configuration */
#define ODCDCLR		PIC32_R (0x860F4)
#define ODCDSET		PIC32_R (0x860F8)
#define ODCDINV		PIC32_R (0x860FC)

#define TRISE		PIC32_R (0x86100) /* Port E: mask of inputs */
#define TRISECLR	PIC32_R (0x86104)
#define TRISESET	PIC32_R (0x86108)
#define TRISEINV	PIC32_R (0x8610C)
#define PORTE		PIC32_R (0x86110) /* Port E: read inputs, write outputs */
#define PORTECLR	PIC32_R (0x86114)
#define PORTESET	PIC32_R (0x86118)
#define PORTEINV	PIC32_R (0x8611C)
#define LATE		PIC32_R (0x86120) /* Port E: read/write outputs */
#define LATECLR		PIC32_R (0x86124)
#define LATESET		PIC32_R (0x86128)
#define LATEINV		PIC32_R (0x8612C)
#define ODCE		PIC32_R (0x86130) /* Port E: open drain configuration */
#define ODCECLR		PIC32_R (0x86134)
#define ODCESET		PIC32_R (0x86138)
#define ODCEINV		PIC32_R (0x8613C)

#define TRISF		PIC32_R (0x86140) /* Port F: mask of inputs */
#define TRISFCLR	PIC32_R (0x86144)
#define TRISFSET	PIC32_R (0x86148)
#define TRISFINV	PIC32_R (0x8614C)
#define PORTF		PIC32_R (0x86150) /* Port F: read inputs, write outputs */
#define PORTFCLR	PIC32_R (0x86154)
#define PORTFSET	PIC32_R (0x86158)
#define PORTFINV	PIC32_R (0x8615C)
#define LATF		PIC32_R (0x86160) /* Port F: read/write outputs */
#define LATFCLR		PIC32_R (0x86164)
#define LATFSET		PIC32_R (0x86168)
#define LATFINV		PIC32_R (0x8616C)
#define ODCF		PIC32_R (0x86170) /* Port F: open drain configuration */
#define ODCFCLR		PIC32_R (0x86174)
#define ODCFSET		PIC32_R (0x86178)
#define ODCFINV		PIC32_R (0x8617C)

#define TRISG		PIC32_R (0x86180) /* Port G: mask of inputs */
#define TRISGCLR	PIC32_R (0x86184)
#define TRISGSET	PIC32_R (0x86188)
#define TRISGINV	PIC32_R (0x8618C)
#define PORTG		PIC32_R (0x86190) /* Port G: read inputs, write outputs */
#define PORTGCLR	PIC32_R (0x86194)
#define PORTGSET	PIC32_R (0x86198)
#define PORTGINV	PIC32_R (0x8619C)
#define LATG		PIC32_R (0x861A0) /* Port G: read/write outputs */
#define LATGCLR		PIC32_R (0x861A4)
#define LATGSET		PIC32_R (0x861A8)
#define LATGINV		PIC32_R (0x861AC)
#define ODCG		PIC32_R (0x861B0) /* Port G: open drain configuration */
#define ODCGCLR		PIC32_R (0x861B4)
#define ODCGSET		PIC32_R (0x861B8)
#define ODCGINV		PIC32_R (0x861BC)

#define CNCON		PIC32_R (0x861C0) /* Interrupt-on-change control */
#define CNCONCLR	PIC32_R (0x861C4)
#define CNCONSET	PIC32_R (0x861C8)
#define CNCONINV	PIC32_R (0x861CC)
#define CNEN		PIC32_R (0x861D0) /* Input change interrupt enable */
#define CNENCLR		PIC32_R (0x861D4)
#define CNENSET		PIC32_R (0x861D8)
#define CNENINV		PIC32_R (0x861DC)
#define CNPUE		PIC32_R (0x861E0) /* Input pin pull-up enable */
#define CNPUECLR	PIC32_R (0x861E4)
#define CNPUESET	PIC32_R (0x861E8)
#define CNPUEINV	PIC32_R (0x861EC)

/*--------------------------------------
 * A/D Converter registers.
 */
#define AD1CON1		PIC32_R (0x9000) /* Control register 1 */
#define AD1CON1CLR	PIC32_R (0x9004)
#define AD1CON1SET	PIC32_R (0x9008)
#define AD1CON1INV	PIC32_R (0x900C)
#define AD1CON2		PIC32_R (0x9010) /* Control register 2 */
#define AD1CON2CLR	PIC32_R (0x9014)
#define AD1CON2SET	PIC32_R (0x9018)
#define AD1CON2INV	PIC32_R (0x901C)
#define AD1CON3		PIC32_R (0x9020) /* Control register 3 */
#define AD1CON3CLR	PIC32_R (0x9024)
#define AD1CON3SET	PIC32_R (0x9028)
#define AD1CON3INV	PIC32_R (0x902C)
#define AD1CHS		PIC32_R (0x9040) /* Channel select */
#define AD1CHSCLR	PIC32_R (0x9044)
#define AD1CHSSET	PIC32_R (0x9048)
#define AD1CHSINV	PIC32_R (0x904C)
#define AD1CSSL		PIC32_R (0x9050) /* Input scan selection */
#define AD1CSSLCLR	PIC32_R (0x9054)
#define AD1CSSLSET	PIC32_R (0x9058)
#define AD1CSSLINV	PIC32_R (0x905C)
#define AD1PCFG		PIC32_R (0x9060) /* Port configuration */
#define AD1PCFGCLR	PIC32_R (0x9064)
#define AD1PCFGSET	PIC32_R (0x9068)
#define AD1PCFGINV	PIC32_R (0x906C)
#define ADC1BUF0	PIC32_R (0x9070) /* Result words */
#define ADC1BUF1	PIC32_R (0x9080)
#define ADC1BUF2	PIC32_R (0x9090)
#define ADC1BUF3	PIC32_R (0x90A0)
#define ADC1BUF4	PIC32_R (0x90B0)
#define ADC1BUF5	PIC32_R (0x90C0)
#define ADC1BUF6	PIC32_R (0x90D0)
#define ADC1BUF7	PIC32_R (0x90E0)
#define ADC1BUF8	PIC32_R (0x90F0)
#define ADC1BUF9	PIC32_R (0x9100)
#define ADC1BUFA	PIC32_R (0x9110)
#define ADC1BUFB	PIC32_R (0x9120)
#define ADC1BUFC	PIC32_R (0x9130)
#define ADC1BUFD	PIC32_R (0x9140)
#define ADC1BUFE	PIC32_R (0x9150)
#define ADC1BUFF	PIC32_R (0x9160)

/*--------------------------------------
 * Parallel master port registers.
 */
#define PMCON		PIC32_R (0x7000) /* Control */
#define PMCONCLR	PIC32_R (0x7004)
#define PMCONSET	PIC32_R (0x7008)
#define PMCONINV	PIC32_R (0x700C)
#define PMMODE		PIC32_R (0x7010) /* Mode */
#define PMMODECLR	PIC32_R (0x7014)
#define PMMODESET	PIC32_R (0x7018)
#define PMMODEINV	PIC32_R (0x701C)
#define PMADDR		PIC32_R (0x7020) /* Address */
#define PMADDRCLR	PIC32_R (0x7024)
#define PMADDRSET	PIC32_R (0x7028)
#define PMADDRINV	PIC32_R (0x702C)
#define PMDOUT		PIC32_R (0x7030) /* Data output */
#define PMDOUTCLR	PIC32_R (0x7034)
#define PMDOUTSET	PIC32_R (0x7038)
#define PMDOUTINV	PIC32_R (0x703C)
#define PMDIN		PIC32_R (0x7040) /* Data input */
#define PMDINCLR	PIC32_R (0x7044)
#define PMDINSET	PIC32_R (0x7048)
#define PMDININV	PIC32_R (0x704C)
#define PMAEN		PIC32_R (0x7050) /* Pin enable */
#define PMAENCLR	PIC32_R (0x7054)
#define PMAENSET	PIC32_R (0x7058)
#define PMAENINV	PIC32_R (0x705C)
#define PMSTAT		PIC32_R (0x7060) /* Status (slave only) */
#define PMSTATCLR	PIC32_R (0x7064)
#define PMSTATSET	PIC32_R (0x7068)
#define PMSTATINV	PIC32_R (0x706C)

/*
 * PMP Control register.
 */
#define PIC32_PMCON_RDSP	0x0001 /* Read strobe polarity active-high */
#define PIC32_PMCON_WRSP	0x0002 /* Write strobe polarity active-high */
#define PIC32_PMCON_CS1P	0x0008 /* Chip select 0 polarity active-high */
#define PIC32_PMCON_CS2P	0x0010 /* Chip select 1 polarity active-high */
#define PIC32_PMCON_ALP		0x0020 /* Address latch polarity active-high */
#define PIC32_PMCON_CSF		0x00C0 /* Chip select function bitmask: */
#define PIC32_PMCON_CSF_NONE	0x0000 /* PMCS2 and PMCS1 as A[15:14] */
#define PIC32_PMCON_CSF_CS2	0x0040 /* PMCS2 as chip select */
#define PIC32_PMCON_CSF_CS21	0x0080 /* PMCS2 and PMCS1 as chip select */
#define PIC32_PMCON_PTRDEN	0x0100 /* Read/write strobe port enable */
#define PIC32_PMCON_PTWREN	0x0200 /* Write enable strobe port enable */
#define PIC32_PMCON_PMPTTL	0x0400 /* TTL input buffer select */
#define PIC32_PMCON_ADRMUX	0x1800 /* Address/data mux selection bitmask: */
#define PIC32_PMCON_ADRMUX_NONE	0x0000 /* Address and data separate */
#define PIC32_PMCON_ADRMUX_AD	0x0800 /* Lower address on PMD[7:0], high on PMA[15:8] */
#define PIC32_PMCON_ADRMUX_D8	0x1000 /* All address on PMD[7:0] */
#define PIC32_PMCON_ADRMUX_D16	0x1800 /* All address on PMD[15:0] */
#define PIC32_PMCON_SIDL	0x2000 /* Stop in idle */
#define PIC32_PMCON_FRZ		0x4000 /* Freeze in debug exception */
#define PIC32_PMCON_ON		0x8000 /* Parallel master port enable */

/*
 * PMP Mode register.
 */
#define PIC32_PMMODE_WAITE(x)	((x)<<0) /* Wait states: data hold after RW strobe */
#define PIC32_PMMODE_WAITM(x)	((x)<<2) /* Wait states: data RW strobe */
#define PIC32_PMMODE_WAITB(x)	((x)<<6) /* Wait states: data setup to RW strobe */
#define PIC32_PMMODE_MODE	0x0300	/* Mode select bitmask: */
#define PIC32_PMMODE_MODE_SLAVE	0x0000	/* Legacy slave */
#define PIC32_PMMODE_MODE_SLENH	0x0100	/* Enhanced slave */
#define PIC32_PMMODE_MODE_MAST2	0x0200	/* Master mode 2 */
#define PIC32_PMMODE_MODE_MAST1	0x0300	/* Master mode 1 */
#define PIC32_PMMODE_MODE16	0x0400	/* 16-bit mode */
#define PIC32_PMMODE_INCM	0x1800	/* Address increment mode bitmask: */
#define PIC32_PMMODE_INCM_NONE	0x0000	/* No increment/decrement */
#define PIC32_PMMODE_INCM_INC	0x0800	/* Increment address */
#define PIC32_PMMODE_INCM_DEC	0x1000	/* Decrement address */
#define PIC32_PMMODE_INCM_SLAVE	0x1800	/* Slave auto-increment */
#define PIC32_PMMODE_IRQM	0x6000	/* Interrupt request bitmask: */
#define PIC32_PMMODE_IRQM_DIS	0x0000	/* No interrupt generated */
#define PIC32_PMMODE_IRQM_END	0x2000	/* Interrupt at end of read/write cycle */
#define PIC32_PMMODE_IRQM_A3	0x4000	/* Interrupt on address 3 */
#define PIC32_PMMODE_BUSY	0x8000	/* Port is busy */

/*
 * PMP Address register.
 */
#define PIC32_PMADDR_PADDR	0x3FFF /* Destination address */
#define PIC32_PMADDR_CS1	0x4000 /* Chip select 1 is active */
#define PIC32_PMADDR_CS2	0x8000 /* Chip select 2 is active */

/*
 * PMP status register (slave only).
 */
#define PIC32_PMSTAT_OB0E	0x0001 /* Output buffer 0 empty */
#define PIC32_PMSTAT_OB1E	0x0002 /* Output buffer 1 empty */
#define PIC32_PMSTAT_OB2E	0x0004 /* Output buffer 2 empty */
#define PIC32_PMSTAT_OB3E	0x0008 /* Output buffer 3 empty */
#define PIC32_PMSTAT_OBUF	0x0040 /* Output buffer underflow */
#define PIC32_PMSTAT_OBE	0x0080 /* Output buffer empty */
#define PIC32_PMSTAT_IB0F	0x0100 /* Input buffer 0 full */
#define PIC32_PMSTAT_IB1F	0x0200 /* Input buffer 1 full */
#define PIC32_PMSTAT_IB2F	0x0400 /* Input buffer 2 full */
#define PIC32_PMSTAT_IB3F	0x0800 /* Input buffer 3 full */
#define PIC32_PMSTAT_IBOV	0x4000 /* Input buffer overflow */
#define PIC32_PMSTAT_IBF	0x8000 /* Input buffer full */

/*--------------------------------------
 * USB registers.
 */
#define U1OTGIR		PIC32_R (0x85040) /* OTG interrupt flags */
#define U1OTGIE		PIC32_R (0x85050) /* OTG interrupt enable */
#define U1OTGSTAT	PIC32_R (0x85060) /* Comparator and pin status */
#define U1OTGCON	PIC32_R (0x85070) /* Resistor and pin control */
#define U1PWRC		PIC32_R (0x85080) /* Power control */
#define U1IR		PIC32_R (0x85200) /* Pending interrupt */
#define U1IE		PIC32_R (0x85210) /* Interrupt enable */
#define U1EIR		PIC32_R (0x85220) /* Pending error interrupt */
#define U1EIE		PIC32_R (0x85230) /* Error interrupt enable */
#define U1STAT		PIC32_R (0x85240) /* Status FIFO */
#define U1CON		PIC32_R (0x85250) /* Control */
#define U1ADDR		PIC32_R (0x85260) /* Address */
#define U1BDTP1		PIC32_R	(0x85270) /* Buffer descriptor table pointer 1 */
#define U1FRML		PIC32_R (0x85280) /* Frame counter low */
#define U1FRMH		PIC32_R (0x85290) /* Frame counter high */
#define U1TOK		PIC32_R (0x852A0) /* Host control */
#define U1SOF		PIC32_R (0x852B0) /* SOF counter */
#define U1BDTP2		PIC32_R (0x852C0) /* Buffer descriptor table pointer 2 */
#define U1BDTP3		PIC32_R (0x852D0) /* Buffer descriptor table pointer 3 */
#define U1CNFG1		PIC32_R (0x852E0) /* Debug and idle */
#define U1EP(n)		PIC32_R (0x85300 + (n << 4)) /* Endpoint control */

/*
 * USB Control register.
 */
#define PIC32_U1CON_USBEN	0x0001 /* USB module enable (device mode) */
#define PIC32_U1CON_SOFEN	0x0001 /* SOF sent every 1 ms (host mode) */
#define PIC32_U1CON_PPBRST	0x0002 /* Ping-pong buffers reset */
#define PIC32_U1CON_RESUME	0x0004 /* Resume signaling enable */
#define PIC32_U1CON_HOSTEN	0x0008 /* Host mode enable */
#define PIC32_U1CON_USBRST	0x0010 /* USB reset */
#define PIC32_U1CON_PKTDIS	0x0020 /* Packet transfer disable */
#define PIC32_U1CON_TOKBUSY	0x0020 /* Token busy indicator */
#define PIC32_U1CON_SE0		0x0040 /* Single ended zero detected */
#define PIC32_U1CON_JSTATE	0x0080 /* Live differential receiver JSTATE flag */

/*
 * USB Power control register.
 */
#define PIC32_U1PWRC_USBPWR	0x0001 /* USB operation enable */
#define PIC32_U1PWRC_USUSPEND	0x0002 /* USB suspend mode */
#define PIC32_U1PWRC_USLPGRD	0x0010 /* USB sleep entry guard */
#define PIC32_U1PWRC_UACTPND	0x0080 /* UAB activity pending */

/*
 * USB Pending interrupt register.
 * USB Interrupt enable register.
 */
#define PIC32_U1I_DETACH	0x0001 /* Detach (host mode) */
#define PIC32_U1I_URST		0x0001 /* USB reset (device mode) */
#define PIC32_U1I_UERR		0x0002 /* USB error condition */
#define PIC32_U1I_SOF		0x0004 /* SOF token  */
#define PIC32_U1I_TRN		0x0008 /* Token processing complete */
#define PIC32_U1I_IDLE		0x0010 /* Idle detect */
#define PIC32_U1I_RESUME	0x0020 /* Resume */
#define PIC32_U1I_ATTACH	0x0040 /* Peripheral attach */
#define PIC32_U1I_STALL		0x0080 /* STALL handshake */

/*
 * USB OTG interrupt flags register.
 * USB OTG interrupt enable register.
 */
#define PIC32_U1OTGI_VBUSVD	0x0001 /* A-device Vbus change */
#define PIC32_U1OTGI_SESEND	0x0004 /* B-device Vbus change */
#define PIC32_U1OTGI_SESVD	0x0008 /* Session valid change */
#define PIC32_U1OTGI_ACTV	0x0010 /* Bus activity indicator */
#define PIC32_U1OTGI_LSTATE	0x0020 /* Line state stable */
#define PIC32_U1OTGI_T1MSEC	0x0040 /* 1 millisecond timer expired */
#define PIC32_U1OTGI_ID		0x0080 /* ID state change */

#define PIC32_U1OTGSTAT_VBUSVD	0x0001 /*  */
#define PIC32_U1OTGSTAT_SESEND	0x0004 /*  */
#define PIC32_U1OTGSTAT_SESVD	0x0008 /*  */
#define PIC32_U1OTGSTAT_LSTATE	0x0020 /*  */
#define PIC32_U1OTGSTAT_ID	0x0080 /*  */

#define PIC32_U1OTGCON_VBUSDIS	0x0001 /*  */
#define PIC32_U1OTGCON_VBUSCHG	0x0002 /*  */
#define PIC32_U1OTGCON_OTGEN	0x0004 /*  */
#define PIC32_U1OTGCON_VBUSON	0x0008 /*  */
#define PIC32_U1OTGCON_DMPULDWN	0x0010 /*  */
#define PIC32_U1OTGCON_DPPULDWN	0x0020 /*  */
#define PIC32_U1OTGCON_DMPULUP	0x0040 /*  */
#define PIC32_U1OTGCON_DPPULUP	0x0080 /*  */

#define PIC32_U1EI_PID		0x0001 /*  */
#define PIC32_U1EI_CRC5		0x0002 /*  */
#define PIC32_U1EI_EOF		0x0002 /*  */
#define PIC32_U1EI_CRC16	0x0004 /*  */
#define PIC32_U1EI_DFN8		0x0008 /*  */
#define PIC32_U1EI_BTO		0x0010 /*  */
#define PIC32_U1EI_DMA		0x0020 /*  */
#define PIC32_U1EI_BMX		0x0040 /*  */
#define PIC32_U1EI_BTS		0x0080 /*  */

#define PIC32_U1STAT_PPBI	0x0004 /*  */
#define PIC32_U1STAT_DIR	0x0008 /*  */
#define PIC32_U1STAT_ENDPT(x)	(((x) >> 4) & 0xF) /*  */

#define PIC32_U1ADDR_DEVADDR	0x007F /*  */
#define PIC32_U1ADDR_USBADDR0	0x0001 /*  */
#define PIC32_U1ADDR_DEVADDR1	0x0002 /*  */
#define PIC32_U1ADDR_DEVADDR2	0x0004 /*  */
#define PIC32_U1ADDR_DEVADDR3	0x0008 /*  */
#define PIC32_U1ADDR_DEVADDR4	0x0010 /*  */
#define PIC32_U1ADDR_DEVADDR5	0x0020 /*  */
#define PIC32_U1ADDR_DEVADDR6	0x0040 /*  */
#define PIC32_U1ADDR_LSPDEN	0x0080 /*  */

#define PIC32_U1FRML_FRM0	0x0001 /*  */
#define PIC32_U1FRML_FRM1	0x0002 /*  */
#define PIC32_U1FRML_FRM2	0x0004 /*  */
#define PIC32_U1FRML_FRM3	0x0008 /*  */
#define PIC32_U1FRML_FRM4	0x0010 /*  */
#define PIC32_U1FRML_FRM5	0x0020 /*  */
#define PIC32_U1FRML_FRM6	0x0040 /*  */
#define PIC32_U1FRML_FRM7	0x0080 /*  */

#define PIC32_U1FRMH_FRM8	0x0001 /*  */
#define PIC32_U1FRMH_FRM9	0x0002 /*  */
#define PIC32_U1FRMH_FRM10	0x0004 /*  */

#define PIC32_U1TOK_EP0		0x0001 /*  */
#define PIC32_U1TOK_EP		0x000F /*  */
#define PIC32_U1TOK_EP1		0x0002 /*  */
#define PIC32_U1TOK_EP2		0x0004 /*  */
#define PIC32_U1TOK_EP3		0x0008 /*  */
#define PIC32_U1TOK_PID0	0x0010 /*  */
#define PIC32_U1TOK_PID		0x00F0 /*  */
#define PIC32_U1TOK_PID1	0x0020 /*  */
#define PIC32_U1TOK_PID2	0x0040 /*  */
#define PIC32_U1TOK_PID3	0x0080 /*  */

#define PIC32_U1CNFG1_USBSIDL	0x0010 /*  */
#define PIC32_U1CNFG1_USBFRZ	0x0020 /*  */
#define PIC32_U1CNFG1_UOEMON	0x0040 /*  */
#define PIC32_U1CNFG1_UTEYE	0x0080 /*  */

#define PIC32_U1EP_EPHSHK	0x0001 /*  */
#define PIC32_U1EP_EPSTALL	0x0002 /*  */
#define PIC32_U1EP_EPTXEN	0x0004 /*  */
#define PIC32_U1EP_EPRXEN	0x0008 /*  */
#define PIC32_U1EP_EPCONDIS	0x0010 /*  */
#define PIC32_U1EP_RETRYDIS	0x0040 /*  */
#define PIC32_U1EP_LSPD		0x0080 /*  */

/* DB status field values */
#define PIC32_DB_BSTALL		(1 << 2)
#define PIC32_DB_DTS		(1 << 3)
#define PIC32_DB_NINC		(1 << 4)
#define PIC32_DB_KEEP		(1 << 5)
#define PIC32_DB_DATA1		(1 << 6)
#define PIC32_DB_UOWN		(1 << 7)
#define PIC32_DB_GET_PID(x)	(((x) >> 2) & 0xF)
#define PIC32_DB_SET_PID(x)	(((x) & 0xF) << 2)
#define PIC32_DB_GET_COUNT(x)	(((x) >> 16) & 0x3FF)
#define PIC32_DB_SET_COUNT(x)	(((x) & 0x3FF) << 16)

/*--------------------------------------
 * SPI registers.
 */
#ifdef PIC32MX4
#define SPI1CON		PIC32_R (0x5800) /* Control */
#define SPI1CONCLR	PIC32_R (0x5804)
#define SPI1CONSET	PIC32_R (0x5808)
#define SPI1CONINV	PIC32_R (0x580C)
#define SPI1STAT	PIC32_R (0x5810) /* Status */
#define SPI1STATCLR	PIC32_R (0x5814)
#define SPI1STATSET	PIC32_R (0x5818)
#define SPI1STATINV	PIC32_R (0x581C)
#define SPI1BUF		PIC32_R (0x5820) /* Transmit and receive buffer */
#define SPI1BRG		PIC32_R (0x5830) /* Baud rate generator */
#define SPI1BRGCLR	PIC32_R (0x5834)
#define SPI1BRGSET	PIC32_R (0x5838)
#define SPI1BRGINV	PIC32_R (0x583C)
#endif
#ifdef PIC32MX7
#define SPI3CON		PIC32_R (0x5800) /* Control */
#define SPI3CONCLR	PIC32_R (0x5804)
#define SPI3CONSET	PIC32_R (0x5808)
#define SPI3CONINV	PIC32_R (0x580C)
#define SPI3STAT	PIC32_R (0x5810) /* Status */
#define SPI3STATCLR	PIC32_R (0x5814)
#define SPI3STATSET	PIC32_R (0x5818)
#define SPI3STATINV	PIC32_R (0x581C)
#define SPI3BUF		PIC32_R (0x5820) /* Transmit and receive buffer */
#define SPI3BRG		PIC32_R (0x5830) /* Baud rate generator */
#define SPI3BRGCLR	PIC32_R (0x5834)
#define SPI3BRGSET	PIC32_R (0x5838)
#define SPI3BRGINV	PIC32_R (0x583C)

#define SPI4CON		PIC32_R (0x5C00) /* Control */
#define SPI4CONCLR	PIC32_R (0x5C04)
#define SPI4CONSET	PIC32_R (0x5C08)
#define SPI4CONINV	PIC32_R (0x5C0C)
#define SPI4STAT	PIC32_R (0x5C10) /* Status */
#define SPI4STATCLR	PIC32_R (0x5C14)
#define SPI4STATSET	PIC32_R (0x5C18)
#define SPI4STATINV	PIC32_R (0x5C1C)
#define SPI4BUF		PIC32_R (0x5C20) /* Transmit and receive buffer */
#define SPI4BRG		PIC32_R (0x5C30) /* Baud rate generator */
#define SPI4BRGCLR	PIC32_R (0x5C34)
#define SPI4BRGSET	PIC32_R (0x5C38)
#define SPI4BRGINV	PIC32_R (0x5C3C)

#define SPI1CON		PIC32_R (0x5E00) /* Control */
#define SPI1CONCLR	PIC32_R (0x5E04)
#define SPI1CONSET	PIC32_R (0x5E08)
#define SPI1CONINV	PIC32_R (0x5E0C)
#define SPI1STAT	PIC32_R (0x5E10) /* Status */
#define SPI1STATCLR	PIC32_R (0x5E14)
#define SPI1STATSET	PIC32_R (0x5E18)
#define SPI1STATINV	PIC32_R (0x5E1C)
#define SPI1BUF		PIC32_R (0x5E20) /* Transmit and receive buffer */
#define SPI1BRG		PIC32_R (0x5E30) /* Baud rate generator */
#define SPI1BRGCLR	PIC32_R (0x5E34)
#define SPI1BRGSET	PIC32_R (0x5E38)
#define SPI1BRGINV	PIC32_R (0x5E3C)
#endif

#define SPI2CON		PIC32_R (0x5A00) /* Control */
#define SPI2CONCLR	PIC32_R (0x5A04)
#define SPI2CONSET	PIC32_R (0x5A08)
#define SPI2CONINV	PIC32_R (0x5A0C)
#define SPI2STAT	PIC32_R (0x5A10) /* Status */
#define SPI2STATCLR	PIC32_R (0x5A14)
#define SPI2STATSET	PIC32_R (0x5A18)
#define SPI2STATINV	PIC32_R (0x5A1C)
#define SPI2BUF		PIC32_R (0x5A20) /* Transmit and receive buffer */
#define SPI2BRG		PIC32_R (0x5A30) /* Baud rate generator */
#define SPI2BRGCLR	PIC32_R (0x5A34)
#define SPI2BRGSET	PIC32_R (0x5A38)
#define SPI2BRGINV	PIC32_R (0x5A3C)

/*
 * SPI Control register.
 */
#define PIC32_SPICON_MSTEN	0x00000020	/* Master mode */
#define PIC32_SPICON_CKP	0x00000040      /* Idle clock is high level */
#define PIC32_SPICON_SSEN	0x00000080      /* Slave mode: SSx pin enable */
#define PIC32_SPICON_CKE	0x00000100      /* Output data changes on
						 * transition from active clock
						 * state to Idle clock state */
#define PIC32_SPICON_SMP	0x00000200      /* Master mode: input data sampled
						 * at end of data output time. */
#define PIC32_SPICON_MODE16	0x00000400      /* 16-bit data width */
#define PIC32_SPICON_MODE32	0x00000800      /* 32-bit data width */
#define PIC32_SPICON_DISSDO	0x00001000      /* SDOx pin is not used */
#define PIC32_SPICON_SIDL	0x00002000      /* Stop in Idle mode */
#define PIC32_SPICON_FRZ	0x00004000      /* Freeze in Debug mode */
#define PIC32_SPICON_ON		0x00008000      /* SPI Peripheral is enabled */
#define PIC32_SPICON_ENHBUF	0x00010000      /* Enhanced buffer enable */
#define PIC32_SPICON_SPIFE	0x00020000      /* Frame synchronization pulse
						 * coincides with the first bit clock */
#define PIC32_SPICON_FRMPOL	0x20000000      /* Frame pulse is active-high */
#define PIC32_SPICON_FRMSYNC	0x40000000      /* Frame sync pulse input (Slave mode) */
#define PIC32_SPICON_FRMEN	0x80000000      /* Framed SPI support */

/*
 * SPI Status register.
 */
#define PIC32_SPISTAT_SPIRBF	0x00000001      /* Receive buffer is full */
#define PIC32_SPISTAT_SPITBF	0x00000002      /* Transmit buffer is full */
#define PIC32_SPISTAT_SPITBE	0x00000008      /* Transmit buffer is empty */
#define PIC32_SPISTAT_SPIRBE    0x00000020      /* Receive buffer is empty */
#define PIC32_SPISTAT_SPIROV	0x00000040      /* Receive overflow flag */
#define PIC32_SPISTAT_SPIBUSY	0x00000800      /* SPI is busy */

/*--------------------------------------
 * DMA controller registers.
 */
#define DMACON          PIC32_R (0x83000)       /* DMA Control */
#define DMACONCLR	PIC32_R (0x83004)
#define DMACONSET	PIC32_R (0x83008)
#define DMACONINV	PIC32_R (0x8300C)
#define DMASTAT         PIC32_R (0x83010)       /* DMA Status */
#define DMAADDR         PIC32_R (0x83020)       /* DMA Address */
// TODO: other DMA registers.

/*--------------------------------------
 * System controller registers.
 */
#define OSCCON          PIC32_R (0xf000)
#define OSCTUN          PIC32_R (0xf010)
#define DDPCON          PIC32_R (0xf200)        /* Debug Data Port Control */
#define DEVID           PIC32_R (0xf220)
#define SYSKEY          PIC32_R (0xf230)
#define RCON            PIC32_R (0xf600)
#define RCONCLR         PIC32_R (0xf604)
#define RCONSET         PIC32_R (0xf608)
#define RCONINV         PIC32_R (0xf60C)
#define RSWRST          PIC32_R (0xf610)
#define RSWRSTCLR       PIC32_R (0xf614)
#define RSWRSTSET       PIC32_R (0xf618)
#define RSWRSTINV       PIC32_R (0xf61C)

/*
 * Reset control register.
 */
#define PIC32_RCON_POR          0x00000001
#define PIC32_RCON_BOR          0x00000002
#define PIC32_RCON_IDLE         0x00000004
#define PIC32_RCON_SLEEP        0x00000008
#define PIC32_RCON_WDTO         0x00000010
#define PIC32_RCON_SWR          0x00000040
#define PIC32_RCON_EXTR         0x00000080
#define PIC32_RCON_VREGS        0x00000100
#define PIC32_RCON_CMR          0x00000200

/*--------------------------------------
 * Prefetch cache controller registers.
 */
#define CHECON          PIC32_R (0x84000)       /* Prefetch cache control */
#define CHECONCLR	PIC32_R (0x84004)
#define CHECONSET	PIC32_R (0x84008)
#define CHECONINV	PIC32_R (0x8400C)
// TODO: other prefetech registers

/*--------------------------------------
 * Bus matrix control registers.
 */
#define BMXCON          PIC32_R (0x82000)       /* Memory configuration */
#define BMXCONCLR	PIC32_R (0x82004)
#define BMXCONSET	PIC32_R (0x82008)
#define BMXCONINV	PIC32_R (0x8200C)
#define BMXDKPBA        PIC32_R (0x82010)       /* Data RAM kernel program base address */
#define BMXDUDBA        PIC32_R (0x82020)       /* Data RAM user data base address */
#define BMXDUPBA        PIC32_R (0x82030)       /* Data RAM user program base address */
#define BMXDRMSZ        PIC32_R (0x82040)       /* Data RAM size */
#define BMXPUPBA        PIC32_R (0x82050)       /* Program Flash user program base address */
#define BMXPFMSZ        PIC32_R (0x82060)       /* Program Flash size */
#define BMXBOOTSZ       PIC32_R (0x82070)       /* Boot Flash size */

/*--------------------------------------
 * Non-volatile memory control registers.
 */
#define NVMCON          PIC32_R (0x0F400)
#define NVMCONCLR       PIC32_R (0x0F404)
#define NVMCONSET       PIC32_R (0x0F408)
#define NVMCONINV       PIC32_R (0x0F40C)
#define NVMKEY          PIC32_R (0x0F410)
#define NVMADDR         PIC32_R (0x0F420)
#define NVMADDRCLR      PIC32_R (0x0F424)
#define NVMADDRSET      PIC32_R (0x0F428)
#define NVMADDRINV      PIC32_R (0x0F42C)
#define NVMDATA         PIC32_R (0x0F430)
#define NVMSRCADDR      PIC32_R (0x0F440)

#define PIC32_NVMCON_NVMOP      0x0000000F
#define PIC32_NVMCON_NOP                 0 /* No operation */
#define PIC32_NVMCON_WORD_PGM            1 /* Word program */
#define PIC32_NVMCON_ROW_PGM             3 /* Row program */
#define PIC32_NVMCON_PAGE_ERASE          4 /* Page erase */

#define PIC32_NVMCON_LVDSTAT    0x00000800
#define PIC32_NVMCON_LVDERR     0x00001000
#define PIC32_NVMCON_WRERR      0x00002000
#define PIC32_NVMCON_WREN       0x00004000
#define PIC32_NVMCON_WR         0x00008000


/*
 * Timer2 registers
 */
#define T2CON 		PIC32_R (0x0800)
#define T2CONSET 	PIC32_R (0x0808)
#define TMR2  		PIC32_R (0x0810)
#define PR2   		PIC32_R (0x0820)

/*
 * Output compare registers
 */
#define OC1CON		PIC32_R (0x3000)
#define OC1R		PIC32_R (0x3010)
#define OC1RS		PIC32_R (0x3020)
#define OC4CON   	PIC32_R (0x3600)
#define OC4R		PIC32_R (0x3610)
#define OC4RS		PIC32_R (0x3620)

#define BLRKEY      *(volatile unsigned*)(0x80000000)

/*--------------------------------------
 * Configuration registers.
 */
#define DEVCFG0		*(volatile unsigned*)0x9fc02ffc
#define DEVCFG1		*(volatile unsigned*)0x9fc02ff8
#define DEVCFG2		*(volatile unsigned*)0x9fc02ff4
#define DEVCFG3		*(volatile unsigned*)0x9fc02ff0

#define PIC32_DEVCFG(cfg0, cfg1, cfg2, cfg3) \
    asm (".section .config"); \
    unsigned __DEVCFG0 __attribute__ ((section (".config0"))) = (cfg0) ^ 0x7fffffff; \
    unsigned __DEVCFG1 __attribute__ ((section (".config1"))) = (cfg1) | DEVCFG1_UNUSED; \
    unsigned __DEVCFG2 __attribute__ ((section (".config2"))) = (cfg2) | DEVCFG2_UNUSED; \
    unsigned __DEVCFG3 __attribute__ ((section (".config3"))) = (cfg3) | DEVCFG3_UNUSED

/*
 * Config0 register at 1fc02ffc, inverted.
 */
#define DEVCFG0_DEBUG_MASK      0x00000003 /* Debugger enable bits */
#define DEVCFG0_DEBUG_DISABLED  0x00000000 /* Debugger disabled */
#define DEVCFG0_DEBUG_ENABLED   0x00000002 /* Debugger enabled */
#define DEVCFG0_ICESEL          0x00000008 /* Use PGC1/PGD1 (default PGC2/PGD2) */
#define DEVCFG0_PWP_MASK        0x000ff000 /* Program flash write protect */
#define DEVCFG0_BWP             0x01000000 /* Boot flash write protect */
#define DEVCFG0_CP              0x10000000 /* Code protect */

/*
 * Config1 register at 1fc02ff8.
 */
#define DEVCFG1_UNUSED          0xff600858
#define DEVCFG1_FNOSC_MASK      0x00000007 /* Oscillator selection */
#define DEVCFG1_FNOSC_FRC       0x00000000 /* Fast RC */
#define DEVCFG1_FNOSC_FRCDIVPLL 0x00000001 /* Fast RC with divide-by-N and PLL */
#define DEVCFG1_FNOSC_PRI       0x00000002 /* Primary oscillator XT, HS, EC */
#define DEVCFG1_FNOSC_PRIPLL    0x00000003 /* Primary with PLL */
#define DEVCFG1_FNOSC_SEC       0x00000004 /* Secondary oscillator */
#define DEVCFG1_FNOSC_LPRC      0x00000005 /* Low-power RC */
#define DEVCFG1_FNOSC_FRCDIV    0x00000007 /* Fast RC with divide-by-N */
#define DEVCFG1_FSOSCEN         0x00000020 /* Secondary oscillator enable */
#define DEVCFG1_IESO            0x00000080 /* Internal-external switch over */
#define DEVCFG1_POSCMOD_MASK    0x00000300 /* Primary oscillator config */
#define DEVCFG1_POSCMOD_EXT     0x00000000 /* External mode */
#define DEVCFG1_POSCMOD_XT      0x00000100 /* XT oscillator */
#define DEVCFG1_POSCMOD_HS      0x00000200 /* HS oscillator */
#define DEVCFG1_POSCMOD_DISABLE 0x00000300 /* Disabled */
#define DEVCFG1_OSCIOFNC        0x00000400 /* CLKO output active */
#define DEVCFG1_FPBDIV_MASK     0x00003000 /* Peripheral bus clock divisor */
#define DEVCFG1_FPBDIV_1        0x00000000 /* SYSCLK / 1 */
#define DEVCFG1_FPBDIV_2        0x00001000 /* SYSCLK / 2 */
#define DEVCFG1_FPBDIV_4        0x00002000 /* SYSCLK / 4 */
#define DEVCFG1_FPBDIV_8        0x00003000 /* SYSCLK / 8 */
#define DEVCFG1_FCKM_DISABLE    0x00004000 /* Fail-safe clock monitor disable */
#define DEVCFG1_FCKS_DISABLE    0x00008000 /* Clock switching disable */
#define DEVCFG1_WDTPS_MASK      0x001f0000 /* Watchdog postscale */
#define DEVCFG1_WDTPS_1         0x00000000 /* 1:1 */
#define DEVCFG1_WDTPS_2         0x00010000 /* 1:2 */
#define DEVCFG1_WDTPS_4         0x00020000 /* 1:4 */
#define DEVCFG1_WDTPS_8         0x00030000 /* 1:8 */
#define DEVCFG1_WDTPS_16        0x00040000 /* 1:16 */
#define DEVCFG1_WDTPS_32        0x00050000 /* 1:32 */
#define DEVCFG1_WDTPS_64        0x00060000 /* 1:64 */
#define DEVCFG1_WDTPS_128       0x00070000 /* 1:128 */
#define DEVCFG1_WDTPS_256       0x00080000 /* 1:256 */
#define DEVCFG1_WDTPS_512       0x00090000 /* 1:512 */
#define DEVCFG1_WDTPS_1024      0x000a0000 /* 1:1024 */
#define DEVCFG1_WDTPS_2048      0x000b0000 /* 1:2048 */
#define DEVCFG1_WDTPS_4096      0x000c0000 /* 1:4096 */
#define DEVCFG1_WDTPS_8192      0x000d0000 /* 1:8192 */
#define DEVCFG1_WDTPS_16384     0x000e0000 /* 1:16384 */
#define DEVCFG1_WDTPS_32768     0x000f0000 /* 1:32768 */
#define DEVCFG1_WDTPS_65536     0x00100000 /* 1:65536 */
#define DEVCFG1_WDTPS_131072    0x00110000 /* 1:131072 */
#define DEVCFG1_WDTPS_262144    0x00120000 /* 1:262144 */
#define DEVCFG1_WDTPS_524288    0x00130000 /* 1:524288 */
#define DEVCFG1_WDTPS_1048576   0x00140000 /* 1:1048576 */
#define DEVCFG1_FWDTEN          0x00800000 /* Watchdog enable */

/*
 * Config2 register at 1fc02ff4.
 */
#define DEVCFG2_UNUSED          0xfff87888
#define DEVCFG2_FPLLIDIV_MASK   0x00000007 /* PLL input divider */
#define DEVCFG2_FPLLIDIV_1      0x00000000 /* 1x */
#define DEVCFG2_FPLLIDIV_2      0x00000001 /* 2x */
#define DEVCFG2_FPLLIDIV_3      0x00000002 /* 3x */
#define DEVCFG2_FPLLIDIV_4      0x00000003 /* 4x */
#define DEVCFG2_FPLLIDIV_5      0x00000004 /* 5x */
#define DEVCFG2_FPLLIDIV_6      0x00000005 /* 6x */
#define DEVCFG2_FPLLIDIV_10     0x00000006 /* 10x */
#define DEVCFG2_FPLLIDIV_12     0x00000007 /* 12x */
#define DEVCFG2_FPLLMUL_MASK    0x00000070 /* PLL multiplier */
#define DEVCFG2_FPLLMUL_15      0x00000000 /* 15x */
#define DEVCFG2_FPLLMUL_16      0x00000010 /* 16x */
#define DEVCFG2_FPLLMUL_17      0x00000020 /* 17x */
#define DEVCFG2_FPLLMUL_18      0x00000030 /* 18x */
#define DEVCFG2_FPLLMUL_19      0x00000040 /* 19x */
#define DEVCFG2_FPLLMUL_20      0x00000050 /* 20x */
#define DEVCFG2_FPLLMUL_21      0x00000060 /* 21x */
#define DEVCFG2_FPLLMUL_24      0x00000070 /* 24x */
#define DEVCFG2_UPLLIDIV_MASK   0x00000700 /* USB PLL input divider */
#define DEVCFG2_UPLLIDIV_1      0x00000000 /* 1x */
#define DEVCFG2_UPLLIDIV_2      0x00000100 /* 2x */
#define DEVCFG2_UPLLIDIV_3      0x00000200 /* 3x */
#define DEVCFG2_UPLLIDIV_4      0x00000300 /* 4x */
#define DEVCFG2_UPLLIDIV_5      0x00000400 /* 5x */
#define DEVCFG2_UPLLIDIV_6      0x00000500 /* 6x */
#define DEVCFG2_UPLLIDIV_10     0x00000600 /* 10x */
#define DEVCFG2_UPLLIDIV_12     0x00000700 /* 12x */
#define DEVCFG2_UPLLDIS         0x00008000 /* Disable USB PLL */
#define DEVCFG2_FPLLODIV_MASK   0x00070000 /* Default postscaler for PLL */
#define DEVCFG2_FPLLODIV_1      0x00000000 /* 1x */
#define DEVCFG2_FPLLODIV_2      0x00010000 /* 2x */
#define DEVCFG2_FPLLODIV_4      0x00020000 /* 4x */
#define DEVCFG2_FPLLODIV_8      0x00030000 /* 8x */
#define DEVCFG2_FPLLODIV_16     0x00040000 /* 16x */
#define DEVCFG2_FPLLODIV_32     0x00050000 /* 32x */
#define DEVCFG2_FPLLODIV_64     0x00060000 /* 64x */
#define DEVCFG2_FPLLODIV_256    0x00070000 /* 256x */

/*
 * Config3 register at 1fc02ff0.
 */
#define DEVCFG3_UNUSED          0x38f80000
#define DEVCFG3_USERID_MASK     0x0000ffff /* User-defined ID */
#define DEVCFG3_USERID(x)       ((x) & 0xffff)
#define DEVCFG3_FSRSSEL_MASK    0x00070000 /* SRS select */
#define DEVCFG3_FSRSSEL_ALL     0x00000000 /* All irqs assigned to shadow set */
#define DEVCFG3_FSRSSEL_1       0x00010000 /* Assign irq priority 1 to shadow set */
#define DEVCFG3_FSRSSEL_2       0x00020000 /* Assign irq priority 2 to shadow set */
#define DEVCFG3_FSRSSEL_3       0x00030000 /* Assign irq priority 3 to shadow set */
#define DEVCFG3_FSRSSEL_4       0x00040000 /* Assign irq priority 4 to shadow set */
#define DEVCFG3_FSRSSEL_5       0x00050000 /* Assign irq priority 5 to shadow set */
#define DEVCFG3_FSRSSEL_6       0x00060000 /* Assign irq priority 6 to shadow set */
#define DEVCFG3_FSRSSEL_7       0x00070000 /* Assign irq priority 7 to shadow set */
#define DEVCFG3_FMIIEN          0x01000000 /* Ethernet MII enable */
#define DEVCFG3_FETHIO          0x02000000 /* Ethernet pins default */
#define DEVCFG3_FCANIO          0x04000000 /* CAN pins default */
#define DEVCFG3_FUSBIDIO        0x40000000 /* USBID pin: controlled by USB */
#define DEVCFG3_FVBUSONIO       0x80000000 /* VBuson pin: controlled by USB */

/*--------------------------------------
 * Interrupt controller registers.
 */
#define INTCON		PIC32_R (0x81000)	/* Interrupt Control */
#define INTCONCLR	PIC32_R (0x81004)
#define INTCONSET	PIC32_R (0x81008)
#define INTCONINV	PIC32_R (0x8100C)
#define INTSTAT		PIC32_R (0x81010)	/* Interrupt Status */
#define IPTMR		PIC32_R (0x81020)	/* Temporal Proximity Timer */
#define IPTMRCLR	PIC32_R (0x81024)
#define IPTMRSET	PIC32_R (0x81028)
#define IPTMRINV	PIC32_R (0x8102C)
#define IFS(n)		PIC32_R (0x81030+((n)<<4)) /* IFS(0..2) - Interrupt Flag Status */
#define IFSCLR(n)	PIC32_R (0x81034+((n)<<4))
#define IFSSET(n)	PIC32_R (0x81038+((n)<<4))
#define IFSINV(n)	PIC32_R (0x8103C+((n)<<4))
#define IEC(n)		PIC32_R (0x81060+((n)<<4)) /* IEC(0..2) - Interrupt Enable Control */
#define IECCLR(n)	PIC32_R (0x81064+((n)<<4))
#define IECSET(n)	PIC32_R (0x81068+((n)<<4))
#define IECINV(n)	PIC32_R (0x8106C+((n)<<4))
#define IPC(n)		PIC32_R (0x81090+((n)<<4)) /* IPC(0..12) - Interrupt Priority Control */
#define IPCCLR(n)	PIC32_R (0x81094+((n)<<4))
#define IPCSET(n)	PIC32_R (0x81098+((n)<<4))
#define IPCINV(n)	PIC32_R (0x8109C+((n)<<4))

/*
 * Interrupt Control register.
 */
#define PIC32_INTCON_INT0EP	0x0001	/* External interrupt 0 polarity rising edge */
#define PIC32_INTCON_INT1EP	0x0002	/* External interrupt 1 polarity rising edge */
#define PIC32_INTCON_INT2EP	0x0004	/* External interrupt 2 polarity rising edge */
#define PIC32_INTCON_INT3EP	0x0008	/* External interrupt 3 polarity rising edge */
#define PIC32_INTCON_INT4EP	0x0010	/* External interrupt 4 polarity rising edge */
#define PIC32_INTCON_TPC(x)	((x)<<8) /* Temporal proximity group priority */
#define PIC32_INTCON_MVEC	0x1000	/* Multi-vectored mode */
#define PIC32_INTCON_FRZ	0x4000	/* Freeze in debug mode */
#define PIC32_INTCON_SS0	0x8000	/* Single vector has a shadow register set */

/*
 * Interrupt Status register.
 */
#define PIC32_INTSTAT_VEC(s)	((s) & 0xFF)	/* Interrupt vector */
#define PIC32_INTSTAT_SRIPL(s)	((s) >> 8 & 7)	/* Requested priority level */
#define PIC32_INTSTAT_SRIPL_MASK 0x0700

/*
 * Interrupt Prority Control register.
 */
#define PIC32_IPC_IS0(x)	(x)		/* Interrupt 0 subpriority */
#define PIC32_IPC_IP0(x)	((x)<<2)	/* Interrupt 0 priority */
#define PIC32_IPC_IS1(x)	((x)<<8)	/* Interrupt 1 subpriority */
#define PIC32_IPC_IP1(x)	((x)<<10)	/* Interrupt 1 priority */
#define PIC32_IPC_IS2(x)	((x)<<16)	/* Interrupt 2 subpriority */
#define PIC32_IPC_IP2(x)	((x)<<18)	/* Interrupt 2 priority */
#define PIC32_IPC_IS3(x)	((x)<<24)	/* Interrupt 3 subpriority */
#define PIC32_IPC_IP3(x)	((x)<<26)	/* Interrupt 3 priority */

/*
 * IRQ numbers for PIC32MX3xx/4xx/5xx/6xx/7xx
 */
#define PIC32_IRQ_CT        0   /* Core Timer Interrupt */
#define PIC32_IRQ_CS0       1   /* Core Software Interrupt 0 */
#define PIC32_IRQ_CS1       2   /* Core Software Interrupt 1 */
#define PIC32_IRQ_INT0      3   /* External Interrupt 0 */
#define PIC32_IRQ_T1        4   /* Timer1 */
#define PIC32_IRQ_IC1       5   /* Input Capture 1 */
#define PIC32_IRQ_OC1       6   /* Output Compare 1 */
#define PIC32_IRQ_INT1      7   /* External Interrupt 1 */
#define PIC32_IRQ_T2        8   /* Timer2 */
#define PIC32_IRQ_IC2       9   /* Input Capture 2 */
#define PIC32_IRQ_OC2       10  /* Output Compare 2 */
#define PIC32_IRQ_INT2      11  /* External Interrupt 2 */
#define PIC32_IRQ_T3        12  /* Timer3 */
#define PIC32_IRQ_IC3       13  /* Input Capture 3 */
#define PIC32_IRQ_OC3       14  /* Output Compare 3 */
#define PIC32_IRQ_INT3      15  /* External Interrupt 3 */
#define PIC32_IRQ_T4        16  /* Timer4 */
#define PIC32_IRQ_IC4       17  /* Input Capture 4 */
#define PIC32_IRQ_OC4       18  /* Output Compare 4 */
#define PIC32_IRQ_INT4      19  /* External Interrupt 4 */
#define PIC32_IRQ_T5        20  /* Timer5 */
#define PIC32_IRQ_IC5       21  /* Input Capture 5 */
#define PIC32_IRQ_OC5       22  /* Output Compare 5 */
#define PIC32_IRQ_SPI1E     23  /* SPI1 Fault */
#define PIC32_IRQ_SPI1TX    24  /* SPI1 Transfer Done */
#define PIC32_IRQ_SPI1RX    25  /* SPI1 Receive Done */

#define PIC32_IRQ_SPI3E     26  /* SPI3 Fault */
#define PIC32_IRQ_SPI3TX    27  /* SPI3 Transfer Done */
#define PIC32_IRQ_SPI3RX    28  /* SPI3 Receive Done */
#define PIC32_IRQ_U1E       26  /* UART1 Error */
#define PIC32_IRQ_U1RX      27  /* UART1 Receiver */
#define PIC32_IRQ_U1TX      28  /* UART1 Transmitter */
#define PIC32_IRQ_I2C3B     26  /* I2C3 Bus Collision Event */
#define PIC32_IRQ_I2C3S     27  /* I2C3 Slave Event */
#define PIC32_IRQ_I2C3M     28  /* I2C3 Master Event */

#define PIC32_IRQ_I2C1B     29  /* I2C1 Bus Collision Event */
#define PIC32_IRQ_I2C1S     30  /* I2C1 Slave Event */
#define PIC32_IRQ_I2C1M     31  /* I2C1 Master Event */
#define PIC32_IRQ_CN        32  /* Input Change Interrupt */
#define PIC32_IRQ_AD1       33  /* ADC1 Convert Done */
#define PIC32_IRQ_PMP       34  /* Parallel Master Port */
#define PIC32_IRQ_CMP1      35  /* Comparator Interrupt */
#define PIC32_IRQ_CMP2      36  /* Comparator Interrupt */

#define PIC32_IRQ_SPI2E     37  /* SPI2 Fault */
#define PIC32_IRQ_SPI2TX    38  /* SPI2 Transfer Done */
#define PIC32_IRQ_SPI2RX    39  /* SPI2 Receive Done */
#define PIC32_IRQ_U3E       37  /* UART3 Error */
#define PIC32_IRQ_U3RX      38  /* UART3 Receiver */
#define PIC32_IRQ_U3TX      39  /* UART3 Transmitter */
#define PIC32_IRQ_I2C4B     37  /* I2C4 Bus Collision Event */
#define PIC32_IRQ_I2C4S     38  /* I2C4 Slave Event */
#define PIC32_IRQ_I2C4M     39  /* I2C4 Master Event */

#define PIC32_IRQ_SPI4E     40  /* SPI4 Fault */
#define PIC32_IRQ_SPI4TX    41  /* SPI4 Transfer Done */
#define PIC32_IRQ_SPI4RX    42  /* SPI4 Receive Done */
#define PIC32_IRQ_U2E       40  /* UART2 Error */
#define PIC32_IRQ_U2RX      41  /* UART2 Receiver */
#define PIC32_IRQ_U2TX      42  /* UART2 Transmitter */
#define PIC32_IRQ_I2C5B     40  /* I2C5 Bus Collision Event */
#define PIC32_IRQ_I2C5S     41  /* I2C5 Slave Event */
#define PIC32_IRQ_I2C5M     42  /* I2C5 Master Event */

#define PIC32_IRQ_I2C2B     43  /* I2C2 Bus Collision Event */
#define PIC32_IRQ_I2C2S     44  /* I2C2 Slave Event */
#define PIC32_IRQ_I2C2M     45  /* I2C2 Master Event */
#define PIC32_IRQ_FSCM      46  /* Fail-Safe Clock Monitor */
#define PIC32_IRQ_RTCC      47  /* Real-Time Clock and Calendar */
#define PIC32_IRQ_DMA0      48  /* DMA Channel 0 */
#define PIC32_IRQ_DMA1      49  /* DMA Channel 1 */
#define PIC32_IRQ_DMA2      50  /* DMA Channel 2 */
#define PIC32_IRQ_DMA3      51  /* DMA Channel 3 */
#define PIC32_IRQ_DMA4      52  /* DMA Channel 4 */
#define PIC32_IRQ_DMA5      53  /* DMA Channel 5 */
#define PIC32_IRQ_DMA6      54  /* DMA Channel 6 */
#define PIC32_IRQ_DMA7      55  /* DMA Channel 7 */
#define PIC32_IRQ_FCE       56  /* Flash Control Event */
#define PIC32_IRQ_USB       57  /* USB */
#define PIC32_IRQ_CAN1      58  /* Control Area Network 1 */
#define PIC32_IRQ_CAN2      59  /* Control Area Network 2 */
#define PIC32_IRQ_ETH       60  /* Ethernet Interrupt */
#define PIC32_IRQ_IC1E      61  /* Input Capture 1 Error */
#define PIC32_IRQ_IC2E      62  /* Input Capture 2 Error */
#define PIC32_IRQ_IC3E      63  /* Input Capture 3 Error */
#define PIC32_IRQ_IC4E      64  /* Input Capture 4 Error */
#define PIC32_IRQ_IC5E      65  /* Input Capture 5 Error */
#define PIC32_IRQ_PMPE      66  /* Parallel Master Port Error */
#define PIC32_IRQ_U4E       67  /* UART4 Error */
#define PIC32_IRQ_U4RX      68  /* UART4 Receiver */
#define PIC32_IRQ_U4TX      69  /* UART4 Transmitter */
#define PIC32_IRQ_U6E       70  /* UART6 Error */
#define PIC32_IRQ_U6RX      71  /* UART6 Receiver */
#define PIC32_IRQ_U6TX      72  /* UART6 Transmitter */
#define PIC32_IRQ_U5E       73  /* UART5 Error */
#define PIC32_IRQ_U5RX      74  /* UART5 Receiver */
#define PIC32_IRQ_U5TX      75  /* UART5 Transmitter */

/*
 * Interrupt vector numbers for PIC32MX3xx/4xx/5xx/6xx/7xx
 */
#define PIC32_VECT_CT       0   /* Core Timer Interrupt */
#define PIC32_VECT_CS0      1   /* Core Software Interrupt 0 */
#define PIC32_VECT_CS1      2   /* Core Software Interrupt 1 */
#define PIC32_VECT_INT0     3   /* External Interrupt 0 */
#define PIC32_VECT_T1       4   /* Timer1 */
#define PIC32_VECT_IC1      5   /* Input Capture 1 */
#define PIC32_VECT_OC1      6   /* Output Compare 1 */
#define PIC32_VECT_INT1     7   /* External Interrupt 1 */
#define PIC32_VECT_T2       8   /* Timer2 */
#define PIC32_VECT_IC2      9   /* Input Capture 2 */
#define PIC32_VECT_OC2      10  /* Output Compare 2 */
#define PIC32_VECT_INT2     11  /* External Interrupt 2 */
#define PIC32_VECT_T3       12  /* Timer3 */
#define PIC32_VECT_IC3      13  /* Input Capture 3 */
#define PIC32_VECT_OC3      14  /* Output Compare 3 */
#define PIC32_VECT_INT3     15  /* External Interrupt 3 */
#define PIC32_VECT_T4       16  /* Timer4 */
#define PIC32_VECT_IC4      17  /* Input Capture 4 */
#define PIC32_VECT_OC4      18  /* Output Compare 4 */
#define PIC32_VECT_INT4     19  /* External Interrupt 4 */
#define PIC32_VECT_T5       20  /* Timer5 */
#define PIC32_VECT_IC5      21  /* Input Capture 5 */
#define PIC32_VECT_OC5      22  /* Output Compare 5 */
#define PIC32_VECT_SPI1     23  /* SPI1 */

#define PIC32_VECT_SPI3     24  /* SPI3 */
#define PIC32_VECT_U1       24  /* UART1 */
#define PIC32_VECT_I2C3     24  /* I2C3 */

#define PIC32_VECT_I2C1     25  /* I2C1 */
#define PIC32_VECT_CN       26  /* Input Change Interrupt */
#define PIC32_VECT_AD1      27  /* ADC1 Convert Done */
#define PIC32_VECT_PMP      28  /* Parallel Master Port */
#define PIC32_VECT_CMP1     29  /* Comparator Interrupt */
#define PIC32_VECT_CMP2     30  /* Comparator Interrupt */

#define PIC32_VECT_SPI2     31  /* SPI2 */
#define PIC32_VECT_U3       31  /* UART3 */
#define PIC32_VECT_I2C4     31  /* I2C4 */

#define PIC32_VECT_SPI4     32  /* SPI4 */
#define PIC32_VECT_U2       32  /* UART2 */
#define PIC32_VECT_I2C5     32  /* I2C5 */

#define PIC32_VECT_I2C2     33  /* I2C2 */
#define PIC32_VECT_FSCM     34  /* Fail-Safe Clock Monitor */
#define PIC32_VECT_RTCC     35  /* Real-Time Clock and Calendar */
#define PIC32_VECT_DMA0     36  /* DMA Channel 0 */
#define PIC32_VECT_DMA1     37  /* DMA Channel 1 */
#define PIC32_VECT_DMA2     38  /* DMA Channel 2 */
#define PIC32_VECT_DMA3     39  /* DMA Channel 3 */
#define PIC32_VECT_DMA4     40  /* DMA Channel 4 */
#define PIC32_VECT_DMA5     41  /* DMA Channel 5 */
#define PIC32_VECT_DMA6     42  /* DMA Channel 6 */
#define PIC32_VECT_DMA7     43  /* DMA Channel 7 */
#define PIC32_VECT_FCE      44  /* Flash Control Event */
#define PIC32_VECT_USB      45  /* USB */
#define PIC32_VECT_CAN1     46  /* Control Area Network 1 */
#define PIC32_VECT_CAN2     47  /* Control Area Network 2 */
#define PIC32_VECT_ETH      48  /* Ethernet Interrupt */
#define PIC32_VECT_U4       49  /* UART4 */
#define PIC32_VECT_U6       50  /* UART6 */
#define PIC32_VECT_U5       51  /* UART5 */

#endif /* _IO_PIC32MX_H */
