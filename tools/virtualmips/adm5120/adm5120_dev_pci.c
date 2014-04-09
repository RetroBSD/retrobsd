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
#include "memory.h"
#include "adm5120.h"
#include "cpu.h"

m_uint32_t pci_table[PCI_INDEX_MAX];

struct pci_data {
    struct vdevice *dev;
    m_uint8_t *pci_ptr;
    m_uint32_t pci_size;
};

void *dev_pci_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct pci_data *d = dev->priv_data;

    if (offset >= d->pci_size) {
        *data = 0;
        return NULL;
    }

    return ((void *) (d->pci_ptr + offset));
}

void dev_pci_init_defaultvalue ()
{

}

int dev_pci_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct pci_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "PCI: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->pci_ptr = (m_uint8_t *) (m_iptr_t) (&pci_table[0]);
    d->pci_size = len;
    d->dev->handler = dev_pci_access;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);
    dev_pci_init_defaultvalue ();
    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
