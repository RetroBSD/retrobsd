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

#define BMXCON_REG_SIZE     0x80

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

void *dev_pic32_bmxcon_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    if (offset >= BMXCON_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    case PIC32_BMXCON & 0x1f0:      /* Interrupt Control */
        if (op_type == MTS_READ) {
            *data = pic32->bmxcon;
            if (cpu->vm->debug_level > 2)
                printf ("        read BMXCON -> %08x\n", *data);
        } else {
            pic32->bmxcon = write_op (pic32->bmxcon, *data, offset);
            if (cpu->vm->debug_level > 2)
                printf ("        BMXCON := %08x\n", pic32->bmxcon);
        }
        break;

    case PIC32_BMXDKPBA & 0x1f0:    /* Data RAM kernel program base address */
        if (op_type == MTS_READ) {
            *data = pic32->bmx_ram_kpba;
        } else {
            pic32->bmx_ram_kpba = write_op (pic32->bmx_ram_kpba, *data, offset);
        }
        break;

    case PIC32_BMXDUDBA & 0x1f0:    /* Data RAM user data base address */
        if (op_type == MTS_READ) {
            *data = pic32->bmx_ram_udba;
        } else {
            pic32->bmx_ram_udba = write_op (pic32->bmx_ram_udba, *data, offset);
        }
        break;

    case PIC32_BMXDUPBA & 0x1f0:    /* Data RAM user program base address */
        if (op_type == MTS_READ) {
            *data = pic32->bmx_ram_upba;
        } else {
            pic32->bmx_ram_upba = write_op (pic32->bmx_ram_upba, *data, offset);
        }
        break;

    case PIC32_BMXPUPBA & 0x1f0:    /* Program Flash user program base address */
        if (op_type == MTS_READ) {
            *data = pic32->bmx_flash_upba;
        } else {
            pic32->bmx_flash_upba = write_op (pic32->bmx_flash_upba, *data, offset);
        }
        break;

    case PIC32_BMXDRMSZ & 0x1f0:    /* Data RAM memory size */
        if (op_type == MTS_READ) {
            *data = cpu->vm->ram_size * 1024;
        }
        break;

    case PIC32_BMXPFMSZ & 0x1f0:    /* Program Flash memory size */
        if (op_type == MTS_READ) {
            *data = cpu->vm->flash_size * 1024;
        }
        break;

    case PIC32_BMXBOOTSZ & 0x1f0:   /* Boot Flash size */
        if (op_type == MTS_READ) {
            *data = pic32->boot_flash_size * 1024;
        }
        break;

    default:
        ASSERT (0, "unknown bmxcon offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_bmxcon_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->bmxcon = 0x001f0041;
    pic32->bmx_ram_kpba = 0;
    pic32->bmx_ram_udba = 0;
    pic32->bmx_ram_upba = 0;
    pic32->bmx_flash_upba = 0;
}

int dev_pic32_bmxcon_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->bmxdev = dev_create (name);
    if (! pic32->bmxdev)
        return (-1);
    pic32->bmxdev->priv_data = pic32;
    pic32->bmxdev->phys_addr = paddr;
    pic32->bmxdev->phys_len = BMXCON_REG_SIZE;
    pic32->bmxdev->handler = dev_pic32_bmxcon_access;
    pic32->bmxdev->reset_handler = dev_pic32_bmxcon_reset;
    pic32->bmxdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->bmxdev);
    return (0);
}
