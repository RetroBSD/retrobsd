/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/inode.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/fs.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/reboot.h>
#include <sys/msgbuf.h>
#include <sys/namei.h>
#include <sys/mount.h>
#include <sys/systm.h>
#include <sys/kconfig.h>
#include <sys/tty.h>
#include <machine/uart.h>
#include <machine/usb_uart.h>
#ifdef UARTUSB_ENABLED
#   include <machine/usb_device.h>
#   include <machine/usb_function_cdc.h>
#endif

#ifdef POWER_ENABLED
extern void power_init();
extern void power_off();
#endif

#ifdef LED_TTY_INVERT
#define LED_TTY_ON()        LAT_CLR(LED_TTY_PORT) = 1 << LED_TTY_PIN
#define LED_TTY_OFF()       LAT_SET(LED_TTY_PORT) = 1 << LED_TTY_PIN
#else
#define LED_TTY_ON()        LAT_SET(LED_TTY_PORT) = 1 << LED_TTY_PIN
#define LED_TTY_OFF()       LAT_CLR(LED_TTY_PORT) = 1 << LED_TTY_PIN
#endif

#ifdef LED_DISK_INVERT
#define LED_DISK_ON()       LAT_CLR(LED_DISK_PORT) = 1 << LED_DISK_PIN
#define LED_DISK_OFF()      LAT_SET(LED_DISK_PORT) = 1 << LED_DISK_PIN
#else
#define LED_DISK_ON()       LAT_SET(LED_DISK_PORT) = 1 << LED_DISK_PIN
#define LED_DISK_OFF()      LAT_CLR(LED_DISK_PORT) = 1 << LED_DISK_PIN
#endif

#ifdef LED_KERNEL_INVERT
#define LED_KERNEL_ON()     LAT_CLR(LED_KERNEL_PORT) = 1 << LED_KERNEL_PIN
#define LED_KERNEL_OFF()    LAT_SET(LED_KERNEL_PORT) = 1 << LED_KERNEL_PIN
#else
#define LED_KERNEL_ON()     LAT_SET(LED_KERNEL_PORT) = 1 << LED_KERNEL_PIN
#define LED_KERNEL_OFF()    LAT_CLR(LED_KERNEL_PORT) = 1 << LED_KERNEL_PIN
#endif

#ifdef LED_SWAP_INVERT
#define LED_SWAP_ON()       LAT_CLR(LED_SWAP_PORT) = 1 << LED_SWAP_PIN
#define LED_SWAP_OFF()      LAT_SET(LED_SWAP_PORT) = 1 << LED_SWAP_PIN
#else
#define LED_SWAP_ON()       LAT_SET(LED_SWAP_PORT) = 1 << LED_SWAP_PIN
#define LED_SWAP_OFF()      LAT_CLR(LED_SWAP_PORT) = 1 << LED_SWAP_PIN
#endif

#ifdef LED_MISC1_INVERT
#define LED_MISC1_ON()      LAT_CLR(LED_MISC1_PORT) = 1 << LED_MISC1_PIN
#define LED_MISC1_OFF()     LAT_SET(LED_MISC1_PORT) = 1 << LED_MISC1_PIN
#else
#define LED_MISC1_ON()      LAT_SET(LED_MISC1_PORT) = 1 << LED_MISC1_PIN
#define LED_MISC1_OFF()     LAT_CLR(LED_MISC1_PORT) = 1 << LED_MISC1_PIN
#endif

#ifdef LED_MISC2_INVERT
#define LED_MISC2_ON()      LAT_CLR(LED_MISC2_PORT) = 1 << LED_MISC2_PIN
#define LED_MISC2_OFF()     LAT_SET(LED_MISC2_PORT) = 1 << LED_MISC2_PIN
#else
#define LED_MISC2_ON()      LAT_SET(LED_MISC2_PORT) = 1 << LED_MISC2_PIN
#define LED_MISC2_OFF()     LAT_CLR(LED_MISC2_PORT) = 1 << LED_MISC2_PIN
#endif

#ifdef LED_MISC3_INVERT
#define LED_MISC3_ON()      LAT_CLR(LED_MISC3_PORT) = 1 << LED_MISC3_PIN
#define LED_MISC3_OFF()     LAT_SET(LED_MISC3_PORT) = 1 << LED_MISC3_PIN
#else
#define LED_MISC3_ON()      LAT_SET(LED_MISC3_PORT) = 1 << LED_MISC3_PIN
#define LED_MISC3_OFF()     LAT_CLR(LED_MISC3_PORT) = 1 << LED_MISC3_PIN
#endif

#ifdef LED_MISC4_INVERT
#define LED_MISC4_ON()      LAT_CLR(LED_MISC4_PORT) = 1 << LED_MISC4_PIN
#define LED_MISC4_OFF()     LAT_SET(LED_MISC4_PORT) = 1 << LED_MISC4_PIN
#else
#define LED_MISC4_ON()      LAT_SET(LED_MISC4_PORT) = 1 << LED_MISC4_PIN
#define LED_MISC4_OFF()     LAT_CLR(LED_MISC4_PORT) = 1 << LED_MISC4_PIN
#endif

int     hz = HZ;
int     usechz = (1000000L + HZ - 1) / HZ;
#ifdef TIMEZONE
struct  timezone tz = { TIMEZONE, DST };
#else
struct  timezone tz = { 8*60, 1 };
#endif
int     nproc = NPROC;

struct  namecache namecache [NNAMECACHE];
char    bufdata [NBUF * MAXBSIZE];
struct  inode inode [NINODE];
struct  callout callout [NCALL];
struct  mount mount [NMOUNT];
struct  buf buf [NBUF], bfreelist [BQUEUES];
struct  bufhd bufhash [BUFHSZ];
struct  cblock cfree [NCLIST];
struct  proc proc [NPROC];
struct  file file [NFILE];

/*
 * Remove the ifdef/endif to run the kernel in unsecure mode even when in
 * a multiuser state.  Normally 'init' raises the security level to 1
 * upon transitioning to multiuser.  Setting the securelevel to -1 prevents
 * the secure level from being raised by init.
 */
#ifdef  PERMANENTLY_INSECURE
int securelevel = -1;
#else
int securelevel = 0;
#endif

struct mapent   swapent[SMAPSIZ];
struct map  swapmap[1] = {
    { swapent,
      &swapent[SMAPSIZ],
      "swapmap" },
};

int waittime = -1;

/* CPU package type: 64 pins or 100 pins. */
int cpu_pins;

static int
nodump(dev_t dev)
{
    printf("\ndumping to dev %o off %D: not implemented\n", dumpdev, dumplo);
    return 0;
}

int (*dump)(dev_t) = nodump;

dev_t   pipedev;
daddr_t dumplo = (daddr_t) 1024;

/*
 * Check whether button 1 is pressed.
 */
static inline int
button1_pressed()
{
#ifdef BUTTON1_PORT
    int val;

    TRIS_SET(BUTTON1_PORT) = 1 << BUTTON1_PIN;
    val = PORT_VAL(BUTTON1_PORT);
#ifdef BUTTON1_INVERT
    val = ~val;
#endif
    return (val >> BUTTON1_PIN) & 1;
#else
    return 0;
#endif
}

/*
 * Machine dependent startup code
 */
void
startup()
{
    extern void _etext(), _exception_base_();
    extern unsigned __data_start;

    /* Initialize STATUS register: master interrupt disable.
     * Setup interrupt vector base. */
    mips_write_c0_register(C0_STATUS, 0, ST_CU0 | ST_BEV);
    mips_write_c0_register(C0_EBASE, 1, _exception_base_);
    mips_write_c0_register(C0_STATUS, 0, ST_CU0);

    /* Set vector spacing: not used really, but must be nonzero. */
    mips_write_c0_register(C0_INTCTL, 1, 32);

    /* Clear CAUSE register: use special interrupt vector 0x200. */
    mips_write_c0_register(C0_CAUSE, 0, CA_IV);

    /* Setup memory. */
    BMXPUPBA = 512 << 10;               /* Kernel Flash memory size */
#ifdef KERNEL_EXECUTABLE_RAM
    /*
     * Set boundry for kernel executable ram on smallest
     * 2k boundry required to allow the keram segment to fit.
     * This means that there is possibly some u0area ramspace that
     * is executable, but as it is isolated from userspace this
     * should be ok, given the apparent goals of this project.
     */
    extern void _keram_start(), _keram_end();
    unsigned keram_size = (((char*)&_keram_end-(char*)&_keram_start+(2<<10))/(2<<10)*(2<<10));
    BMXDKPBA = ((32<<10)-keram_size);   /* Kernel RAM size */
    BMXDUDBA = BMXDKPBA+(keram_size);   /* Executable RAM in kernel */
#else
    BMXDKPBA = 32 << 10;                /* Kernel RAM size */
    BMXDUDBA = BMXDKPBA;                /* Zero executable RAM in kernel */
#endif
    BMXDUPBA = BMXDUDBA;                /* All user RAM is executable */

    /*
     * Setup interrupt controller.
     */
    INTCON = 0;                         /* Interrupt Control */
    IPTMR = 0;                          /* Temporal Proximity Timer */

    /* Interrupt Flag Status */
    IFS(0) = PIC32_IPC_IP0(2) | PIC32_IPC_IP1(1) |
             PIC32_IPC_IP2(1) | PIC32_IPC_IP3(1) |
             PIC32_IPC_IS0(0) | PIC32_IPC_IS1(0) |
             PIC32_IPC_IS2(0) | PIC32_IPC_IS3(0) ;
    IFS(1) = 0;
    IFS(2) = 0;

    /* Interrupt Enable Control */
    IEC(0) = 0;
    IEC(1) = 0;
    IEC(2) = 0;

    /* Interrupt Priority Control */
    unsigned ipc = PIC32_IPC_IP0(1) | PIC32_IPC_IP1(1) |
                   PIC32_IPC_IP2(1) | PIC32_IPC_IP3(1) |
                   PIC32_IPC_IS0(0) | PIC32_IPC_IS1(0) |
                   PIC32_IPC_IS2(0) | PIC32_IPC_IS3(0) ;
    IPC(0) = ipc;
    IPC(1) = ipc;
    IPC(2) = ipc;
    IPC(3) = ipc;
    IPC(4) = ipc;
    IPC(5) = ipc;
    IPC(6) = ipc;
    IPC(7) = ipc;
    IPC(8) = ipc;
    IPC(9) = ipc;
    IPC(10) = ipc;
    IPC(11) = ipc;
    IPC(12) = ipc;

    /*
     * Setup wait states.
     */
    CHECON = 2;
    BMXCONCLR = 0x40;
    CHECONSET = 0x30;

    /* Disable JTAG port, to use it for i/o. */
    DDPCON = 0;

    /* Use all B ports as digital. */
    AD1PCFG = ~0;

    /* Config register: enable kseg0 caching. */
    mips_write_c0_register(C0_CONFIG, 0,
    mips_read_c0_register(C0_CONFIG, 0) | 3);

    /* Kernel mode, interrupts disabled.  */
    mips_write_c0_register(C0_STATUS, 0, ST_CU0);
    mips_ehb();

    /*
     * Configure LED pins.
     */
#ifdef LED_TTY_PORT                     /* Terminal i/o */
    LED_TTY_OFF();
    TRIS_CLR(LED_TTY_PORT) = 1 << LED_TTY_PIN;
#endif
#ifdef LED_DISK_PORT                    /* Disk i/o */
    LED_DISK_OFF();
    TRIS_CLR(LED_DISK_PORT) = 1 << LED_DISK_PIN;
#endif
#ifdef LED_KERNEL_PORT                  /* Kernel activity */
    LED_KERNEL_OFF();
    TRIS_CLR(LED_KERNEL_PORT) = 1 << LED_KERNEL_PIN;
#endif
#ifdef LED_SWAP_PORT                    /* Auxiliary */
    LED_SWAP_OFF();
    TRIS_CLR(LED_SWAP_PORT) = 1 << LED_SWAP_PIN;
#endif
#ifdef GPIO_CLEAR_PORT                  /* Clear pin */
    LAT_CLR(GPIO_CLEAR_PORT) = 1 << GPIO_CLEAR_PIN;
    TRIS_CLR(GPIO_CLEAR_PORT) = 1 << GPIO_CLEAR_PIN;
#endif
#ifdef POWER_ENABLED
    power_init();
#endif

    //SETVAL(0);

    /* Initialize .data + .bss segments by zeroes. */
    bzero(&__data_start, KERNEL_DATA_SIZE - 96);

#if __MPLABX__
    /* Microchip C32 compiler generates a .dinit table with
     * initialization values for .data segment. */
    extern const unsigned _dinit_addr[];
    unsigned const *dinit = &_dinit_addr[0];
    for (;;) {
        char *dst = (char*) (*dinit++);
        if (dst == 0)
            break;

        unsigned nbytes = *dinit++;
        unsigned fmt = *dinit++;
        if (fmt == 0) {                 /* Clear */
            do {
                *dst++ = 0;
            } while (--nbytes > 0);
        } else {                        /* Copy */
            char *src = (char*) dinit;
            do {
                *dst++ = *src++;
            } while (--nbytes > 0);
            dinit = (unsigned*) ((unsigned) (src + 3) & ~3);
        }
    }
#else
    /* Copy the .data image from flash to ram.
     * Linker places it at the end of .text segment. */
    extern unsigned _edata;
    unsigned *src = (unsigned*) &_etext;
    unsigned *dest = &__data_start;
    unsigned *limit = &_edata;
    while (dest < limit) {
        /*printf("copy %08x from (%08x) to (%08x)\n", *src, src, dest);*/
        *dest++ = *src++;
    }

#ifdef KERNEL_EXECUTABLE_RAM
    /* Copy code that must run out of ram (due to timing restrictions)
     * from flash to the executable section of kernel ram.
     * This was added to support swap on sdram */

    extern void _ramfunc_image_begin();
    extern void _ramfunc_begin();
    extern void _ramfunc_end();

    unsigned *src1 = (unsigned*) &_ramfunc_image_begin;
    unsigned *dest1 = (unsigned*)&_ramfunc_begin;
    unsigned *limit1 = (unsigned*)&_ramfunc_end;
    /*printf("copy from (%08x) to (%08x)\n", src1, dest1);*/
    while (dest1 < limit1) {
        *dest1++ = *src1++;
    }
#endif
#endif /* __MPLABX__ */

    /*
     * Setup peripheral bus clock divisor.
     */
    unsigned osccon = OSCCON & ~PIC32_OSCCON_PBDIV_MASK;
#if BUS_DIV == 1
    osccon |= PIC32_OSCCON_PBDIV_1;
#elif BUS_DIV == 2
    osccon |= PIC32_OSCCON_PBDIV_2;
#elif BUS_DIV == 4
    osccon |= PIC32_OSCCON_PBDIV_4;
#elif BUS_DIV == 8
    osccon |= PIC32_OSCCON_PBDIV_8;
#else
#error Incorrect BUS_DIV value!
#endif
    /* Unlock access to OSCCON register */
    SYSKEY = 0;
    SYSKEY = 0xaa996655;
    SYSKEY = 0x556699aa;

    OSCCON = osccon;

    /*
     * Early setup for console devices.
     */
#if CONS_MAJOR == UART_MAJOR
    uartinit(CONS_MINOR);
#endif
#if CONS_MAJOR == UARTUSB_MAJOR
    usbinit();
#endif

    /* Get total RAM size. */
    physmem = BMXDRMSZ;

    /*
     * When button 1 is pressed - boot to single user mode.
     */
    boothowto = 0;
    if (button1_pressed()) {
        boothowto |= RB_SINGLE;
    }
}

static void cpuidentify()
{
    unsigned devid = DEVID, osccon = OSCCON;
    static const char pllmult[]  = { 15, 16, 17, 18, 19, 20, 21, 24 };
    static const char plldiv[]   = { 1, 2, 3, 4, 5, 6, 10, 12 };
    static const char *poscmod[] = { "external", "XT crystal",
                                     "HS crystal", "(disabled)" };

    printf("cpu: ");
    switch (devid & 0x0fffffff) {
    case 0x04307053:
        cpu_pins = 100;
        printf("795F512L");
        break;
    case 0x0430E053:
        cpu_pins = 64;
        printf("795F512H");
        break;
    case 0x04341053:
        cpu_pins = 100;
        printf("695F512L");
        break;
    case 0x04325053:
        cpu_pins = 64;
        printf("695F512H");
        break;
    default:
        /* Assume 100-pin package. */
        cpu_pins = 100;
        printf("DevID %08x", devid);
    }
    printf(" %u MHz, bus %u MHz\n", CPU_KHZ/1000, BUS_KHZ/1000);

    /* COSC: current oscillator selection bits */
    printf("oscillator: ");
    switch (osccon >> 12 & 7) {
    case 0:
        printf("internal Fast RC\n");
        break;
    case 1:
        printf("internal Fast RC, PLL div 1:%d mult x%d\n",
            plldiv [DEVCFG2 & 7], pllmult [osccon >> 16 & 7]);
        break;
    case 2:
        printf("%s\n", poscmod [DEVCFG1 >> 8 & 3]);
        break;
    case 3:
        printf("%s, PLL div 1:%d mult x%d\n",
            poscmod [DEVCFG1 >> 8 & 3],
            plldiv [DEVCFG2 & 7], pllmult [osccon >> 16 & 7]);
        break;
    case 4:
        printf("secondary\n");
        break;
    case 5:
        printf("internal Low-Power RC\n");
        break;
    case 6:
        printf("internal Fast RC, divided 1:16\n");
        break;
    case 7:
        printf("internal Fast RC, divided\n");
        break;
    }
}

/*
 * Check whether the controller has been successfully initialized.
 */
static int
is_controller_alive(struct driver *driver, int unit)
{
    struct conf_ctlr *ctlr;

    /* No controller - that's OK. */
    if (driver == 0)
        return 1;

    for (ctlr = conf_ctlr_init; ctlr->ctlr_driver; ctlr++) {
        if (ctlr->ctlr_driver == driver &&
            ctlr->ctlr_unit == unit &&
            ctlr->ctlr_alive)
        {
            return 1;
        }
    }
    return 0;
}

/*
 * Configure all controllers and devices as specified
 * in the kernel configuration file.
 */
void kconfig()
{
    struct conf_ctlr *ctlr;
    struct conf_device *dev;

    cpuidentify();

    /* Probe and initialize controllers first. */
    for (ctlr = conf_ctlr_init; ctlr->ctlr_driver; ctlr++) {
        if ((*ctlr->ctlr_driver->d_init)(ctlr)) {
            ctlr->ctlr_alive = 1;
        }
    }

    /* Probe and initialize devices. */
    for (dev = conf_device_init; dev->dev_driver; dev++) {
        if (is_controller_alive(dev->dev_cdriver, dev->dev_ctlr)) {
            if ((*dev->dev_driver->d_init)(dev)) {
                dev->dev_alive = 1;
            }
        }
    }
}

/*
 * Sit and wait for something to happen...
 */
void
idle()
{
    /* Indicate that no process is running. */
    noproc = 1;

    /* Set SPL low so we can be interrupted. */
    int x = spl0();

    /* Wait for something to happen. */
    asm volatile ("wait");

    /* Restore previous SPL. */
    splx(x);
}

void
boot(dev_t dev, int howto)
{
    if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw) {
        register struct fs *fp;
        register struct buf *bp;
        int iter, nbusy;

        /*
         * Force the root filesystem's superblock to be updated,
         * so the date will be as current as possible after
         * rebooting.
         */
        fp = getfs(rootdev);
        if (fp)
            fp->fs_fmod = 1;
        waittime = 0;
        printf("syncing disks... ");
        (void) splnet();
        sync();
        for (iter = 0; iter < 20; iter++) {
            nbusy = 0;
            for (bp = &buf[NBUF]; --bp >= buf; )
                if (bp->b_flags & B_BUSY)
                    nbusy++;
            if (nbusy == 0)
                break;
            printf("%d ", nbusy);
            udelay(40000L * iter);
        }
        printf("done\n");
    }
    (void) splhigh();
    if (! (howto & RB_HALT)) {
        if ((howto & RB_DUMP) && dumpdev != NODEV) {
            /*
             * Take a dump of memory by calling (*dump)(),
             * which must correspond to dumpdev.
             * It should dump from dumplo blocks to the end
             * of memory or to the end of the logical device.
             */
            (*dump)(dumpdev);
        }
        /* Restart from dev, howto */
#ifdef UARTUSB_ENABLED
        /* Disable USB module, and wait awhile for the USB cable
         * capacitance to discharge down to disconnected (SE0) state.
         */
        U1CON = 0x0000;
        udelay(1000);

        /* Stop DMA */
        if (! (DMACON & 0x1000)) {
            DMACONSET = 0x1000;
            while (DMACON & 0x800)
                continue;
        }
#endif
        /* Unlock access to reset register */
        SYSKEY = 0;
        SYSKEY = 0xaa996655;
        SYSKEY = 0x556699aa;

        /* Reset microcontroller */
        RSWRSTSET = 1;
        (void) RSWRST;
    }
    printf("halted\n");

    if (howto & RB_BOOTLOADER) {
        printf("entering bootloader\n");
        BLRKEY = 0x12345678;
        /* Unlock access to reset register */
        SYSKEY = 0;
        SYSKEY = 0xaa996655;
        SYSKEY = 0x556699aa;

        /* Reset microcontroller */
        RSWRSTSET = 1;
        (void) RSWRST;
    }

#ifdef HALTREBOOT
    printf("press any key to reboot...");
    cngetc();

    /* Unlock access to reset register */
    SYSKEY = 0;
    SYSKEY = 0xaa996655;
    SYSKEY = 0x556699aa;

    /* Reset microcontroller */
    RSWRSTSET = 1;
    (void) RSWRST;
#endif

    for (;;) {
#ifdef UARTUSB_ENABLED
        usb_device_tasks();
        cdc_consume(0);
        cdc_tx_service();
#else
#ifdef POWER_ENABLED
        if (howto & RB_POWEROFF)
            power_off();
#endif
        asm volatile ("wait");
#endif
    }
    /*NOTREACHED*/
}

/*
 * Microsecond delay routine for MIPS processor.
 *
 * We rely on a hardware register Count, which is increased
 * every next clock cycle, i.e. at rate CPU_KHZ/2 per millisecond.
 */
void
udelay(u_int usec)
{
    unsigned now = mips_read_c0_register(C0_COUNT, 0);
    unsigned final = now + usec * (CPU_KHZ / 1000) / 2;

    for (;;) {
        now = mips_read_c0_register(C0_COUNT, 0);

        /* This comparison is valid only when using a signed type. */
        if ((int) (now - final) >= 0)
            break;
    }
}

/*
 * Control LEDs, installed on the board.
 */
void led_control(int mask, int on)
{
#ifdef LED_SWAP_PORT
    if (mask & LED_SWAP) {       /* Auxiliary */
        if (on) LED_SWAP_ON();
        else    LED_SWAP_OFF();
    }
#endif
#ifdef LED_DISK_PORT
    if (mask & LED_DISK) {      /* Disk i/o */
        if (on) LED_DISK_ON();
        else    LED_DISK_OFF();
    }
#endif
#ifdef LED_KERNEL_PORT
    if (mask & LED_KERNEL) {    /* Kernel activity */
        if (on) LED_KERNEL_ON();
        else    LED_KERNEL_OFF();
    }
#endif
#ifdef LED_TTY_PORT
    if (mask & LED_TTY) {       /* Terminal i/o */
        if (on) LED_TTY_ON();
        else    LED_TTY_OFF();
    }
#endif
}

/*
 * Increment user profiling counters.
 */
void addupc(caddr_t pc, struct uprof *pbuf, int ticks)
{
    unsigned indx;

    if (pc < (caddr_t) pbuf->pr_off)
        return;

    indx = pc - (caddr_t) pbuf->pr_off;
    indx = (indx * pbuf->pr_scale) >> 16;
    if (indx >= pbuf->pr_size)
        return;

    pbuf->pr_base[indx] += ticks;
}

/*
 * Find the index of the least significant set bit in the 32-bit word.
 * If LSB bit is set - return 1.
 * If only MSB bit is set - return 32.
 * Return 0 when no bit is set.
 */
int
ffs(u_long i)
{
    if (i != 0)
        i = 32 - mips_clz(i & -i);
    return i;
}

/*
 * Copy a null terminated string from one point to another.
 * Returns zero on success, ENOENT if maxlength exceeded.
 * If lencopied is non-zero, *lencopied gets the length of the copy
 * (including the null terminating byte).
 */
int
copystr(caddr_t src, caddr_t dest, u_int maxlength, u_int *lencopied)
{
    caddr_t dest0 = dest;
    int error = ENOENT;

    if (maxlength != 0) {
        while ((*dest++ = *src++) != '\0') {
            if (--maxlength == 0) {
                /* Failed. */
                goto done;
            }
        }
        /* Succeeded. */
        error = 0;
    }
done:
    if (lencopied != 0)
        *lencopied = dest - dest0;
    return error;
}

/*
 * Calculate the length of a string.
 */
size_t
strlen(const char *s)
{
    const char *s0 = s;

    while (*s++ != '\0')
        ;
    return s - s0 - 1;
}

/*
 * Return 0 if a user address is valid.
 * There are two memory regions allowed for user: flash and RAM.
 */
int
baduaddr(caddr_t addr)
{
    if (addr >= (caddr_t) USER_FLASH_START &&
        addr < (caddr_t) USER_FLASH_END)
        return 0;
    if (addr >= (caddr_t) USER_DATA_START &&
        addr < (caddr_t) USER_DATA_END)
        return 0;
    return 1;
}

/*
 * Return 0 if a kernel address is valid.
 * There is only one memory region allowed for kernel: RAM.
 */
int
badkaddr(caddr_t addr)
{
    if (addr >= (caddr_t) KERNEL_DATA_START &&
        addr < (caddr_t) KERNEL_DATA_END)
        return 0;
    if (addr >= (caddr_t) KERNEL_FLASH_START &&
        addr < (caddr_t) KERNEL_FLASH_START + FLASH_SIZE)
        return 0;
    return 1;
}

/*
 * Insert the specified element into a queue immediately after
 * the specified predecessor element.
 */
void insque(void *element, void *predecessor)
{
    struct que {
        struct que *q_next;
        struct que *q_prev;
    };
    register struct que *e = (struct que *) element;
    register struct que *prev = (struct que *) predecessor;

    e->q_prev = prev;
    e->q_next = prev->q_next;
    prev->q_next->q_prev = e;
    prev->q_next = e;
}

/*
 * Remove the specified element from the queue.
 */
void remque(void *element)
{
    struct que {
        struct que *q_next;
        struct que *q_prev;
    };
    register struct que *e = (struct que *) element;

    e->q_prev->q_next = e->q_next;
    e->q_next->q_prev = e->q_prev;
}

/*
 * Compare strings.
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
    register int ret, tmp;

    if (n == 0)
        return 0;
    do {
        ret = *s1++ - (tmp = *s2++);
    } while ((ret == 0) && (tmp != 0) && --n);
    return ret;
}

/* Nonzero if pointer is not aligned on a "sz" boundary.  */
#define UNALIGNED(p, sz)    ((unsigned) (p) & ((sz) - 1))

/*
 * Copy data from the memory region pointed to by src0 to the memory
 * region pointed to by dst0.
 * If the regions overlap, the behavior is undefined.
 */
void
bcopy(const void *src0, void *dst0, size_t nbytes)
{
    unsigned char *dst = dst0;
    const unsigned char *src = src0;
    unsigned *aligned_dst;
    const unsigned *aligned_src;

//printf("bcopy (%08x, %08x, %d)\n", src0, dst0, nbytes);
    /* If the size is small, or either SRC or DST is unaligned,
     * then punt into the byte copy loop.  This should be rare.  */
    if (nbytes >= 4*sizeof(unsigned) &&
        ! UNALIGNED(src, sizeof(unsigned)) &&
        ! UNALIGNED(dst, sizeof(unsigned))) {
        aligned_dst = (unsigned*) dst;
        aligned_src = (const unsigned*) src;

        /* Copy 4X unsigned words at a time if possible.  */
        while (nbytes >= 4*sizeof(unsigned)) {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            nbytes -= 4*sizeof(unsigned);
        }

        /* Copy one unsigned word at a time if possible.  */
        while (nbytes >= sizeof(unsigned)) {
            *aligned_dst++ = *aligned_src++;
            nbytes -= sizeof(unsigned);
        }

        /* Pick up any residual with a byte copier.  */
        dst = (unsigned char*) aligned_dst;
        src = (const unsigned char*) aligned_src;
    }

    while (nbytes--)
        *dst++ = *src++;
}

void *
memcpy(void *dst, const void *src, size_t nbytes)
{
    bcopy(src, dst, nbytes);
    return dst;
}

/*
 * Fill the array with zeroes.
 */
void
bzero(void *dst0, size_t nbytes)
{
    unsigned char *dst;
    unsigned *aligned_dst;

    dst = (unsigned char*) dst0;
    while (UNALIGNED(dst, sizeof(unsigned))) {
        *dst++ = 0;
        if (--nbytes == 0)
            return;
    }
    if (nbytes >= sizeof(unsigned)) {
        /* If we get this far, we know that nbytes is large and dst is word-aligned. */
        aligned_dst = (unsigned*) dst;

        while (nbytes >= 4*sizeof(unsigned)) {
            *aligned_dst++ = 0;
            *aligned_dst++ = 0;
            *aligned_dst++ = 0;
            *aligned_dst++ = 0;
            nbytes -= 4*sizeof(unsigned);
        }
        while (nbytes >= sizeof(unsigned)) {
            *aligned_dst++ = 0;
            nbytes -= sizeof(unsigned);
        }
        dst = (unsigned char*) aligned_dst;
    }

    /* Pick up the remainder with a bytewise loop.  */
    while (nbytes--)
        *dst++ = 0;
}

/*
 * Compare not more than nbytes of data pointed to by m1 with
 * the data pointed to by m2. Return an integer greater than, equal to or
 * less than zero according to whether the object pointed to by
 * m1 is greater than, equal to or less than the object
 * pointed to by m2.
 */
int
bcmp(const void *m1, const void *m2, size_t nbytes)
{
    const unsigned char *s1 = (const unsigned char*) m1;
    const unsigned char *s2 = (const unsigned char*) m2;
    const unsigned *aligned1, *aligned2;

    /* If the size is too small, or either pointer is unaligned,
     * then we punt to the byte compare loop.  Hopefully this will
     * not turn up in inner loops.  */
    if (nbytes >= 4*sizeof(unsigned) &&
        ! UNALIGNED(s1, sizeof(unsigned)) &&
        ! UNALIGNED(s2, sizeof(unsigned))) {
        /* Otherwise, load and compare the blocks of memory one
           word at a time.  */
        aligned1 = (const unsigned*) s1;
        aligned2 = (const unsigned*) s2;
        while (nbytes >= sizeof(unsigned)) {
            if (*aligned1 != *aligned2)
                break;
            aligned1++;
            aligned2++;
            nbytes -= sizeof(unsigned);
        }

        /* check remaining characters */
        s1 = (const unsigned char*) aligned1;
        s2 = (const unsigned char*) aligned2;
    }
    while (nbytes--) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++;
        s2++;
    }
    return 0;
}

int
copyout(caddr_t from, caddr_t to, u_int nbytes)
{
    //printf("copyout (from=%p, to=%p, nbytes=%u)\n", from, to, nbytes);
    if (baduaddr(to) || baduaddr(to + nbytes - 1))
        return EFAULT;
    bcopy(from, to, nbytes);
    return 0;
}

int copyin (caddr_t from, caddr_t to, u_int nbytes)
{
    if (baduaddr(from) || baduaddr(from + nbytes - 1))
        return EFAULT;
    bcopy(from, to, nbytes);
    return 0;
}

/*
 * Routines for access to general purpose I/O pins.
 */
static const char pin_name[16] = "?ABCDEFG????????";

void gpio_set_input(int pin)
{
    struct gpioreg *port = (struct gpioreg*) &TRISA;

    port += (pin >> 4 & 15) - 1;
    port->trisset = (1 << (pin & 15));
}

void gpio_set_output(int pin)
{
    struct gpioreg *port = (struct gpioreg*) &TRISA;

    port += (pin >> 4 & 15) - 1;
    port->trisclr = (1 << (pin & 15));
}

void gpio_set(int pin)
{
    struct gpioreg *port = (struct gpioreg*) &TRISA;

    port += (pin >> 4 & 15) - 1;
    port->latset = (1 << (pin & 15));
}

void gpio_clr(int pin)
{
    struct gpioreg *port = (struct gpioreg*) &TRISA;

    port += (pin >> 4 & 15) - 1;
    port->latclr = (1 << (pin & 15));
}

int gpio_get(int pin)
{
    struct gpioreg *port = (struct gpioreg*) &TRISA;

    port += (pin >> 4 & 15) - 1;
    return ((port->port & (1 << (pin & 15))) ? 1 : 0);
}

char gpio_portname(int pin)
{
    return pin_name[(pin >> 4) & 15];
}

int gpio_pinno(int pin)
{
    return pin & 15;
}
