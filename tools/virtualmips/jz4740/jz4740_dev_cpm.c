 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*Just a dummy cpm for JZ4740
I need document!!!
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
#include "cpu.h"
#include "jz4740.h"

m_uint32_t jz4740_cpm_table[JZ4740_CPM_INDEX_MAX];

struct jz4740_cpm_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_cpm_ptr;
    m_uint32_t jz4740_cpm_size;
};

void *dev_jz4740_cpm_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_cpm_data *d = dev->priv_data;

    if (offset >= d->jz4740_cpm_size) {
        *data = 0;
        return NULL;
    }
    return ((void *) (d->jz4740_cpm_ptr + offset));

}

void dev_jz4740_cpm_init_defaultvalue ()
{
    memset (jz4740_cpm_table, 0x0, sizeof (jz4740_cpm_table));
}

void dev_jz4740_cpm_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_cpm_init_defaultvalue ();
}

int dev_jz4740_cpm_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_cpm_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "jz4740_cpm: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->jz4740_cpm_ptr = (m_uint8_t *) (&jz4740_cpm_table[0]);
    d->jz4740_cpm_size = len;
    d->dev->handler = dev_jz4740_cpm_access;
    d->dev->reset_handler = dev_jz4740_cpm_reset;

    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);
    //dev_jz4740_cpm_init_defaultvalue();

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
