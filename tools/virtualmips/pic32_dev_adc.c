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

#define ADC_REG_SIZE     0x170

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

void *dev_pic32_adc_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    if (offset >= ADC_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0xff0) {
    case PIC32_AD1CON1 & 0xff0:         /* Control register 1 */
        if (op_type == MTS_READ) {
            *data = pic32->ad1con1;
        } else {
            pic32->ad1con1 = write_op (pic32->ad1con1, *data, offset);
        }
        break;

    case PIC32_AD1CON2 & 0xff0:         /* Control register 2 */
        if (op_type == MTS_READ) {
            *data = pic32->ad1con2;
        } else {
            pic32->ad1con2 = write_op (pic32->ad1con2, *data, offset);
        }
        break;

    case PIC32_AD1CON3 & 0xff0:         /* Control register 3 */
        if (op_type == MTS_READ) {
            *data = pic32->ad1con3;
        } else {
            pic32->ad1con3 = write_op (pic32->ad1con3, *data, offset);
        }
        break;

    case PIC32_AD1CHS & 0xff0:          /* Channel select */
        if (op_type == MTS_READ) {
            *data = pic32->ad1chs;
        } else {
            pic32->ad1chs = write_op (pic32->ad1chs, *data, offset);
        }
        break;

    case PIC32_AD1CSSL & 0xff0:         /* Input scan selection */
        if (op_type == MTS_READ) {
            *data = pic32->ad1cssl;
        } else {
            pic32->ad1cssl = write_op (pic32->ad1cssl, *data, offset);
        }
        break;

    case PIC32_AD1PCFG & 0xff0:         /* Port configuration */
        if (op_type == MTS_READ) {
            *data = pic32->ad1pcfg;
        } else {
            pic32->ad1pcfg = write_op (pic32->ad1pcfg, *data, offset);
        }
        break;

    case PIC32_ADC1BUF0 & 0xff0: case PIC32_ADC1BUF1 & 0xff0:
    case PIC32_ADC1BUF2 & 0xff0: case PIC32_ADC1BUF3 & 0xff0:
    case PIC32_ADC1BUF4 & 0xff0: case PIC32_ADC1BUF5 & 0xff0:
    case PIC32_ADC1BUF6 & 0xff0: case PIC32_ADC1BUF7 & 0xff0:
    case PIC32_ADC1BUF8 & 0xff0: case PIC32_ADC1BUF9 & 0xff0:
    case PIC32_ADC1BUFA & 0xff0: case PIC32_ADC1BUFB & 0xff0:
    case PIC32_ADC1BUFC & 0xff0: case PIC32_ADC1BUFD & 0xff0:
    case PIC32_ADC1BUFE & 0xff0: case PIC32_ADC1BUFF & 0xff0:
        if (op_type == MTS_READ) {      /* Result words */
            *data = 0; // TODO
        }
        break;

    default:
        ASSERT (0, "unknown adc offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_adc_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->ad1con1 = 0;
    pic32->ad1con2 = 0;
    pic32->ad1con3 = 0;
    pic32->ad1chs = 0;
    pic32->ad1cssl = 0;
    pic32->ad1pcfg = 0;
}

int dev_pic32_adc_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->sysdev = dev_create (name);
    if (! pic32->sysdev)
        return (-1);
    pic32->sysdev->priv_data = pic32;
    pic32->sysdev->phys_addr = paddr;
    pic32->sysdev->phys_len = ADC_REG_SIZE;
    pic32->sysdev->handler = dev_pic32_adc_access;
    pic32->sysdev->reset_handler = dev_pic32_adc_reset;
    pic32->sysdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, pic32->sysdev);
    return (0);
}
