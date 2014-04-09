/*
 * UART emulation for PIC32.
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

#define UART_REG_SIZE     0x50
#define UART_TIME_OUT     1

struct pic32_uart_data {
    struct vdevice  *dev;
    vtty_t          *vtty;
    vm_instance_t   *vm;
    vp_timer_t      *uart_timer;

    u_int           output;
    u_int           irq;            /* base irq number */
#define IRQ_ERR     0               /* error interrupt */
#define IRQ_RX      1               /* receiver interrupt */
#define IRQ_TX      2               /* transmitter interrupt */

    m_uint32_t      mode;           /* 0x00 - mode */
    m_uint32_t      sta;            /* 0x10 - status and control */
    m_uint32_t      txreg;          /* 0x20 - transmit */
    m_uint32_t      rxreg;          /* 0x30 - receive */
    m_uint32_t      brg;            /* 0x40 - baud rate */
};

extern cpu_mips_t *current_cpu;

static void pic32_tty_con_input (vtty_t * vtty)
{
    struct pic32_uart_data *d = vtty->priv_data;

    if (d->mode & PIC32_UMODE_ON) {
        /* UART enabled. */
        if (d->sta & PIC32_USTA_URXEN) {
            /* Receiver enabled - activate interrupt. */
            d->vm->set_irq (d->vm, d->irq + IRQ_RX);

            /* Receive data available */
            d->sta |= PIC32_USTA_URXDA;
        }
    }
}

void *dev_pic32_uart_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value)
{
    struct pic32_uart_data *d = dev->priv_data;
    unsigned newval;

    if (offset >= UART_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ) {
        /*
         * Reading UART registers.
         */
        switch (offset) {
        case PIC32_U1RXREG & 0xff:              /* Receive data */
            *data = vtty_get_char (d->vtty);
            if (vtty_is_char_avail (d->vtty)) {
                d->sta |= PIC32_USTA_URXDA;
            } else {
                d->sta &= ~PIC32_USTA_URXDA;
                d->vm->clear_irq (d->vm, d->irq + IRQ_RX);
            }
            break;

        case PIC32_U1BRG & 0xff:                /* Baud rate */
            *data = d->brg;
            break;

        case PIC32_U1MODE & 0xff:               /* Mode */
            *data = d->mode;
            break;

        case PIC32_U1STA & 0xff:                /* Status and control */
            d->sta |= PIC32_USTA_RIDLE |        /* Receiver is idle */
                PIC32_USTA_TRMT;                /* Transmit shift register is empty */
            if (vtty_is_char_avail (d->vtty))
                d->sta |= PIC32_USTA_URXDA;
            *data = d->sta;
#if 0
            printf ("<%x>", d->sta);
            fflush (stdout);
#endif
            break;

        case PIC32_U1TXREG & 0xff:              /* Transmit */
        case PIC32_U1MODECLR & 0xff:
        case PIC32_U1MODESET & 0xff:
        case PIC32_U1MODEINV & 0xff:
        case PIC32_U1STACLR & 0xff:
        case PIC32_U1STASET & 0xff:
        case PIC32_U1STAINV & 0xff:
        case PIC32_U1BRGCLR & 0xff:
        case PIC32_U1BRGSET & 0xff:
        case PIC32_U1BRGINV & 0xff:
            *data = 0;
            break;

        default:
            ASSERT (0, "reading unknown uart offset %x\n", offset);
        }
        *has_set_value = TRUE;
#if 0
        printf ("--- uart: read %02x -> %08x\n", offset, *data);
        fflush (stdout);
#endif
    } else {
        /*
         * Writing UART registers.
         */
#if 0
        printf ("--- uart: write %02x := %08x\n", offset, *data);
        fflush (stdout);
#endif
        switch (offset) {
        case PIC32_U1TXREG & 0xff:              /* Transmit */
            /* Skip ^M. */
            if ((char) (*data) != '\r')
                vtty_put_char (d->vtty, (char) (*data));
            if ((d->mode & PIC32_UMODE_ON) &&
                (d->sta & PIC32_USTA_UTXEN) && (d->output == 0)) {
                /*
                 * yajin.
                 *
                 * In order to put the next data more quickly,
                 * just set irq not waiting for
                 * host_alarm_handler to set irq. Sorry uart,
                 * too much work for you.
                 *
                 * Sometimes, linux kernel prints "serial8250:
                 * too much work for irq9" if we print large
                 * data on screen. Please patch the kernel.
                 * comment "printk(KERN_ERR "serial8250: too
                 * much work for " "irq%d\n", irq);" qemu has
                 * some question.
                 * http://lkml.org/lkml/2008/1/12/135
                 * http://kerneltrap.org/mailarchive/linux-ker
                 * nel/2008/2/7/769924
                 *
                 * If jit is used in future, we may not need to
                 * set irq here because simulation is quick
                 * enough. Then we have no "too much work for
                 * irq9" problem.
                 */
                d->output = TRUE;
                d->vm->set_irq (d->vm, d->irq + IRQ_TX);
            }
            break;

        case PIC32_U1MODE & 0xff:               /* Mode */
            newval = *data;
write_mode:
            d->mode = newval;
            if (!(d->mode & PIC32_UMODE_ON)) {
                d->vm->clear_irq (d->vm, d->irq + IRQ_RX);
                d->vm->clear_irq (d->vm, d->irq + IRQ_TX);
                d->sta &= ~PIC32_USTA_URXDA;
                d->sta &= ~(PIC32_USTA_URXDA | PIC32_USTA_FERR |
                    PIC32_USTA_PERR | PIC32_USTA_UTXBF);
                d->sta |= PIC32_USTA_RIDLE | PIC32_USTA_TRMT;
            }
            break;
        case PIC32_U1MODECLR & 0xff:
            newval = d->mode & ~*data;
            goto write_mode;
        case PIC32_U1MODESET & 0xff:
            newval = d->mode | *data;
            goto write_mode;
        case PIC32_U1MODEINV & 0xff:
            newval = d->mode ^ *data;
            goto write_mode;

        case PIC32_U1STA & 0xff:                /* Status and control */
            newval = *data;
write_sta:
            d->sta &= PIC32_USTA_URXDA | PIC32_USTA_FERR |
                PIC32_USTA_PERR | PIC32_USTA_RIDLE |
                PIC32_USTA_TRMT | PIC32_USTA_UTXBF;
            d->sta |= newval & ~(PIC32_USTA_URXDA | PIC32_USTA_FERR |
                PIC32_USTA_PERR | PIC32_USTA_RIDLE |
                PIC32_USTA_TRMT | PIC32_USTA_UTXBF);
            if (!(d->sta & PIC32_USTA_URXEN)) {
                d->vm->clear_irq (d->vm, d->irq + IRQ_RX);
                d->sta &= ~(PIC32_USTA_URXDA | PIC32_USTA_FERR |
                    PIC32_USTA_PERR);
            }
            if (!(d->sta & PIC32_USTA_UTXEN)) {
                d->vm->clear_irq (d->vm, d->irq + IRQ_TX);
                d->sta &= ~PIC32_USTA_UTXBF;
                d->sta |= PIC32_USTA_TRMT;
            }
            break;
        case PIC32_U1STACLR & 0xff:
            newval = d->sta & ~*data;
            goto write_sta;
        case PIC32_U1STASET & 0xff:
            newval = d->sta | *data;
            goto write_sta;
        case PIC32_U1STAINV & 0xff:
            newval = d->sta ^ *data;
            goto write_sta;

        case PIC32_U1BRG & 0xff:                /* Baud rate */
            newval = *data;
write_brg:
            d->brg = newval;
            break;
        case PIC32_U1BRGCLR & 0xff:
            newval = d->brg & ~*data;
            goto write_brg;
        case PIC32_U1BRGSET & 0xff:
            newval = d->brg | *data;
            goto write_brg;
        case PIC32_U1BRGINV & 0xff:
            newval = d->brg ^ *data;
            goto write_brg;

        case PIC32_U1RXREG & 0xff:              /* Receive */
            /* Ignore */
            break;

        default:
            ASSERT (0, "writing unknown uart offset %x\n", offset);
        }
        *has_set_value = TRUE;
    }
    return NULL;
}

void dev_pic32_uart_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    struct pic32_uart_data *d = dev->priv_data;

    d->mode = 0;
    d->sta = PIC32_USTA_RIDLE |         /* Receiver is idle */
        PIC32_USTA_TRMT;                /* Transmit shift register is empty */
    d->txreg = 0;
    d->rxreg = 0;
    d->brg = 0;
}

void dev_pic32_uart_cb (void *opaque)
{
    struct pic32_uart_data *d = (struct pic32_uart_data *) opaque;

    d->output = 0;
    if (d->mode & PIC32_UMODE_ON) {
        /* UART enabled. */
        if ((d->sta & PIC32_USTA_URXEN) && vtty_is_char_avail (d->vtty)) {
            /* Receive data available */
            d->sta |= PIC32_USTA_URXDA;

            /* Activate receive interrupt. */
            d->vm->set_irq (d->vm, d->irq + IRQ_RX);
            vp_mod_timer (d->uart_timer,
                vp_get_clock (rt_clock) + UART_TIME_OUT);
            return;
        }
        if ((d->sta & PIC32_USTA_UTXEN) && (d->output == 0)) {
            /* Activate transmit interrupt. */
            d->output = TRUE;
            d->vm->set_irq (d->vm, d->irq + IRQ_TX);
            vp_mod_timer (d->uart_timer,
                vp_get_clock (rt_clock) + UART_TIME_OUT);
            return;
        }
    }
    vp_mod_timer (d->uart_timer, vp_get_clock (rt_clock) + UART_TIME_OUT);
}

int dev_pic32_uart_init (vm_instance_t *vm, char *name, unsigned paddr,
    unsigned irq, vtty_t *vtty)
{
    struct pic32_uart_data *d;

    /* allocate the private data structure */
    d = malloc (sizeof (*d));
    if (!d) {
        fprintf (stderr, "PIC32 UART: unable to create device.\n");
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
    d->dev->phys_len = UART_REG_SIZE;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->vm = vm;
    (*d).vtty = vtty;
    d->irq = irq;
    vtty->priv_data = d;
    d->dev->handler = dev_pic32_uart_access;
    d->dev->reset_handler = dev_pic32_uart_reset;
    (*d).vtty->read_notifier = pic32_tty_con_input;
    d->uart_timer = vp_new_timer (rt_clock, dev_pic32_uart_cb, d);

    vp_mod_timer (d->uart_timer, vp_get_clock (rt_clock) + UART_TIME_OUT);

    vm_bind_device (vm, d->dev);
    return (0);
}
