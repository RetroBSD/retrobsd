/*
 * Interrupt controller for PIC32.
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
#include "cpu.h"
#include "pic32.h"

#define INTCON_REG_SIZE     0x190

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

void *dev_pic32_intcon_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;
    unsigned n, v;

    if (offset >= INTCON_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    case PIC32_INTCON & 0x1f0:      /* Interrupt Control */
        if (op_type == MTS_READ) {
            *data = pic32->intcon;
        } else {
            pic32->intcon = write_op (pic32->intcon, *data, offset);
        }
        break;

    case PIC32_INTSTAT & 0x1f0:     /* Interrupt Status */
        if (op_type == MTS_READ) {
            *data = pic32->intstat;
        }
        break;

    case PIC32_IPTMR & 0x1f0:		/* Temporal Proximity Timer */
        if (op_type == MTS_READ) {
            *data = pic32->iptmr;
        } else {
            pic32->iptmr = write_op (pic32->iptmr, *data, offset);
        }
        break;

    case PIC32_IFS(0) & 0x1f0:      /* IFS(0..2) - Interrupt Flag Status */
    case PIC32_IFS(1) & 0x1f0:
    case PIC32_IFS(2) & 0x1f0:
        n = (offset - (PIC32_IFS(0) & 0x1f0)) >> 4;
        if (op_type == MTS_READ) {
            *data = pic32->ifs[n];
        } else {
            pic32->ifs[n] = write_op (pic32->ifs[n], *data, offset);
            pic32_update_irq_flag (pic32);
        }
        break;

    case PIC32_IEC(0) & 0x1f0:      /* IEC(0..2) - Interrupt Enable Control */
    case PIC32_IEC(1) & 0x1f0:
    case PIC32_IEC(2) & 0x1f0:
        n = (offset - (PIC32_IEC(0) & 0x1f0)) >> 4;
        if (op_type == MTS_READ) {
            *data = pic32->iec[n];
        } else {
            pic32->iec[n] = write_op (pic32->iec[n], *data, offset);
            pic32_update_irq_flag (pic32);
        }
        break;

    case PIC32_IPC(0) & 0x1f0:      /* IPC(0..11) - Interrupt Priority Control */
    case PIC32_IPC(1) & 0x1f0:
    case PIC32_IPC(2) & 0x1f0:
    case PIC32_IPC(3) & 0x1f0:
    case PIC32_IPC(4) & 0x1f0:
    case PIC32_IPC(5) & 0x1f0:
    case PIC32_IPC(6) & 0x1f0:
    case PIC32_IPC(7) & 0x1f0:
    case PIC32_IPC(8) & 0x1f0:
    case PIC32_IPC(9) & 0x1f0:
    case PIC32_IPC(10) & 0x1f0:
    case PIC32_IPC(11) & 0x1f0:
    case PIC32_IPC(12) & 0x1f0:
        n = (offset - (PIC32_IPC(0) & 0x1f0)) >> 4;
        if (op_type == MTS_READ) {
            *data = pic32->ipc[n];
        } else {
            pic32->ipc[n] = write_op (pic32->ipc[n], *data, offset);
            v = n << 2;
            pic32->ivprio[v]   = pic32->ipc[n] >> 2 & 63;
            pic32->ivprio[v+1] = pic32->ipc[n] >> 10 & 63;
            pic32->ivprio[v+2] = pic32->ipc[n] >> 18 & 63;
            pic32->ivprio[v+3] = pic32->ipc[n] >> 26 & 63;
            pic32_update_irq_flag (pic32);
        }
        break;

    default:
        ASSERT (0, "unknown intcon offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_intcon_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->intcon = 0;
    pic32->intstat = 0;
    pic32->iptmr = 0;
    memset (pic32->ifs, 0, sizeof (pic32->ifs));
    memset (pic32->iec, 0, sizeof (pic32->iec));
    memset (pic32->ipc, 0, sizeof (pic32->ipc));
    memset (pic32->ivprio, 0, sizeof (pic32->ivprio));

    cpu->irq_cause = 0;
    cpu->irq_pending = 0;
}

int dev_pic32_intcon_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->intdev = dev_create (name);
    if (! pic32->intdev)
        return (-1);
    pic32->intdev->priv_data = pic32;
    pic32->intdev->phys_addr = paddr;
    pic32->intdev->phys_len = INTCON_REG_SIZE;
    pic32->intdev->handler = dev_pic32_intcon_access;
    pic32->intdev->reset_handler = dev_pic32_intcon_reset;
    pic32->intdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->intdev);
    return (0);
}
