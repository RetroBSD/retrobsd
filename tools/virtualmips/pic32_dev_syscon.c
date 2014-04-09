/*
 * System controller for PIC32.
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

#define SYSCON_REG_SIZE     0x1000

extern cpu_mips_t *current_cpu;

static int syskey_unlock;

static void soft_reset (cpu_mips_t *cpu)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    mips_reset (cpu);
    cpu->pc = pic32->start_address;

    /* reset all devices */
    dev_reset_all (cpu->vm);
    dev_sdcard_reset (cpu);
}

void *dev_pic32_syscon_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    if (offset >= SYSCON_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0xff0) {
    case PIC32_OSCCON & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->osccon;
            if (cpu->vm->debug_level > 2)
                printf ("        read OSCCON -> %08x\n", *data);
        } else {
            pic32->osccon = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        OSCCON := %08x\n", *data);
        }
        break;

    case PIC32_OSCTUN & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->osctun;
            if (cpu->vm->debug_level > 2)
                printf ("        read OSCTUN -> %08x\n", *data);
        } else {
            pic32->osctun = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        OSCTUN := %08x\n", *data);
        }
        break;

    case PIC32_DDPCON & 0xff0:          /* Debug Data Port Control */
        if (op_type == MTS_READ) {
            *data = pic32->ddpcon;
            if (cpu->vm->debug_level > 2)
                printf ("        read DDPCON -> %08x\n", *data);
        } else {
            pic32->ddpcon = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        DDPCON := %08x\n", *data);
        }
        break;

    case PIC32_DEVID & 0xff0:           /* Device identifier */
        /* read-only register */
        if (op_type == MTS_READ) {
            *data = pic32->devid;
            if (cpu->vm->debug_level > 2)
                printf ("        read DEVID -> %08x\n", *data);
        }
        break;

    case PIC32_SYSKEY & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->syskey;
            if (cpu->vm->debug_level > 2)
                printf ("        read SYSKEY -> %08x\n", *data);
        } else {
            pic32->syskey = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        SYSKEY := %08x\n", *data);

            /* Unlock state machine. */
            switch (syskey_unlock) {
            case 0:
                if (pic32->syskey == 0xaa996655)
                    syskey_unlock = 1;
                else
                    syskey_unlock = 0;
                break;
            case 1:
                if (pic32->syskey == 0x556699aa)
                    syskey_unlock = 2;
                else
                    syskey_unlock = 0;
                break;
            default:
                syskey_unlock = 0;
                break;
            }
        }
        break;
    case PIC32_RCON & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->rcon;
            if (cpu->vm->debug_level > 2)
                printf ("        read RCON -> %08x\n", *data);
        } else {
            pic32->rcon = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        RCON := %08x\n", *data);
        }
        break;
    case PIC32_RSWRST & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->rswrst;
            if (cpu->vm->debug_level > 2)
                printf ("        read RSWRST -> %08x\n", *data);
        } else {
            pic32->rswrst = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        RSWRST := %08x\n", *data);

            if (syskey_unlock == 2 && (pic32->rswrst & 1))
                soft_reset (cpu);
        }
        break;

    default:
        ASSERT (0, "unknown syscon offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_syscon_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->osccon = 0x01453320; /* from ubw32 board */
    pic32->osctun = 0;
    pic32->ddpcon = 0;
    pic32->devid = 0x04307053;  /* 795F512L */
    pic32->syskey = 0;
    pic32->rcon = 0;
    pic32->rswrst = 0;
    syskey_unlock = 0;
}

int dev_pic32_syscon_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->sysdev = dev_create (name);
    if (! pic32->sysdev)
        return (-1);
    pic32->sysdev->priv_data = pic32;
    pic32->sysdev->phys_addr = paddr;
    pic32->sysdev->phys_len = SYSCON_REG_SIZE;
    pic32->sysdev->handler = dev_pic32_syscon_access;
    pic32->sysdev->reset_handler = dev_pic32_syscon_reset;
    pic32->sysdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->sysdev);
    return (0);
}
