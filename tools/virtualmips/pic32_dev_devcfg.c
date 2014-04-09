/*
 * Device configuration registers for PIC32.
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
#include "cpu.h"
#include "pic32.h"

#define DEVCFG_REG_SIZE     0x100

void *dev_pic32_devcfg_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    if (offset >= DEVCFG_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0xfc) {
    case 0xf0:                          /* DEVCFG3 */
        if (op_type == MTS_READ) {
            *data = pic32->devcfg3;
            if (cpu->vm->debug_level > 2)
                printf ("        read DEVCFG3 -> %08x\n", *data);
        }
        break;
    case 0xf4:                          /* DEVCFG2 */
        if (op_type == MTS_READ) {
            *data = pic32->devcfg2;
            if (cpu->vm->debug_level > 2)
                printf ("        read DEVCFG3 -> %08x\n", *data);
        }
        break;
    case 0xf8:                          /* DEVCFG1 */
        if (op_type == MTS_READ) {
            *data = pic32->devcfg1;
            if (cpu->vm->debug_level > 2)
                printf ("        read DEVCFG3 -> %08x\n", *data);
        }
        break;
    case 0xfc:                          /* DEVCFG0 */
        if (op_type == MTS_READ) {
            *data = pic32->devcfg0;
            if (cpu->vm->debug_level > 2)
                printf ("        read DEVCFG3 -> %08x\n", *data);
        }
        break;

    // TODO: other registers.

    default:
        ASSERT (0, "unknown devcfg offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_devcfg_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->devcfg3 = 0xffff0722;    // From Max32 bootloader
    pic32->devcfg2 = 0xd979f8f9;
    pic32->devcfg1 = 0x5bfd6aff;
    pic32->devcfg0 = 0xffffff7f;
}

int dev_pic32_devcfg_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->cfgdev = dev_create (name);
    if (! pic32->cfgdev)
        return (-1);
    pic32->cfgdev->priv_data = pic32;
    pic32->cfgdev->phys_addr = paddr;
    pic32->cfgdev->phys_len = DEVCFG_REG_SIZE;
    pic32->cfgdev->handler = dev_pic32_devcfg_access;
    pic32->cfgdev->reset_handler = dev_pic32_devcfg_reset;
    pic32->cfgdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->cfgdev);
    return (0);
}
