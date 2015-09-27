/*
 * GPIO driver for PIC32.
 *
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
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
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/kconfig.h>

/*
 * Devices:
 *      /dev/porta ... /dev/portg
 *      /dev/confa ... /dev/confg
 *
 * Example:
 *      echo ....oiiid....iiii > /dev/confa
 *      echo ....1...0........ > /dev/porta
 *
 * Write to /dev/confX:
 *      'i' - configure the corresponding port pin as an input;
 *      'o' - configure the corresponding port pin as an output;
 *      'd' - configure the corresponding port pin as an open-drain output;
 *      'x' - deconfigure the corresponding port pin;
 *      '.' - no action.
 *
 * Write to /dev/portX:
 *      '0' - set output pin low;
 *      '1' - set output pin high;
 *      '+' - invert the value of output pin;
 *      '.' - no action.
 *
 * Use ioctl() on any of devices to control pins from the user program.
 *      ioctl(fd, GPIO_PORTA |  GPIO_CONFIN, mask)  - configure as input
 *      ioctl(fd, GPIO_PORTB |  GPIO_CONFOUT,mask)  - configure as output
 *      ioctl(fd, GPIO_PORTC |  GPIO_CONFOD, mask)  - configure as open drain
 *      ioctl(fd, GPIO_PORTD |  GPIO_DECONF, mask)  - deconfigure
 *      ioctl(fd, GPIO_PORTE |  GPIO_STORE,  val)   - set values of all pins
 *      ioctl(fd, GPIO_PORTF |  GPIO_SET,    mask)  - set to 1 by mask
 *      ioctl(fd, GPIO_PORTG |  GPIO_CLEAR,  mask)  - set to 0 by mask
 *      ioctl(fd, GPIO_PORT(0)| GPIO_INVERT, mask)  - invert by mask
 * val= ioctl(fd, GPIO_PORT(1)| GPIO_POLL,   0)     - get input values
 *
 * Several operations can be combined in one call.
 * For example, to toggle pin A2 high thew low, and get value
 * of all PORTA pins:
 * val = ioctl(fd, GPIO_PORTA | GPIO_SET | GPIO_CLEAR | GPIO_POLL, 1<<3);
 */
#define NGPIO           7               /* Ports A, B, C, D, E, F, G */
#define NPINS           16              /* Number of pins per port */

#define MINOR_CONF      0x40            /* Minor mask: /dev/confX */
#define MINOR_UNIT      0x07            /* Minor mask: unit number */

/*
 * Some pins are actually not available in hardware.
 * Here are masks of real pins.
 */
#define MASK(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
    (a<<15 | b<<14 | c<<13 | d<<12 | e<<11 | f<<10 | g<<9 | h<<8 | \
     i<<7  | j<<6  | k<<5  | l<<4  | m<<3  | n<<2  | o<<1 | p)

/*
 * Mask of configured pins, default empty.
 */
u_int gpio_confmask [NGPIO];

/*
 * To enable debug output, comment out the first line,
 * and uncomment the second line.
 */
#define PRINTDBG(...) /*empty*/
//#define PRINTDBG printf

static void
gpio_print (dev, buf)
    dev_t dev;
    char *buf;
{
    u_int unit = minor(dev) & MINOR_UNIT;
    register struct gpioreg *reg = unit + (struct gpioreg*) &TRISA;
    register u_int mask, conf, tris;
    register char c;

    conf = gpio_confmask[unit];
    tris = reg->tris;
    if (minor(dev) & MINOR_CONF) {
        /* /dev/confX device: port configuration mask */
        u_int odc = reg->odc;
        for (mask=1<<(NPINS-1); mask; mask>>=1) {
            if (! (conf & mask))
                c = '-';
            else if (tris & mask)
                c = 'i';
            else
                c = (odc & mask) ? 'd' : 'o';
            *buf++ = c;
        }
    } else {
        /* /dev/portX device: port value mask */
        u_int lat = reg->lat;
        u_int port = reg->port;
        for (mask=1<<(NPINS-1); mask; mask>>=1) {
            if (! (conf & mask))
                c = '-';
            else if (tris & mask)
                c = (port & mask) ? '1' : '0';
            else
                c = (lat & mask) ? '1' : '0';
            *buf++ = c;
        }
    }
    *buf++ = '\n';
    *buf = 0;
}

static void
gpio_parse (dev, buf)
    dev_t dev;
    char *buf;
{
    u_int unit = minor(dev) & MINOR_UNIT;
    register struct gpioreg *reg = unit + (struct gpioreg*) &TRISA;
    register u_int mask;
    register char c;

    if (minor(dev) & MINOR_CONF) {
        /* /dev/confX device: port configuration mask */
        for (mask=1<<(NPINS-1); mask; mask>>=1) {
            c = *buf++;
            if (c <= ' ' || c > '~')
                break;

            if (c == 'x' || c == 'X') {
                gpio_confmask[unit] &= ~mask;
                reg->trisset = mask;
            } else if (c == 'i' || c == 'I') {
                gpio_confmask[unit] |= mask;
                reg->trisset = mask;
            } else if (c == 'o' || c == 'O') {
                gpio_confmask[unit] |= mask;
                reg->odcclr = mask;
                reg->trisclr = mask;
            } else if (c == 'd' || c == 'D') {
                gpio_confmask[unit] |= mask;
                reg->odcset = mask;
                reg->trisclr = mask;
            }
        }
    } else {
        /* /dev/portX device: port value mask */
        u_int conf = gpio_confmask[unit];
        u_int tris = reg->tris;
        for (mask=1<<(NPINS-1); mask; mask>>=1) {
            c = *buf++;
            if (c <= ' ' || c > '~')
                break;

            if (! (conf & mask) || (tris & mask))
                continue;

            if (c == '0')
                reg->latclr = mask;
            else
                reg->latset = mask;
        }
    }
}

int
gpioopen (dev, flag, mode)
    dev_t dev;
{
    register u_int unit = minor(dev) & MINOR_UNIT;

    if (unit >= NGPIO)
        return ENXIO;
    if (u.u_uid != 0)
        return EPERM;
    return 0;
}

int
gpioclose (dev, flag, mode)
    dev_t dev;
{
    return 0;
}

int
gpioread (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    register struct iovec *iov;
    register u_int cnt = NPINS + 1;
    char buf [20];

    /* I/o size should be large enough. */
    iov = uio->uio_iov;
    if (iov->iov_len < cnt)
        return EIO;

    /* Read only cnt bytes. */
    if (uio->uio_offset >= cnt)
        return 0;
    cnt -= uio->uio_offset;

    /* Print port status to buffer. */
    gpio_print (dev, buf);
    //PRINTDBG ("gpioread -> %s", buf);

    bcopy (buf + uio->uio_offset, iov->iov_base, cnt);
    iov->iov_base += cnt;
    iov->iov_len -= cnt;
    uio->uio_resid -= cnt;
    uio->uio_offset += cnt;
    return 0;
}

int
gpiowrite (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    register struct iovec *iov = uio->uio_iov;
    register u_int cnt = NPINS;
    char buf [20];

    /* I/o size should be large enough. */
    if (iov->iov_len < cnt)
        return EIO;

    bcopy (iov->iov_base, buf, cnt);
    iov->iov_base += cnt;
    iov->iov_len -= cnt;
    uio->uio_resid -= cnt;
    uio->uio_offset += cnt;

    PRINTDBG ("gpiowrite ('%s')\n", buf);
    gpio_parse (dev, buf);
    return 0;
}

/*
 * Display a picture on LoL shield.
 * Duration in milliseconds is specified.
 */
static void
gpio_lol (msec, data)
    u_int msec;
    const short *data;
{
    /* Number of control pins for LoL Shield. */
    #define LOL_NPINS   12

    /* Number of rows on LoL Shield. */
    #define LOL_NROW    9

    /* Number of columns on LoL Shield. */
    #define LOL_NCOL    14

    /* Sequence of pins to set high during refresh cycle. */
    static const unsigned high [LOL_NPINS] = {
        1 << 10,        /* PB10 - labeled D13 on board (connect to A5) */
        1 << 9,         /* PB9  - D12 (connect to A4) */
        1 << 7,         /* PB7  - D11 (connect to A3) */
        1 << 6,         /* PB6  - D10 (connect to A2) */
        1 << 4,         /* PB4  - D9 (connect to A1) */
        1 << 11,        /* PB11 - D8 */
        0x10000 << 7,   /* PE7  - D7 */
        0x10000 << 6,   /* PE6  - D6 */
        0x10000 << 5,   /* PE5  - D5 */
        0x10000 << 4,   /* PE4  - D4 */
        0x10000 << 3,   /* PE3  - D3 */
        0x10000 << 2,   /* PE2  - D2 */
    };

    /* Remap pixels to pin indexes. */
    static const unsigned char lol_map [LOL_NROW*LOL_NCOL*2] =
    {
        0,8,0,7,0,6,0,5,0,4,0,3,0,2,0,1,0,9,9,0,0,10,10,0,0,11,11,0,
        1,8,1,7,1,6,1,5,1,4,1,3,1,2,1,0,1,9,9,1,1,10,10,1,1,11,11,1,
        2,8,2,7,2,6,2,5,2,4,2,3,2,1,2,0,2,9,9,2,2,10,10,2,2,11,11,2,
        3,8,3,7,3,6,3,5,3,4,3,2,3,1,3,0,3,9,9,3,3,10,10,3,3,11,11,3,
        4,8,4,7,4,6,4,5,4,3,4,2,4,1,4,0,4,9,9,4,4,10,10,4,4,11,11,4,
        5,8,5,7,5,6,5,4,5,3,5,2,5,1,5,0,5,9,9,5,5,10,10,5,5,11,11,5,
        6,8,6,7,6,5,6,4,6,3,6,2,6,1,6,0,6,9,9,6,6,10,10,6,6,11,11,6,
        7,8,7,6,7,5,7,4,7,3,7,2,7,1,7,0,7,9,9,7,7,10,10,7,7,11,11,7,
        8,7,8,6,8,5,8,4,8,3,8,2,8,1,8,0,8,9,9,8,8,10,10,8,8,11,11,8,
    };

    unsigned row, mask, bmask, emask;
    const unsigned char *map;
    unsigned low [LOL_NPINS];

    /* Clear pin masks. */
    for (row = 0; row < LOL_NPINS; row++)
        low [row] = 0;

    /* Convert image to array of pin masks. */
    for (row = 0; row < LOL_NROW; row++) {
        mask = *data++ & ((1 << LOL_NCOL) - 1);
        map = &lol_map [row * LOL_NCOL * 2];
        while (mask != 0) {
            if (mask & 1) {
                low [map[0]] |= high [map[1]];
            }
            map += 2;
            mask >>= 1;
        }
    }
    bmask = high[0] | high[1] | high[2] | high[3] | high[4] | high[5];
    emask = (high[6] | high[7] | high[8] | high[9] | high[10] |
         high[11]) >> 16;

    /* Display the image. */
    if (msec < 1)
        msec = 20;
    while (msec-- > 0) {
        for (row = 0; row < LOL_NPINS; row++) {
            /* Set all pins to tristate. */
            TRISBSET = bmask;
            TRISESET = emask;

            /* Set one pin to high. */
            mask = high [row];
            if (row < 6) {
                TRISBCLR = mask;
                LATBSET = mask;
            } else {
                mask >>= 16;
                TRISECLR = mask;
                LATESET = mask;
            }

            /* Set other pins to low. */
            mask = low [row];
            TRISBCLR = mask;
            LATBCLR = mask;
            mask >>= 16;
            TRISECLR = mask;
            LATECLR = mask;

            /* Pause to make it visible. */
            udelay (1000 / LOL_NPINS);
        }
    }

    /* Turn display off. */
    TRISBSET = bmask;
    TRISESET = emask;
}

/*
 * Commands:
 * GPIO_CONFIN  - configure as input
 * GPIO_CONFOUT - configure as output
 * GPIO_CONFOD  - configure as open drain
 * GPIO_DECONF  - deconfigure
 * GPIO_STORE   - store all outputs
 * GPIO_SET     - set to 1 by mask
 * GPIO_CLEAR   - set to 0 by mask
 * GPIO_INVERT  - invert by mask
 * GPIO_POLL    - poll
 *
 * Use GPIO_PORT(n) to set port number.
 */
int
gpioioctl (dev, cmd, addr, flag)
    dev_t dev;
    register u_int cmd;
    caddr_t addr;
{
    register u_int unit, mask, value;
    register struct gpioreg *reg;

    PRINTDBG ("gpioioctl (cmd=%08x, addr=%08x, flag=%d)\n", cmd, addr, flag);
    unit = cmd & 0xff;
    cmd &= ~0xff;
    if (cmd == GPIO_LOL) {
        /* display 9x14 image on a lol shield  */
        if (baduaddr (addr) || baduaddr (addr + LOL_NROW*2 - 1))
            return EFAULT;
        gpio_lol (unit, (const short*) addr);
        return 0;
    }

    if ((cmd & (IOC_INOUT | IOC_VOID)) != IOC_VOID ||
        ((cmd >> 8) & 0xff) != 'g')
        return EINVAL;
    if (unit >= NGPIO)
        return ENXIO;

    reg = unit + (struct gpioreg*) &TRISA;
    mask = (u_int) addr & 0xffff;
    if (cmd & GPIO_COMMAND & (GPIO_CONFIN | GPIO_CONFOUT | GPIO_CONFOD))
        mask = mask;
    else
        mask &= gpio_confmask[unit];

    if (cmd & GPIO_COMMAND & GPIO_CONFIN) {
        /* configure as input */
        PRINTDBG ("TRIS%cSET %p := %04x\n", unit+'A', &reg->trisset, mask);
        reg->trisset = mask;
        gpio_confmask[unit] |= mask;

        /* skip output-only bits */
        gpio_confmask[unit] ^= mask & ~reg->tris;
    }
    if (cmd & GPIO_COMMAND & (GPIO_CONFOUT | GPIO_CONFOD)) {
        if (cmd & GPIO_COMMAND & GPIO_CONFOUT) {
            /* configure as output */
            PRINTDBG ("ODC%cCLR %p := %04x\n", unit+'A', &reg->odcclr, mask);
            reg->odcclr = mask;
        } else {
            /* configure as open drain */
            PRINTDBG ("ODC%cSET %p := %04x\n", unit+'A', &reg->odcset, mask);
            reg->odcset = mask;
        }
        PRINTDBG ("TRIS%cCLR %p := %04x\n", unit+'A', &reg->trisclr, mask);
        reg->trisclr = mask;
        gpio_confmask[unit] |= mask;

        /* skip input-only bits */
        gpio_confmask[unit] ^= mask & reg->tris;
    }
    if (cmd & GPIO_COMMAND & GPIO_DECONF) {
        /* deconfigure */
        gpio_confmask[unit] &= ~mask;
    }
    if (cmd & GPIO_COMMAND & GPIO_STORE) {
        /* store all outputs */
        value = reg->lat;
        PRINTDBG ("LAT%c %p -> %04x\n", unit+'A', &reg->lat, value);
        value &= ~gpio_confmask[unit];
        value |= mask;
        PRINTDBG ("LAT%c %p := %04x\n", unit+'A', &reg->lat, value);
        reg->lat = value;
    }
    if (cmd & GPIO_COMMAND & GPIO_SET) {
        /* set to 1 by mask */
        PRINTDBG ("LAT%cSET %p := %04x\n", unit+'A', &reg->latset, mask);
        reg->latset = mask;
    }
    if (cmd & GPIO_COMMAND & GPIO_CLEAR) {
        /* set to 0 by mask */
        PRINTDBG ("LAT%cCLR %p := %04x\n", unit+'A', &reg->latclr, mask);
        reg->latclr = mask;
    }
    if (cmd & GPIO_COMMAND & GPIO_INVERT) {
        /* invert by mask */
        PRINTDBG ("LAT%cINV %p := %04x\n", unit+'A', &reg->latinv, mask);
        reg->latinv = mask;
    }
    if (cmd & GPIO_COMMAND & GPIO_POLL) {
        /* send current pin values to user */
        value = reg->port;
        PRINTDBG ("PORT%c %p -> %04x\n", unit+'A', &reg->port, value);
        u.u_rval = value;
    }
    return 0;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
gpioprobe(config)
    struct conf_device *config;
{
    int unit = config->dev_unit;
    int flags = config->dev_flags;
    char buf[20];

    if (unit < 0 || unit >= NGPIO)
        return 0;

    gpio_confmask[unit] = flags;

    gpio_print(unit | MINOR_CONF, buf);
    printf("gpio%u: port%c, pins %s", unit, unit + 'A', buf);
    return 1;
}

struct driver gpiodriver = {
    "gpio", gpioprobe,
};
