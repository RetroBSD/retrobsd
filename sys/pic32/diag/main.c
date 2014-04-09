#include "param.h"
#include "conf.h"
#include "systm.h"
#include "buf.h"
#include "reboot.h"

#define SECTSIZE	512     /* size of SD card sector */

size_t  physmem;                /* total amount of physical memory */
u_int   swapstart, nswap;       /* start and size of swap space */

const struct linesw linesw[] = { { 0 } };
int nldisp = 1;

char data [SECTSIZE * 2];

extern int card_size (int unit, unsigned *nsectors);
extern int card_read (int unit, unsigned offset, char *data, unsigned bcount);
extern int card_write (int unit, unsigned offset, char *data, unsigned bcount);

static void
fill_sector (p, byte0, byte1)
        char *p;
        unsigned byte0, byte1;
{
        unsigned i;

        for (i=0; i<SECTSIZE; i+=2) {
                *p++ = byte0;
                *p++ = byte1;
        }
}

static int
check_data (byte0, byte1)
        unsigned byte0, byte1;
{
        unsigned char *p = (unsigned char *) data;
        unsigned i;

        for (i=0; i<SECTSIZE; i+=2) {
                if (*p != byte0) {
                        printf ("Offset %u written %08X read %08X\n",
                                i, byte0, *p);
                        return 0;
                }
                p++;
                if (*p != byte1) {
                        printf ("Offset %u written %08X read %08X\n",
                                i, byte0, *p);
                        return 0;
                }
                p++;
        }
        return 1;
}

static void
test_sectors (first, last)
        unsigned first, last;
{
        unsigned i, r0, r1, w0, w1, kbytes;

        printf ("Testing sectors %u-%u\n", first, last);
        printf ("Write...");
        w0 = mips_read_c0_register (C0_COUNT, 0);
        for (i=first; i<last; i+=2) {
                fill_sector (data, 0x55^i, 0xaa^i);
                fill_sector (data+SECTSIZE, 0x55^(i+1), 0xaa^(i+1));
                if (! card_write (0, i*SECTSIZE, data, SECTSIZE*2)) {
                        printf ("Sector %u: write error.\n", i);
                        break;
                }
        }
        w1 = mips_read_c0_register (C0_COUNT, 0);
        printf (" done\n");
        printf ("Verify...");
        r0 = mips_read_c0_register (C0_COUNT, 0);
        for (i=first; i<last; i++) {
                if (! card_read (0, i*SECTSIZE, data, SECTSIZE)) {
                        printf ("Sector %u: read error.\n", i);
                        break;
                }
                if (! check_data (0x55^i, 0xaa^i)) {
                        printf ("Sector %u: data error.\n", i);
                        break;
                }
        }
        r1 = mips_read_c0_register (C0_COUNT, 0);
        printf (" done\n");

        kbytes = (last - first + 1) * SECTSIZE / 1024;
        printf ("Write: %u kbytes/sec\n", kbytes * CPU_KHZ/2 * 1000 / (w1 - w0));
        printf (" Read: %u kbytes/sec\n", kbytes * CPU_KHZ/2 * 1000 / (r1 - r0));
}

static void
print_data (buf, nbytes)
        char *buf;
        unsigned nbytes;
{
        unsigned i;

        printf ("%02x", (unsigned char) *buf++);
        for (i=1; i<nbytes; i++) {
                if ((i & 15) == 0)
                        printf ("\n%02x", (unsigned char) *buf++);
                else
                        printf ("-%02x", (unsigned char) *buf++);
        }
        printf ("\n");
}

int
main()
{
	startup();
	printf ("Standalone diagnostic utility\n");
	printf ("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        cpuidentify();
        cnidentify();
        spl0();

	for (;;) {
                unsigned cmd, nsectors;

                printf ("\n  1. Initialize a card");
                printf ("\n  2. Get card size");
                printf ("\n  3. Read sector #0");
                printf ("\n  4. Read sector #1");
                printf ("\n  5. Read sector #99");
                printf ("\n  6. Write sector #0");
                printf ("\n  7. Write-read sectors #0...200");
                printf ("\n  H. Halt");
                printf ("\n\n");
getcmd:
		/* Get command. */
		printf ("Command: ");
		cmd = cngetc();
                printf ("\r\33[K");

                switch (cmd) {
                case '\n': case '\r':
			break;
		case '1':
                        if (sdopen (0, 0, 0) == 0) {
                                printf ("Card initialized successfully.\n");
                        }
			break;
		case '2':
                        if (card_size (0, &nsectors))
                                printf ("%u sectors, %u kbytes, %u Mbytes\n",
                                        nsectors, nsectors/2, nsectors/2/1024);
			break;
		case '3':
                        if (card_read (0, 0, data, SECTSIZE))
                                print_data (data, SECTSIZE);
			break;
		case '4':
                        if (card_read (0, SECTSIZE, data, SECTSIZE))
                                print_data (data, SECTSIZE);
			break;
		case '5':
                        if (card_read (0, 99*SECTSIZE, data, SECTSIZE))
                                print_data (data, SECTSIZE);
			break;
		case '6':
                        if (card_write (0, 0, data, SECTSIZE))
                                printf ("Data written successfully.\n");
			break;
		case '7':
                        test_sectors (0, 200);
			break;
		case 'h': case 'H':
                        U1CON = 0x0000;
                        udelay (1000);
                        boot (0, RB_NOSYNC | RB_HALT);
			break;
                default:
                        goto getcmd;
		}
        }
}

/*
 * Translate interrupt vector number to IRQ mask.
 */
static const unsigned mask_by_vector[] = {
	(1 << PIC32_IRQ_CT),		/* 0  - Core Timer Interrupt */
	(1 << PIC32_IRQ_CS0),           /* 1  - Core Software Interrupt 0 */
	(1 << PIC32_IRQ_CS1),           /* 2  - Core Software Interrupt 1 */
	(1 << PIC32_IRQ_INT0),          /* 3  - External Interrupt 0 */
	(1 << PIC32_IRQ_T1),            /* 4  - Timer1 */
	(1 << PIC32_IRQ_IC1),           /* 5  - Input Capture 1 */
	(1 << PIC32_IRQ_OC1),           /* 6  - Output Compare 1 */
	(1 << PIC32_IRQ_INT1),          /* 7  - External Interrupt 1 */
	(1 << PIC32_IRQ_T2),            /* 8  - Timer2 */
	(1 << PIC32_IRQ_IC2),           /* 9  - Input Capture 2 */
	(1 << PIC32_IRQ_OC2),           /* 10 - Output Compare 2 */
	(1 << PIC32_IRQ_INT2),          /* 11 - External Interrupt 2 */
	(1 << PIC32_IRQ_T3),            /* 12 - Timer3 */
	(1 << PIC32_IRQ_IC3),           /* 13 - Input Capture 3 */
	(1 << PIC32_IRQ_OC3),           /* 14 - Output Compare 3 */
	(1 << PIC32_IRQ_INT3),          /* 15 - External Interrupt 3 */
	(1 << PIC32_IRQ_T4),            /* 16 - Timer4 */
	(1 << PIC32_IRQ_IC4),           /* 17 - Input Capture 4 */
	(1 << PIC32_IRQ_OC4),           /* 18 - Output Compare 4 */
	(1 << PIC32_IRQ_INT4),          /* 19 - External Interrupt 4 */
	(1 << PIC32_IRQ_T5),            /* 20 - Timer5 */
	(1 << PIC32_IRQ_IC5),           /* 21 - Input Capture 5 */
	(1 << PIC32_IRQ_OC5),           /* 22 - Output Compare 5 */
	(1 << PIC32_IRQ_SPI1E)       |  /* 23 - SPI1 */
	(1 << PIC32_IRQ_SPI1TX)      |
	(1 << PIC32_IRQ_SPI1RX),

	(1 << PIC32_IRQ_U1E)         |  /* 24 - UART1 */
	(1 << PIC32_IRQ_U1RX)        |
	(1 << PIC32_IRQ_U1TX)        |
	(1 << PIC32_IRQ_SPI3E)       |  /* 24 - SPI3 */
	(1 << PIC32_IRQ_SPI3TX)      |
	(1 << PIC32_IRQ_SPI3RX)      |
	(1 << PIC32_IRQ_I2C3B)       |  /* 24 - I2C3 */
	(1 << PIC32_IRQ_I2C3S)       |
	(1 << PIC32_IRQ_I2C3M),

	(1 << PIC32_IRQ_I2C1B)       |  /* 25 - I2C1 */
	(1 << PIC32_IRQ_I2C1S)       |
	(1 << PIC32_IRQ_I2C1M),
	(1 << (PIC32_IRQ_CN-32)),       /* 26 - Input Change Interrupt */
	(1 << (PIC32_IRQ_AD1-32)),      /* 27 - ADC1 Convert Done */
	(1 << (PIC32_IRQ_PMP-32)),      /* 28 - Parallel Master Port */
	(1 << (PIC32_IRQ_CMP1-32)),     /* 29 - Comparator Interrupt */
	(1 << (PIC32_IRQ_CMP2-32)),     /* 30 - Comparator Interrupt */

	(1 << (PIC32_IRQ_U3E-32))    |  /* 31 - UART3 */
	(1 << (PIC32_IRQ_U3E-32))    |
	(1 << (PIC32_IRQ_U3E-32))    |
	(1 << (PIC32_IRQ_SPI2E-32))  |  /* 31 - SPI2 */
	(1 << (PIC32_IRQ_SPI2TX-32)) |
	(1 << (PIC32_IRQ_SPI2RX-32)) |
	(1 << (PIC32_IRQ_I2C4B-32))  |  /* 31 - I2C4 */
	(1 << (PIC32_IRQ_I2C4S-32))  |
	(1 << (PIC32_IRQ_I2C4M-32)),

	(1 << (PIC32_IRQ_U2E-32))    |  /* 32 - UART2 */
	(1 << (PIC32_IRQ_U2RX-32))   |
	(1 << (PIC32_IRQ_U2TX-32))   |
	(1 << (PIC32_IRQ_SPI4E-32))  |  /* 32 - SPI4 */
	(1 << (PIC32_IRQ_SPI4TX-32)) |
	(1 << (PIC32_IRQ_SPI4RX-32)) |
	(1 << (PIC32_IRQ_I2C5B-32))  |  /* 32 - I2C5 */
	(1 << (PIC32_IRQ_I2C5S-32))  |
	(1 << (PIC32_IRQ_I2C5M-32)),

	(1 << (PIC32_IRQ_I2C2B-32))  |  /* 33 - I2C2 */
	(1 << (PIC32_IRQ_I2C2S-32))  |
	(1 << (PIC32_IRQ_I2C2M-32)),
	(1 << (PIC32_IRQ_FSCM-32)),     /* 34 - Fail-Safe Clock Monitor */
	(1 << (PIC32_IRQ_RTCC-32)),     /* 35 - Real-Time Clock and Calendar */
	(1 << (PIC32_IRQ_DMA0-32)),     /* 36 - DMA Channel 0 */
	(1 << (PIC32_IRQ_DMA1-32)),     /* 37 - DMA Channel 1 */
	(1 << (PIC32_IRQ_DMA2-32)),     /* 38 - DMA Channel 2 */
	(1 << (PIC32_IRQ_DMA3-32)),     /* 39 - DMA Channel 3 */
	(1 << (PIC32_IRQ_DMA4-32)),	/* 40 - DMA Channel 4 */
	(1 << (PIC32_IRQ_DMA5-32)),	/* 41 - DMA Channel 5 */
	(1 << (PIC32_IRQ_DMA6-32)),	/* 42 - DMA Channel 6 */
	(1 << (PIC32_IRQ_DMA7-32)),	/* 43 - DMA Channel 7 */
	(1 << (PIC32_IRQ_FCE-32)),      /* 44 - Flash Control Event */
	(1 << (PIC32_IRQ_USB-32)),      /* 45 - USB */
	(1 << (PIC32_IRQ_CAN1-32)),     /* 46 - Control Area Network 1 */
	(1 << (PIC32_IRQ_CAN2-32)),     /* 47 - Control Area Network 2 */
	(1 << (PIC32_IRQ_ETH-32)),      /* 48 - Ethernet Controller */
	(1 << (PIC32_IRQ_U4E-64))    |  /* 49 - UART4 */
	(1 << (PIC32_IRQ_U4RX-64))   |
	(1 << (PIC32_IRQ_U4TX-64)),
	(1 << (PIC32_IRQ_U6E-64))    |  /* 50 - UART6 */
	(1 << (PIC32_IRQ_U6RX-64))   |
	(1 << (PIC32_IRQ_U6TX-64)),
	(1 << (PIC32_IRQ_U5E-64))    |  /* 51 - UART5 */
	(1 << (PIC32_IRQ_U5RX-64))   |
	(1 << (PIC32_IRQ_U5TX-64))
};

static void
dumpregs (frame)
	int *frame;
{
	unsigned int cause;
	const char *code = 0;

	printf ("\n*** 0x%08x: exception ", frame [FRAME_PC]);

	cause = mips_read_c0_register (C0_CAUSE, 0);
	switch (cause & CA_EXC_CODE) {
	case CA_Int:	code = "Interrupt"; break;
	case CA_AdEL:	code = "Address Load"; break;
	case CA_AdES:	code = "Address Save"; break;
	case CA_IBE:	code = "Bus fetch"; break;
	case CA_DBE:	code = "Bus load/store"; break;
	case CA_Sys:	code = "Syscall"; break;
	case CA_Bp:	code = "Breakpoint"; break;
	case CA_RI:	code = "Reserved Instruction"; break;
	case CA_CPU:	code = "Coprocessor Unusable"; break;
	case CA_Ov:	code = "Arithmetic Overflow"; break;
	case CA_Tr:	code = "Trap"; break;
	}
	if (code)
		printf ("'%s'\n", code);
	else
		printf ("%d\n", cause >> 2 & 31);

	switch (cause & CA_EXC_CODE) {
	case CA_AdEL:
	case CA_AdES:
                printf ("*** badvaddr = 0x%08x\n",
                        mips_read_c0_register (C0_BADVADDR, 0));
        }
	printf ("                t0 = %8x   s0 = %8x   t8 = %8x   lo = %8x\n",
		frame [FRAME_R8], frame [FRAME_R16],
		frame [FRAME_R24], frame [FRAME_LO]);
	printf ("at = %8x   t1 = %8x   s1 = %8x   t9 = %8x   hi = %8x\n",
		frame [FRAME_R1], frame [FRAME_R9], frame [FRAME_R17],
		frame [FRAME_R25], frame [FRAME_HI]);
	printf ("v0 = %8x   t2 = %8x   s2 = %8x               status = %8x\n",
		frame [FRAME_R2], frame [FRAME_R10],
		frame [FRAME_R18], frame [FRAME_STATUS]);
	printf ("v1 = %8x   t3 = %8x   s3 = %8x                cause = %8x\n",
		frame [FRAME_R3], frame [FRAME_R11],
		frame [FRAME_R19], cause);
	printf ("a0 = %8x   t4 = %8x   s4 = %8x   gp = %8x  epc = %8x\n",
		frame [FRAME_R4], frame [FRAME_R12],
		frame [FRAME_R20], frame [FRAME_GP], frame [FRAME_PC]);
	printf ("a1 = %8x   t5 = %8x   s5 = %8x   sp = %8x\n",
		frame [FRAME_R5], frame [FRAME_R13],
		frame [FRAME_R21], frame [FRAME_SP]);
	printf ("a2 = %8x   t6 = %8x   s6 = %8x   fp = %8x\n",
		frame [FRAME_R6], frame [FRAME_R14],
		frame [FRAME_R22], frame [FRAME_FP]);
	printf ("a3 = %8x   t7 = %8x   s7 = %8x   ra = %8x\n",
		frame [FRAME_R7], frame [FRAME_R15],
		frame [FRAME_R23], frame [FRAME_RA]);
}

void
exception (frame)                       /* exception.c */
	int *frame;
{
	unsigned c, irq, status, cause;

        led_control (LED_KERNEL, 1);
	status = frame [FRAME_STATUS];
	cause = mips_read_c0_register (C0_CAUSE, 0);

	if ((cause & CA_EXC_CODE) != CA_Int) {
                /* Exception. */
                dumpregs (frame);
                panic ("unexpected exception");
        }

	/*
	 * Hardware interrupt.
	 */
        /* Get the current irq number */
        c = INTSTAT;
        if ((c & PIC32_INTSTAT_SRIPL_MASK) == 0) {
                printf ("*** unexpected interrupt: INTSTAT %08x\n", c);
                goto ret;
        }
        irq = PIC32_INTSTAT_VEC (c);

        /* Handle the interrupt. */
        switch (irq) {
        case PIC32_VECT_CT:     /* Core Timer */
#if 0
                /* Increment COMPARE register. */
                c = mips_read_c0_register (C0_COMPARE, 0);
                do {
                        c += (CPU_KHZ * 1000 / HZ + 1) / 2;
                        mips_write_c0_register (C0_COMPARE, 0, c);
                } while ((int) (c - mips_read_c0_register (C0_COUNT, 0)) < 0);

                IFSCLR(0) = 1 << PIC32_IRQ_CT;
                hardclock ((caddr_t) frame [FRAME_PC], status);
#endif
#ifdef CONSOLE_USB
                /* Poll USB on every timer tick. */
                cnintr (0);
#endif
                break;
#if defined(CONSOLE_UART1) || defined(CONSOLE_UART2) || \
defined(CONSOLE_UART3) || defined(CONSOLE_UART4) || \
defined(CONSOLE_UART5) || defined(CONSOLE_UART6)
#if CONSOLE_UART1
        case PIC32_VECT_U1:     /* UART1 */
#endif
#if CONSOLE_UART2
        case PIC32_VECT_U2:     /* UART2 */
#endif
#if CONSOLE_UART3
        case PIC32_VECT_U3:     /* UART3 */
#endif
#if CONSOLE_UART4
        case PIC32_VECT_U4:     /* UART4 */
#endif
#if CONSOLE_UART5
        case PIC32_VECT_U5:     /* UART5 */
#endif
#if CONSOLE_UART6
        case PIC32_VECT_U6:     /* UART6 */
#endif
                cnintr (0);
                break;
#endif
#ifdef CONSOLE_USB
        case PIC32_VECT_USB:    /* USB */
                IFSCLR(1) = 1 << (PIC32_IRQ_USB - 32);
                cnintr (0);
                break;
#endif
        default:
                /* Disable the irq, to avoid loops */
                printf ("*** irq %u\n", irq);
                if (irq < PIC32_VECT_CN) {
                        IECCLR(0) = mask_by_vector [irq];
                        IFSCLR(0) = mask_by_vector [irq];
                } else {
                        IECCLR(1) = mask_by_vector [irq];
                        IFSCLR(1) = mask_by_vector [irq];
                }
                break;
        }
ret:    led_control (LED_KERNEL, 0);
}

void
biodone(bp)
        register struct buf *bp;
{
        bp->b_flags |= B_DONE;
}

/*
 * Stubs.
 */
int noproc;                             /* kern_clock.c */

struct fs *
getfs(dev)                              /* ufs_subr.c */
	dev_t dev;
{
        return 0;
}

void
sync()                                  /* ufs_subr.c */
{
        /* empty */
}

void
ttyowake(tp)                            /* tty.c */
        struct tty *tp;
{
        /* empty */
}

void
ttychars(tp)                            /* tty.c */
        struct tty *tp;
{
        /* empty */
}

void
ttyclose(tp)                            /* tty.c */
        struct tty *tp;
{
        /* empty */
}

int
ttyselect (tp, rw)                      /* tty.c */
        struct tty *tp;
        int     rw;
{
        return 0;
}

int
ttioctl(tp, com, data, flag)            /* tty.c */
        struct tty *tp;
        u_int com;
        caddr_t data;
        int flag;
{
        return 0;
}

int
ttyoutput(c, tp)                        /* tty.c */
        int c;
        struct tty *tp;
{
        return 0;
}

void
ttstart(tp)                             /* tty.c */
        struct tty *tp;
{
        /* empty */
}

int
ttycheckoutq (tp, wait)                 /* tty.c */
        struct tty *tp;
        int wait;
{
        return 0;
}

int
logwrt (buf, len, log)                  /* subr_log.c */
        char    *buf;
        int     len;
        int     log;
{
        return 0;
}

void
logwakeup(unit)                         /* subr_log.c */
        int     unit;
{
        /* empty */
}

int
logisopen(unit)                         /* subr_log.c */
        int     unit;
{
        return 0;
}

int
getc(p)                                 /* tty_subr.c */
        struct clist *p;
{
        return '*';
}
