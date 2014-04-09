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

#define RTCC_REG_SIZE     0x80

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

void *dev_pic32_rtcc_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    if (offset >= RTCC_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    case PIC32_RTCCON & 0x1f0:      /* RTC Control */
        if (op_type == MTS_READ) {
            *data = pic32->rtccon;
            if (cpu->vm->debug_level > 2)
                printf ("        read RTCCON -> %08x\n", *data);
        } else {
            pic32->rtccon = write_op (pic32->rtccon, *data, offset);
            if (cpu->vm->debug_level > 2)
                printf ("        RTCCON := %08x\n", pic32->rtccon);
        }
        break;

    case PIC32_RTCALRM & 0x1f0:     /* RTC alarm control */
        if (op_type == MTS_READ) {
            *data = 0;
        } else {
            //pic32->rtcalrm = write_op (pic32->rtcalrm, *data, offset);
        }
        break;

    case PIC32_RTCTIME & 0x1f0:     /* RTC time value */
        if (op_type == MTS_READ) {
            *data = 0;
        } else {
            //pic32->rtctime = write_op (pic32->rtctime, *data, offset);
        }
        break;

    case PIC32_RTCDATE & 0x1f0:     /* RTC date value */
        if (op_type == MTS_READ) {
            *data = 0;
        } else {
            //pic32->rtcdate = write_op (pic32->rtcdate, *data, offset);
        }
        break;

    case PIC32_ALRMTIME & 0x1f0:    /* Alarm time value */
        if (op_type == MTS_READ) {
            *data = 0;
        } else {
            //pic32->alrmtime = write_op (pic32->alrmtime, *data, offset);
        }
        break;

    case PIC32_ALRMDATE & 0x1f0:    /* Alarm date value */
        if (op_type == MTS_READ) {
            *data = 0;
        } else {
            //pic32->alrmdate = write_op (pic32->alrmdate, *data, offset);
        }
        break;

    default:
        ASSERT (0, "unknown rtcc offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_rtcc_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->rtccon = 0;
}

int dev_pic32_rtcc_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->rtcdev = dev_create (name);
    if (! pic32->rtcdev)
        return (-1);
    pic32->rtcdev->priv_data = pic32;
    pic32->rtcdev->phys_addr = paddr;
    pic32->rtcdev->phys_len = RTCC_REG_SIZE;
    pic32->rtcdev->handler = dev_pic32_rtcc_access;
    pic32->rtcdev->reset_handler = dev_pic32_rtcc_reset;
    pic32->rtcdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->rtcdev);
    return (0);
}
