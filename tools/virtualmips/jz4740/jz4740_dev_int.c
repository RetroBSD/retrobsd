 /*
  * Copyright (C) yajin 2008<yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*jz4740 Interrupt controller*/

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

#define VALIDE_INT_OPERATION 0

extern cpu_mips_t *current_cpu;
m_uint32_t jz4740_int_table[JZ4740_INT_INDEX_MAX];

struct jz4740_int_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_int_ptr;
    m_uint32_t jz4740_int_size;
};

void *dev_jz4740_int_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_int_data *d = dev->priv_data;
    m_uint32_t mask_data, mask;

    if (offset >= d->jz4740_int_size) {
        *data = 0;
        return NULL;
    }
#if  VALIDE_INT_OPERATION
    if (op_type == MTS_WRITE) {
        ASSERT (offset != INTC_ISR,
            "Write to read only register in INT. offset %x\n", offset);
        ASSERT (offset != INTC_IPR,
            "Write to read only register in INT. offset %x\n", offset);
    } else if (op_type == MTS_READ) {
        ASSERT (offset != INTC_IMSR,
            "Read write only register in INT. offset %x\n", offset);
        ASSERT (offset != INTC_IMCR,
            "Read write only register in INT. offset %x\n", offset);

    }
#endif

    switch (op_size) {
    case 1:
        mask = 0xff;
        break;
    case 2:
        mask = 0xffff;
        break;
    case 4:
        mask = 0xffffffff;
        break;
    default:
        assert (0);
    }

    switch (offset) {
    case INTC_IMSR:            /*set */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        jz4740_int_table[INTC_IMR / 4] |= mask_data;
        *has_set_value = TRUE;
        break;
    case INTC_IMCR:            /*clear */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        mask_data = ~(mask_data);
        jz4740_int_table[INTC_IMR / 4] &= mask_data;
        *has_set_value = TRUE;
        break;
    case INTC_IPR:             /*clear */
        *data = jz4740_int_table[INTC_IPR / 4];
        jz4740_int_table[INTC_IPR / 4] = 0;
        *has_set_value = TRUE;
        return NULL;
    default:
        return ((void *) (d->jz4740_int_ptr + offset));
    }
    return NULL;
}

void dev_jz4740_int_init_defaultvalue ()
{
    memset (jz4740_int_table, 0x0, sizeof (jz4740_int_table));
    jz4740_int_table[INTC_IMR / 4] = 0xffffffff;
}

void dev_jz4740_int_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_int_init_defaultvalue ();
}

int dev_jz4740_int_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_int_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "jz4740_dma: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->jz4740_int_ptr = (m_uint8_t *) (&jz4740_int_table[0]);
    d->jz4740_int_size = len;
    d->dev->handler = dev_jz4740_int_access;
    d->dev->reset_handler = dev_jz4740_int_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);
    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
