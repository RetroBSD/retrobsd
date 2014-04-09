/*
 * GPIO emulation for PIC32.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "device.h"
#include "mips_memory.h"
#include "pic32.h"
#include "cpu.h"
#include "vp_timer.h"

#define GPIO_REG_SIZE   0x1F0

#ifdef UBW32
#define MASKA_CS0       (1 << 9)            // A9: sd0 on SPI1
#define MASKA_CS1       (1 << 10)           // A10: sd1 on SPI1
#define MASKC_LDADDR    (1 << 1)            // C1: swap LDADDR
#define MASKE_WR        (1 << 9)            // E8: swap WR
#define MASKE_RD        (1 << 8)            // E8: swap RD
#define MASKE_DATA      (0xff << 0)         // E0-E7: swap DATA
#define SHIFTE_DATA     0

#elif defined MAXIMITE
#define MASKE_CS0       (1 << 0)            // E0: sd0 on SPI4
#define MASKE_CS1                           // reserved
#define MASKD_PS2C      (1 << 6)            // PS2 keyboard
#define MASKD_PS2D      (1 << 7)

#elif defined MAX32
#define MASKD_CS0       (1 << 3)            // D3: sd0 on SPI4
#define MASKD_CS1       (1 << 4)            // D4: sd1 on SPI4

#elif defined EXPLORER16
#define MASKB_CS0       (1 << 1)            // B1: sd0 on SPI1
#define MASKB_CS1       (1 << 2)            // B2: sd1 on SPI1
#endif

struct pic32_gpio_data {
    struct vdevice  *dev;
    vtty_t          *vtty;
    vm_instance_t   *vm;
    pic32_t         *pic32;

    unsigned        tris_a;         /* 0x00 - port A mask of inputs */
    unsigned        port_a;         /* 0x10 - port A pins */
    unsigned        lat_a;          /* 0x20 - port A latched outputs */
    unsigned        odc_a;          /* 0x30 - port A open drain configuration */

    unsigned        tris_b;         /* 0x40 - port B mask of inputs */
    unsigned        port_b;         /* 0x50 - port B pins */
    unsigned        lat_b;          /* 0x60 - port B latched outputs */
    unsigned        odc_b;          /* 0x70 - port B open drain configuration */

    unsigned        tris_c;         /* 0x80 - port C mask of inputs */
    unsigned        port_c;         /* 0x90 - port C pins */
    unsigned        lat_c;          /* 0xA0 - port C latched outputs */
    unsigned        odc_c;          /* 0xB0 - port C open drain configuration */

    unsigned        tris_d;         /* 0xC0 - port D mask of inputs */
    unsigned        port_d;         /* 0xD0 - port D pins */
    unsigned        lat_d;          /* 0xE0 - port D latched outputs */
    unsigned        odc_d;          /* 0xF0 - port D open drain configuration */

    unsigned        tris_e;         /* 0x00 - port E mask of inputs */
    unsigned        port_e;         /* 0x10 - port E pins */
    unsigned        lat_e;          /* 0x20 - port E latched outputs */
    unsigned        odc_e;          /* 0x30 - port E open drain configuration */

    unsigned        tris_f;         /* 0x40 - port F mask of inputs */
    unsigned        port_f;         /* 0x50 - port F pins */
    unsigned        lat_f;          /* 0x60 - port F latched outputs */
    unsigned        odc_f;          /* 0x70 - port F open drain configuration */

    unsigned        tris_g;         /* 0x80 - port G mask of inputs */
    unsigned        port_g;         /* 0x90 - port G pins */
    unsigned        lat_g;          /* 0xA0 - port G latched outputs */
    unsigned        odc_g;          /* 0xB0 - port G open drain configuration */

    unsigned        cncon;          /* 0xC0 - interrupt-on-change control */
    unsigned        cnen;           /* 0xD0 - input change interrupt enable */
    unsigned        cnpue;          /* 0xE0 - input pin pull-up enable */
};

extern cpu_mips_t *current_cpu;

/*
 * Perform an assign/clear/set/invert operation.
 */
static inline unsigned write_op (a, b, op)
{
    switch (op & 0xc) {
    case 0x0:           /* Assign */
        a = b;
        break;
    case 0x4:           /* Clear */
        a &= ~b;
        break;
    case 0x8:           /* Set */
        a |= b;
        break;
    case 0xc:           /* Invert */
        a ^= b;
        break;
    }
    return a;
}

void *dev_pic32_gpio_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value)
{
    struct pic32_gpio_data *d = dev->priv_data;

    if (offset >= GPIO_REG_SIZE) {
        printf ("gpio: overhit\n");
        *data = 0;
        return NULL;
    }
    //printf ("gpio: %s offset %#x\n", (op_type == MTS_READ) ? "read" : "write", offset);
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    /*
     * Port A
     */
    case PIC32_TRISA & 0x1f0:             /* Port A: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_a;
        } else {
            d->tris_a = write_op (d->tris_a, *data, offset);
        }
        break;

    case PIC32_PORTA & 0x1f0:             /* Port A: read inputs, write outputs */
        if (op_type == MTS_READ) {
            *data = d->port_a;
        } else {
            goto lat_a;
        }
        break;

    case PIC32_LATA & 0x1f0:              /* Port A: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_a;
        } else {
lat_a:      d->lat_a = write_op (d->lat_a, *data, offset);
#ifdef UBW32
            /* Control SD card 0 */
            if (d->lat_a & MASKA_CS0)
                dev_sdcard_select (cpu, 0, 0);
            else
                dev_sdcard_select (cpu, 0, 1);

            /* Control SD card 1 */
            if (d->lat_a & MASKA_CS1)
                dev_sdcard_select (cpu, 1, 0);
            else
                dev_sdcard_select (cpu, 1, 1);
#endif
        }
        break;

    case PIC32_ODCA & 0x1f0:              /* Port A: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_a;
        } else {
            d->odc_a = write_op (d->odc_a, *data, offset);
        }
        break;

    /*
     * Port B
     */
    case PIC32_TRISB & 0x1f0:             /* Port B: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_b;
        } else {
            d->tris_b = write_op (d->tris_b, *data, offset);
        }
        break;

    case PIC32_PORTB & 0x1f0:             /* Port B: read inputs, write outputs */
        if (op_type == MTS_READ) {
            *data = d->port_b;
        } else {
            goto lat_b;
        }
        break;

    case PIC32_LATB & 0x1f0:              /* Port B: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_b;
        } else {
lat_b:      d->lat_b = write_op (d->lat_b, *data, offset);
#ifdef EXPLORER16
            /* Control SD card 0 */
            if (d->lat_b & MASKB_CS0)
                dev_sdcard_select (cpu, 0, 0);
            else
                dev_sdcard_select (cpu, 0, 1);

            /* Control SD card 1 */
            if (d->lat_b & MASKB_CS1)
                dev_sdcard_select (cpu, 1, 0);
            else
                dev_sdcard_select (cpu, 1, 1);
#endif
        }
        break;

    case PIC32_ODCB & 0x1f0:              /* Port B: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_b;
        } else {
            d->odc_b = write_op (d->odc_b, *data, offset);
        }
        break;

    /*
     * Port C
     */
    case PIC32_TRISC & 0x1f0:             /* Port C: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_c;
        } else {
            d->tris_c = write_op (d->tris_c, *data, offset);
        }
        break;

    case PIC32_PORTC & 0x1f0:             /* Port C: read inputs, write outputs */
        if (op_type == MTS_READ) {
            *data = d->port_c;
        } else {
            goto lat_c;
        }
        break;

    case PIC32_LATC & 0x1f0:              /* Port C: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_c;
        } else {
lat_c:      d->lat_c = write_op (d->lat_c, *data, offset);
#ifdef UBW32
            if (d->lat_c & MASKC_LDADDR)  /* Swap disk: LDADDR */
                dev_swap_ldaddr (cpu, 0);
            else
                dev_swap_ldaddr (cpu, 1);
#endif
        }
        break;

    case PIC32_ODCC & 0x1f0:              /* Port C: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_c;
        } else {
            d->odc_c = write_op (d->odc_c, *data, offset);
        }
        break;

    /*
     * Port D
     */
    case PIC32_TRISD & 0x1f0:             /* Port D: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_d;
        } else {
            d->tris_d = write_op (d->tris_d, *data, offset);
        }
        break;

    case PIC32_PORTD & 0x1f0:             /* Port D: read inputs, write outputs */
#ifndef MAXIMITE
        if (op_type == MTS_READ) {
            *data = d->port_d;
        } else {
            goto lat_d;
        }
#else
        if (op_type == MTS_READ) {
#if 0
            /* Poll PS2 keyboard */
            if (dev_keyboard_clock (cpu))
                d->port_d &= ~MASKD_PS2C;
            else
                d->port_d |= MASKD_PS2C;
            if (dev_keyboard_data (cpu))
                d->port_d &= ~MASKD_PS2D;
            else
                d->port_d |= MASKD_PS2D;
#endif
            *data = d->port_d;
        } else {
            goto lat_d;
        }
#endif
        break;

    case PIC32_LATD & 0x1f0:              /* Port D: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_d;
        } else {
lat_d:      d->lat_d = write_op (d->lat_d, *data, offset);
#ifdef MAX32
            /* Control SD card 0 */
            if (d->lat_d & MASKD_CS0)
                dev_sdcard_select (cpu, 0, 0);
            else
                dev_sdcard_select (cpu, 0, 1);

            /* Control SD card 1 */
            if (d->lat_d & MASKD_CS1)
                dev_sdcard_select (cpu, 1, 0);
            else
                dev_sdcard_select (cpu, 1, 1);
#endif
        }
        break;

    case PIC32_ODCD & 0x1f0:              /* Port D: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_d;
        } else {
            d->odc_d = write_op (d->odc_d, *data, offset);
        }
        break;

    /*
     * Port E
     */
    case PIC32_TRISE & 0x1f0:             /* Port E: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_e;
        } else {
            d->tris_e = write_op (d->tris_e, *data, offset);
        }
        break;

    case PIC32_PORTE & 0x1f0:             /* Port E: read inputs, write outputs */
        if (op_type == MTS_READ) {
#ifdef UBW32
            /* Swap disk: DATA */
            d->port_e &= ~MASKE_DATA;
            d->port_e |= dev_swap_io (cpu, 0, 0xff);
#endif
            *data = d->port_e;
        } else {
            goto lat_e;
        }
        break;

    case PIC32_LATE & 0x1f0:              /* Port E: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_e;
        } else {
lat_e:      d->lat_e = write_op (d->lat_e, *data, offset);
#ifdef MAXIMITE
            /* Control SD card 0 */
            if (d->lat_e & MASKE_CS0)
                dev_sdcard_select (cpu, 0, 0);
            else
                dev_sdcard_select (cpu, 0, 1);
#if 0
            /* Control SD card 1 */
            if (d->lat_e & MASKE_CS1)
                dev_sdcard_select (cpu, 1, 0);
            else
                dev_sdcard_select (cpu, 1, 1);
#endif
#endif
#ifdef UBW32
            if (d->lat_e & MASKE_RD)        /* Swap disk: RD */
                dev_swap_rd (cpu, 0);
            else
                dev_swap_rd (cpu, 1);

            if (d->lat_e & MASKE_WR)        /* Swap disk: WR */
                dev_swap_wr (cpu, 0);
            else
                dev_swap_wr (cpu, 1);

            /* Swap disk: DATA */
            dev_swap_io (cpu, d->lat_e >> SHIFTE_DATA,
                d->tris_e >> SHIFTE_DATA);
#endif
        }
        break;

    case PIC32_ODCE & 0x1f0:              /* Port E: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_e;
        } else {
            d->odc_e = write_op (d->odc_e, *data, offset);
        }
        break;

    /*
     * Port F
     */
    case PIC32_TRISF & 0x1f0:             /* Port F: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_f;
        } else {
            d->tris_f = write_op (d->tris_f, *data, offset);
        }
        break;

    case PIC32_PORTF & 0x1f0:             /* Port F: read inputs, write outputs */
        if (op_type == MTS_READ) {
            *data = d->port_f;
        } else {
            goto lat_f;
        }
        break;

    case PIC32_LATF & 0x1f0:              /* Port F: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_f;
        } else {
lat_f:      d->lat_f = write_op (d->lat_f, *data, offset);
        }
        break;

    case PIC32_ODCF & 0x1f0:              /* Port F: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_f;
        } else {
            d->odc_f = write_op (d->odc_f, *data, offset);
        }
        break;

    /*
     * Port G
     */
    case PIC32_TRISG & 0x1f0:             /* Port G: mask of inputs */
        if (op_type == MTS_READ) {
            *data = d->tris_g;
        } else {
            d->tris_g = write_op (d->tris_g, *data, offset);
        }
        break;

    case PIC32_PORTG & 0x1f0:             /* Port G: read inputs, write outputs */
        if (op_type == MTS_READ) {
            *data = d->port_g;
        } else {
            goto lat_g;
        }
        break;

    case PIC32_LATG & 0x1f0:              /* Port G: read/write outputs */
        if (op_type == MTS_READ) {
            *data = d->lat_g;
        } else {
lat_g:      d->lat_g = write_op (d->lat_g, *data, offset);
        }
        break;

    case PIC32_ODCG & 0x1f0:              /* Port G: open drain configuration */
        if (op_type == MTS_READ) {
            *data = d->odc_g;
        } else {
            d->odc_g = write_op (d->odc_g, *data, offset);
        }
        break;

    /*
     * Change notifier
     */
    case PIC32_CNCON & 0x1f0:             /* Interrupt-on-change control */
        if (op_type == MTS_READ) {
            *data = d->cncon;
        } else {
            d->cncon = write_op (d->cncon, *data, offset);
        }
        break;

    case PIC32_CNEN & 0x1f0:              /* Input change interrupt enable */
        if (op_type == MTS_READ) {
            *data = d->cnen;
        } else {
            d->cnen = write_op (d->cnen, *data, offset);
        }
        break;

    case PIC32_CNPUE & 0x1f0:             /* Input pin pull-up enable */
        if (op_type == MTS_READ) {
            *data = d->cnpue;
        } else {
            d->cnpue = write_op (d->cnpue, *data, offset);
        }
        break;
/* TODO */
//printf ("%s: read data.\n", dev->name);
/* TODO */
//printf ("%s: write %02x.\n", dev->name, d->buf);
    default:
        ASSERT (0, "%s: unknown offset 0x%x\n", dev->name, offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_gpio_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    struct pic32_gpio_data *d = dev->priv_data;

    /* All pins are inputs. */
    d->tris_a = d->tris_b = d->tris_c = d->tris_d =
                d->tris_e = d->tris_f = d->tris_g = 0xFFFF;

    /* All inputs are high. */
    d->port_a = d->port_b = d->port_c = d->port_d =
                d->port_e = d->port_f = d->port_g = 0xFFFF;

    /* All outputs are high. */
    d->lat_a = d->lat_b = d->lat_c = d->lat_d =
                d->lat_e = d->lat_f = d->lat_g = 0xFFFF;

    /* All open drains are disabled. */
    d->odc_a = d->odc_b = d->odc_c = d->odc_d =
                d->odc_e = d->odc_f = d->odc_g = 0;

    /* No interrupts, no pullups. */
    d->cncon = 0;
    d->cnen = 0;
    d->cnpue = 0;
}

int dev_pic32_gpio_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    struct pic32_gpio_data *d;
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    /* allocate the private data structure */
    d = malloc (sizeof (*d));
    if (!d) {
        fprintf (stderr, "PIC32 GPIO: unable to create device.\n");
        return (-1);
    }
    memset (d, 0, sizeof (*d));
    d->dev = dev_create (name);
    if (!d->dev) {
        free (d);
        return (-1);
    }
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = GPIO_REG_SIZE;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->vm = vm;
    d->pic32 = pic32;
    d->dev->handler = dev_pic32_gpio_access;
    d->dev->reset_handler = dev_pic32_gpio_reset;

    vm_bind_device (vm, d->dev);
    return (0);
}
