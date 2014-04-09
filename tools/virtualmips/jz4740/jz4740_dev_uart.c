 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

 /*JZ4740 UART Emulation.
  *
  * JZ4740 UART is compatible with 16c550 .
  *
  * Linux use uart interrupt to receive and send data.
  *
  * For simulator, it is a bad idea for os to use interrupt to send data .
  * Because simulator is always READY for sending, so interrupt is slow than polling.
  *
  *
  * receive:
  * 1.set IER
  * 2. Wating interrupt (read IIR and LSR)
  * 3. if IIR says an interrupt and LSR says that data ready. read RBR.
  *
  * send:
  * 1. set IER to enable transmit request interrupt
  * 2. if UART can send data, generate an interrupt and set IIR LSR
  * 3. linux receives the interrupt ,read IIR and LSR
  * 4. send data.
  *
  */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>

#include "device.h"
#include "mips_memory.h"
#include "jz4740.h"
#include "cpu.h"
#include "vp_timer.h"

/* Interrupt Identification Register */
#define IIR_NPENDING  0x01      /* 0: irq pending, 1: no irq pending */
#define	IIR_TXRDY     0x02
#define	IIR_RXRDY     0x04

struct jz4740_uart_data {
    struct vdevice *dev;

    u_int irq, duart_irq_seq;
    u_int output;

    vtty_t *vtty;
    vm_instance_t *vm;

    m_uint32_t ier;             /*0x04 */
    m_uint32_t iir;             /*0x08 */
    m_uint32_t fcr;             /*0x08 */
    m_uint32_t lcr;             /*0x0c */
    m_uint32_t mcr;             /*0x10 */
    m_uint32_t lsr;             /*0x14 */
    m_uint32_t msr;             /*0x18 */
    m_uint32_t spr;             /*0x1c */
    m_uint32_t isr;             /*0x20 */
    m_uint32_t umr;             /*0x24 */
    m_uint32_t uacr;            /*0x28 */

    m_uint32_t jz4740_uart_size;

    vp_timer_t *uart_timer;

};

static void jz4740_tty_con_input (vtty_t * vtty)
{
    struct jz4740_uart_data *d = vtty->priv_data;

    if (d->ier & UART_IER_RDRIE) {
        d->vm->set_irq (d->vm, d->irq);
    }
    d->lsr |= UART_LSR_DRY;

}

void *dev_jz4740_uart_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{
    struct jz4740_uart_data *d = dev->priv_data;

    u_char odata;

    if (offset >= d->jz4740_uart_size) {
        *data = 0;
        return NULL;
    }

    switch (offset) {
    case UART_RBR:             /*0x0 RBR THR */
        if (op_type == MTS_READ) {

            *data = vtty_get_char (d->vtty);
            if (vtty_is_char_avail (d->vtty))
                d->lsr |= UART_LSR_DRY;
            else
                d->lsr &= ~UART_LSR_DRY;
        } else {
            vtty_put_char (d->vtty, (char) (*data));
            if ((d->ier & UART_IER_TDRIE) && (d->output == 0)
                && (d->fcr & 0x10)) {
                /*yajin.
                 *
                 * In order to put the next data more quickly, just set irq not waiting for host_alarm_handler to set irq.
                 * Sorry uart, too much work for you.
                 *
                 * Sometimes, linux kernel prints "serial8250: too much work for irq9" if we print large data on screen.
                 * Please patch the kernel. comment "printk(KERN_ERR "serial8250: too much work for "
                 * "irq%d\n", irq);"
                 * qemu has some question.
                 * http://lkml.org/lkml/2008/1/12/135
                 * http://kerneltrap.org/mailarchive/linux-kernel/2008/2/7/769924
                 *
                 * If jit is used in future, we may not need to set irq here because simulation is quick enough. Then we have
                 * no "too much work for irq9" problem.
                 *
                 *
                 */
                d->output = TRUE;
                d->vm->set_irq (d->vm, d->irq);
            }

        }

        *has_set_value = TRUE;
        break;

    case UART_IER:             /*0x4 */
        if (op_type == MTS_READ) {
            *data = d->ier;
        } else {
            d->ier = *data & 0xFF;
        }

        *has_set_value = TRUE;
        break;

    case UART_IIR:             /*0x08 */
        d->vm->clear_irq (d->vm, d->irq);
        if (op_type == MTS_READ) {
            odata = IIR_NPENDING;

            if (vtty_is_char_avail (d->vtty)) {
                odata = IIR_RXRDY;
            } else {
                if (d->output) {
                    odata = IIR_TXRDY;
                    d->output = 0;
                }

            }
            *data = odata;
        } else {
            d->fcr = *data;
            if (d->fcr & 0x20)
                d->lsr &= ~UART_LSR_DRY;
        }

        *has_set_value = TRUE;
        break;

    case UART_LSR:             /*0x14 */
        if (op_type == MTS_READ) {
            d->lsr |= UART_LSR_TDRQ | UART_LSR_TEMP;
            if (vtty_is_char_avail (d->vtty))
                d->lsr |= UART_LSR_DRY;
            return &(d->lsr);
        } else
            ASSERT (0, "WRITE TO LSR\n");
        *has_set_value = TRUE;
        break;

    case UART_LCR:
        return &(d->lcr);
    case UART_MCR:
        return &(d->mcr);
    case UART_MSR:
        return &(d->msr);
    case UART_SPR:
        return &(d->spr);
    case UART_ISR:
        return &(d->isr);
    case UART_UMR:
        return &(d->umr);
    case UART_UACR:
        return &(d->uacr);

    default:
        ASSERT (0, "invalid uart offset %x\n", offset);

    }

    return NULL;

}

void dev_jz4740_uart_reset (cpu_mips_t * cpu, struct vdevice *dev)
{

    struct jz4740_uart_data *d = dev->priv_data;
    d->fcr = 0x0;
    d->lcr = 0x0;
    d->mcr = 0x0;
    d->lsr |= UART_LSR_TDRQ | UART_LSR_TEMP;
    d->msr = 0x0;
    d->spr = 0x0;
    d->isr = 0x0;
    d->umr = 0x0;
    d->uacr = 0x0;

}

extern cpu_mips_t *current_cpu;

#define UART_TIME_OUT     25
void dev_jz4740_uart_cb (void *opaque)
{

    struct jz4740_uart_data *d = (struct jz4740_uart_data *) opaque;

    d->output = 0;
    if (vtty_is_char_avail (d->vtty)) {
        d->lsr |= UART_LSR_DRY;
        if (d->ier & UART_IER_RDRIE) {
            d->vm->set_irq (d->vm, d->irq);
            vp_mod_timer (d->uart_timer,
                vp_get_clock (rt_clock) + UART_TIME_OUT);
            return;
        }

    }
    if ((d->ier & UART_IER_TDRIE) && (d->output == 0) && (d->fcr & 0x10)) {
        d->output = TRUE;
        d->vm->set_irq (d->vm, d->irq);
        vp_mod_timer (d->uart_timer, vp_get_clock (rt_clock) + UART_TIME_OUT);
        return;
    }
    // d->uart_timer->set_time=vp_get_clock(rt_clock);
    vp_mod_timer (d->uart_timer, vp_get_clock (rt_clock) + UART_TIME_OUT);

}

int dev_jz4740_uart_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len, u_int irq, vtty_t * vtty)
{
    struct jz4740_uart_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "JZ4740 UART: unable to create device.\n");
        return (-1);
    }
    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->vm = vm;
    (*d).vtty = vtty;
    d->irq = irq;
    vtty->priv_data = d;
    d->jz4740_uart_size = len;
    d->dev->handler = dev_jz4740_uart_access;
    d->dev->reset_handler = dev_jz4740_uart_reset;
    (*d).vtty->read_notifier = jz4740_tty_con_input;
    d->uart_timer = vp_new_timer (rt_clock, dev_jz4740_uart_cb, d);

    //d->uart_timer->set_time=vp_get_clock(rt_clock);
    vp_mod_timer (d->uart_timer, vp_get_clock (rt_clock) + UART_TIME_OUT);

    vm_bind_device (vm, d->dev);

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
