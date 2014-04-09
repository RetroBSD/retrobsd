 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
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
#include "adm5120.h"
#include "cpu.h"

m_uint32_t uart_table[2][UART_INDEX_MAX];

struct uart_data {
    struct vdevice *dev;
    m_uint8_t *uart_ptr;
    m_uint32_t uart_size;
    vtty_t *vtty[2];

};
#define ADM5120_UART_IRQ0		1
#define ADM5120_UART_IRQ1		2

void uart_set_interrupt (cpu_mips_t * cpu, int channel)
{
    if (channel == 0)
        cpu->vm->set_irq (cpu->vm, INT_LVL_UART0);
    else if (channel == 1)
        cpu->vm->set_irq (cpu->vm, INT_LVL_UART1);
    return;
    assert (0);
}

void uart_clear_interrupt (cpu_mips_t * cpu, int channel)
{
    if (channel == 0)
        cpu->vm->clear_irq (cpu->vm, INT_LVL_UART0);
    else if (channel == 1)
        cpu->vm->clear_irq (cpu->vm, INT_LVL_UART1);
    return;
    assert (0);
}

/* Console port input */
static void tty_con0_input (vtty_t * vtty)
{

    uart_table[0][UART_FR_REG / 4] &= ~UART_RX_FIFO_EMPTY;
    if (vtty_is_full (vtty))
        uart_table[0][UART_FR_REG / 4] |= UART_RX_FIFO_FULL;
    if ((uart_table[0][UART_CR_REG / 4] & UART_RX_INT_EN)
        && (uart_table[0][UART_CR_REG / 4] & UART_PORT_EN)) {
        uart_table[0][UART_ICR_REG / 4] |= UART_RX_INT;
        uart_set_interrupt (vtty->vm->boot_cpu, 0);
    }

}

/* Console port input */
static void tty_con1_input (vtty_t * vtty)
{

    uart_table[1][UART_FR_REG / 4] &= ~UART_RX_FIFO_EMPTY;
    if (vtty_is_full (vtty))
        uart_table[1][UART_FR_REG / 4] |= UART_RX_FIFO_FULL;
    if ((uart_table[1][UART_CR_REG / 4] & UART_RX_INT_EN)
        && (uart_table[1][UART_CR_REG / 4] & UART_PORT_EN)) {
        uart_set_interrupt (vtty->vm->boot_cpu, 1);
        uart_table[1][UART_ICR_REG / 4] |= UART_RX_INT;
    }

}

void *dev_uart_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value, m_uint8_t channel)
{

    struct uart_data *d = dev->priv_data;

    if (offset >= d->uart_size) {
        *data = 0;
        return NULL;
    }
    switch (offset) {
    case (UART_DR_REG):
        if (!(uart_table[channel][UART_CR_REG / 4] & UART_PORT_EN)) {
            //uart port is disabled.
            if (op_type == MTS_READ) {
                *data = vmtoh32 (0xffffffff);
            }
            *has_set_value = TRUE;
            return NULL;
        }

        if (op_type == MTS_READ) {

            if (vtty_is_char_avail (d->vtty[channel])) {
                *data = vtty_get_char (d->vtty[channel]);

                uart_table[channel][UART_RSR_REG / 4] = 0;

            } else
                *data = vmtoh32 (0xffffffff);
            if (vtty_is_char_avail (d->vtty[channel])) {
                uart_table[channel][UART_FR_REG / 4] &= ~UART_RX_FIFO_EMPTY;
                uart_table[channel][UART_FR_REG / 4] |= UART_RX_FIFO_FULL;
                if ((uart_table[channel][UART_CR_REG / 4] & UART_RX_INT_EN)
                    && (uart_table[channel][UART_CR_REG / 4] & UART_PORT_EN)) {
                    uart_table[channel][UART_ICR_REG / 4] |= UART_RX_INT;
                    uart_set_interrupt (cpu, channel);
                }

            } else {
                uart_table[channel][UART_FR_REG / 4] |= UART_RX_FIFO_EMPTY;
                uart_table[channel][UART_FR_REG / 4] &= ~UART_RX_FIFO_FULL;
                if ((uart_table[channel][UART_CR_REG / 4] & UART_RX_INT_EN)
                    && (uart_table[channel][UART_CR_REG / 4] & UART_PORT_EN)) {
                    uart_table[channel][UART_ICR_REG / 4] &= ~UART_RX_INT;
                    uart_clear_interrupt (cpu, channel);
                }

            }

            *has_set_value = TRUE;

        } else if (op_type == MTS_WRITE) {

            vtty_put_char (d->vtty[channel], (char) *data);
            *has_set_value = TRUE;
        } else {
            assert (0);
        }
        return NULL;
        break;
    case UART_RSR_REG:
        if (op_type == MTS_WRITE) {
            uart_table[channel][UART_RSR_REG / 4] = 0;
            *has_set_value = TRUE;
        }
        break;
    case UART_CR_REG:
        if (op_type == MTS_WRITE) {
            //enable UART
            if ((*data) & UART_PORT_EN) {

                if (*data & UART_TX_INT_EN) {
                    //START TX
                    uart_table[channel][UART_ICR_REG / 4] |= UART_TX_INT;
                    uart_set_interrupt (cpu, channel);

                } else {
                    //TX interrupt dissabled
                    uart_table[channel][UART_ICR_REG / 4] &= ~UART_TX_INT;
                    uart_clear_interrupt (cpu, channel);
                }
                if (*data & UART_RX_INT_EN) {
                    if (vtty_is_char_avail (d->vtty[channel])) {
                        //set RX interrupt
                        uart_table[channel][UART_ICR_REG / 4] |= UART_RX_INT;
                        uart_set_interrupt (cpu, channel);
                    }

                } else {
                    //disable RX interrupt
                    uart_table[channel][UART_ICR_REG / 4] &= ~UART_RX_INT;
                    uart_clear_interrupt (cpu, channel);
                }
            } else {
                //disable UART
                //clear rx and tx interrupt
                uart_table[channel][UART_ICR_REG / 4] &= ~UART_TX_INT;
                uart_clear_interrupt (cpu, channel);
                uart_table[channel][UART_ICR_REG / 4] &= ~UART_RX_INT;
                uart_clear_interrupt (cpu, channel);
            }
        }

        break;

    }
    return ((void *) (d->uart_ptr + offset));

}

void dev_uart_init_defaultvalue (int uart_index)
{
    uart_table[uart_index][UART_FR_REG / 4] = 0x90;
    uart_table[uart_index][UART_RSR_REG / 4] = 0;

}

void *dev_uart_access0 (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{
    return dev_uart_access (cpu, dev, offset, op_size, op_type, data,
        has_set_value, 0);
}

void *dev_uart_access1 (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{
    return dev_uart_access (cpu, dev, offset, op_size, op_type, data,
        has_set_value, 1);
}

int dev_uart_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len, vtty_t * vtty, int uart_index)
{
    struct uart_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "UART: unable to create device.\n");
        return (-1);
    }
    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    (*d).vtty[uart_index] = vtty;
    d->uart_size = len;
    if (uart_index == 0) {
        d->dev->handler = dev_uart_access0;
        (*d).vtty[uart_index]->read_notifier = tty_con0_input;

    } else {
        d->dev->handler = dev_uart_access1;
        (*d).vtty[uart_index]->read_notifier = tty_con1_input;
    }

    d->uart_ptr = (m_uint8_t *) (m_iptr_t) (&uart_table[uart_index]);

    vm_bind_device (vm, d->dev);
    dev_uart_init_defaultvalue (uart_index);
    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
