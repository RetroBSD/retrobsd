/*
 * DMA controller for PIC32.
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

#define DMACON_REG_SIZE     0x660

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

void *dev_pic32_dmacon_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    if (offset >= DMACON_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    case PIC32_DMACON & 0x1f0:      /* DMA Control */
        if (op_type == MTS_READ) {
            *data = pic32->dmacon;
            if (cpu->vm->debug_level > 2)
                printf ("        read DMACON -> %08x\n", *data);
        } else {
            pic32->dmacon = write_op (pic32->dmacon, *data, offset);
            if (cpu->vm->debug_level > 2)
                printf ("        DMACON := %08x\n", pic32->dmacon);
        }
        break;

    case PIC32_DMASTAT:             /* DMA Status */
        if (op_type == MTS_READ) {
            *data = pic32->dmastat;
            if (cpu->vm->debug_level > 2)
                printf ("        read DMASTAT -> %08x\n", *data);
        }
        break;

    case PIC32_DMAADDR:             /* DMA address */
        if (op_type == MTS_READ) {
            *data = pic32->dmaaddr;
            if (cpu->vm->debug_level > 2)
                printf ("        read DMAADDR -> %08x\n", *data);
        } else {
            pic32->dmaaddr = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        DMAADDR := %08x\n", *data);
        }
        break;

    // TODO: other registers.

    default:
        ASSERT (0, "unknown dmacon offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_dmacon_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->dmacon = 0;
    pic32->dmastat = 0;
    pic32->dmaaddr = 0;
}

int dev_pic32_dmacon_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->dmadev = dev_create (name);
    if (! pic32->dmadev)
        return (-1);
    pic32->dmadev->priv_data = pic32;
    pic32->dmadev->phys_addr = paddr;
    pic32->dmadev->phys_len = DMACON_REG_SIZE;
    pic32->dmadev->handler = dev_pic32_dmacon_access;
    pic32->dmadev->reset_handler = dev_pic32_dmacon_reset;
    pic32->dmadev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->dmadev);
    return (0);
}
