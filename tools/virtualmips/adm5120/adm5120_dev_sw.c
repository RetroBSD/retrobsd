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

m_uint32_t sw_table[SW_INDEX_MAX];

struct sw_data {
    struct vdevice *dev;
    m_uint8_t *sw_ptr;
    m_uint32_t sw_size;
};
extern int timeout;
extern m_uint32_t time_reload;
void *dev_sw_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{
    struct sw_data *d = dev->priv_data;
    if (offset >= d->sw_size) {
        *data = 0;
        return NULL;
    }

    switch (offset) {
    case Timer_int_REG:
        if (op_type == MTS_WRITE) {
            if (*data & SW_TIMER_INT) {
                timeout = 0;
            }
        } else if (op_type == MTS_READ) {

            *data = sw_table[Timer_int_REG / 4] & (~SW_TIMER_INT);
            *data |= timeout;
            *has_set_value = TRUE;
            return NULL;
        } else
            assert (0);
        break;
    case Timer_REG:
        if (op_type == MTS_WRITE) {

            time_reload = *data & SW_TIMER_MASK;
        }
        break;
    }
    return ((void *) (d->sw_ptr + offset));
}

void dev_sw_init_defaultvalue ()
{
    sw_table[CODE_REG / 4] = 0x34085120;
    sw_table[Timer_int_REG / 4] = 0x10000;
    sw_table[Timer_REG / 4] = 0xffff;
}

int dev_sw_init (vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len)
{
    struct sw_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "SW: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->sw_ptr = (m_uint8_t *) (m_iptr_t) (&sw_table[0]);
    d->sw_size = len;
    d->dev->handler = dev_sw_access;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);
    dev_sw_init_defaultvalue ();
    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
