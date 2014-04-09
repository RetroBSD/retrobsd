/*
 * Timer emulation for PIC32.
 *
 * Copyright (C) 2012 Serge Vakulenko <serge@vak.ru>
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
#include "dev_sdcard.h"

#define TIMER_REG_SIZE    0x30

struct pic32_timer_data {
    struct vdevice  *dev;
    vm_instance_t   *vm;
    pic32_t         *pic32;

    unsigned        irq;            /* irq number */
    unsigned        con;            /* 0x00 - Control */
    unsigned        count;          /* 0x10 - Count */
    unsigned        period;         /* 0x20 - Period */
    unsigned        scale;          /* prescale value */
};

extern cpu_mips_t *current_cpu;

static const int timer_scale[8] = {
    1,  2,  4,  8,  16, 32, 64, 256,
};

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

void *dev_pic32_timer_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value)
{
    struct pic32_timer_data *d = dev->priv_data;
    unsigned newval;

    if (offset >= TIMER_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    case PIC32_T1CON & 0x1f0:           /* Timer control */
         if (op_type == MTS_READ) {
            *data = d->con;
fprintf (stderr, "%s read TCON -> %04x\n", dev->name, d->con);
        } else {
            d->con = write_op (d->con, *data, offset);
            d->scale = timer_scale [(d->con >> 4) & 7];
fprintf (stderr, "%s write TCON %04x\n", dev->name, d->con);
            if (! (d->con & PIC32_TCON_ON)) {
                d->vm->clear_irq (d->vm, d->irq);
                d->count = 0;
            }
        }
        break;

    case PIC32_TMR1 & 0x1f0:            /* Timer count */
        if (op_type == MTS_READ) {
            *data = d->count / d->scale;
fprintf (stderr, "%s read TMR -> %04x\n", dev->name, d->count / d->scale);
        } else {
            newval = write_op (d->count / d->scale, *data, offset);
fprintf (stderr, "%s write TMR %04x\n", dev->name, newval);
            d->count = newval * d->scale;
        }
        break;

    case PIC32_PR1 & 0x1ff:             /* Timer period */
        if (op_type == MTS_READ) {
            *data = d->period;
fprintf (stderr, "%s read PR -> %04x\n", dev->name, d->period);
            //d->vm->clear_irq (d->vm, d->irq);
        } else {
            newval = write_op (d->period, *data, offset);
fprintf (stderr, "%s write PR %04x\n", dev->name, newval);
            d->period = newval;
            //d->vm->set_irq (d->vm, d->irq);
        }
        break;

    default:
        ASSERT (0, "unknown timer offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

/*
 * Increment timer counter.
 * Fire periodic interrupt.
 */
void dev_pic32_timer_tick (cpu_mips_t *cpu, struct vdevice *dev, unsigned nclocks)
{
    struct pic32_timer_data *d = dev->priv_data;

    /* Check that timer is enabled. */
    if (! (d->con & PIC32_TCON_ON) || d->period == 0)
        return;

    /* Update counter and check overflow. */
    d->count += nclocks;
    if (d->count < d->period * d->scale)
        return;

    /* Counter matched. */
    d->count %= d->period * d->scale;
    pic32_set_irq (cpu->vm, d->irq);
fprintf (stderr, "%s irq %u\n", dev->name, d->irq);
}

void dev_pic32_timer_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    struct pic32_timer_data *d = dev->priv_data;

    d->con = 0;
    d->count = 0;
    d->period = 0;
    d->scale = 1;
    pic32_clear_irq (cpu->vm, d->irq);
}

struct vdevice *dev_pic32_timer_init (vm_instance_t *vm, char *name,
    unsigned paddr, unsigned irq)
{
    struct pic32_timer_data *d;
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    /* allocate the private data structure */
    d = malloc (sizeof (*d));
    if (!d) {
        fprintf (stderr, "PIC32 timer: unable to create device.\n");
        return 0;
    }
    memset (d, 0, sizeof (*d));
    d->dev = dev_create (name);
    if (! d->dev) {
        free (d);
        return 0;
    }
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = TIMER_REG_SIZE;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->vm = vm;
    d->irq = irq;
    d->pic32 = pic32;
    d->dev->handler = dev_pic32_timer_access;
    d->dev->reset_handler = dev_pic32_timer_reset;

    vm_bind_device (vm, d->dev);
    return d->dev;
}
