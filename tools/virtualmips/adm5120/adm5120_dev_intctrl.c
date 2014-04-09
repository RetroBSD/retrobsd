 /*
  * Copyright (C) yajin 2008<yajinzhou@gmail.com >
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

m_uint32_t intctrl_table[INTCTRL_INDEX_MAX];

struct intctrl_data {
    struct vdevice *dev;
    m_uint8_t *intctrl_ptr;
    m_uint32_t intctrl_size;
};

void *dev_intctrl_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct intctrl_data *d = dev->priv_data;

    if (offset >= d->intctrl_size) {
        *data = 0;
        return NULL;
    }

    switch (offset) {
    case IRQ_DISABLE_REG:
        if (MTS_WRITE == op_type) {
            intctrl_table[IRQ_ENABLE_REG / 4] = *data & 0x3ff;
            *has_set_value = TRUE;
            return NULL;
        }
        break;
    }

    return ((void *) (d->intctrl_ptr + offset));
}

void dev_intctrl_init_defaultvalue ()
{

}

int dev_intctrl_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct intctrl_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "INTCTRL: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->intctrl_ptr = (m_uint8_t *) (m_iptr_t) (&intctrl_table[0]);
    d->intctrl_size = len;
    d->dev->handler = dev_intctrl_access;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);
    dev_intctrl_init_defaultvalue ();
    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
