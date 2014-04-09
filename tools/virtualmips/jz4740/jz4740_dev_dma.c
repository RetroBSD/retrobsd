 /*
  * Copyright (C) yajin 2008<yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*DMA Controller
Only support AUTO Request.

TODO:
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

m_uint32_t jz4740_dma_table[JZ4740_DMA_INDEX_MAX];

struct jz4740_dma_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_dma_ptr;
    m_uint32_t jz4740_dma_size;
};

typedef struct {
    m_uint32_t dcmd;            /* DCMD value for the current transfer */
    m_uint32_t dsadr;           /* DSAR value for the current transfer */
    m_uint32_t dtadr;           /* DTAR value for the current transfer */
    m_uint32_t ddadr;           /* Points to the next descriptor + transfer count */
} jz_dma_desc;

static int get_dma_unit_size (int channel, m_uint32_t cmd_value)
{
    //switch (jz4740_dma_table[DMAC_DCMD(channel)/4]&DMAC_DCMD_DS_MASK)
    switch (cmd_value & DMAC_DCMD_DS_MASK) {
    case DMAC_DCMD_DS_32BIT:
        return 4;
    case DMAC_DCMD_DS_16BIT:
        return 2;
    case DMAC_DCMD_DS_8BIT:
        return 1;
    case DMAC_DCMD_DS_16BYTE:
        return 16;
    case DMAC_DCMD_DS_32BYTE:
        return 32;
    default:
        assert (0);
    }

}

void dma_non_descriptor_trans (cpu_mips_t * cpu, struct jz4740_dma_data *d,
    int channel)
{

    physmem_dma_transfer (cpu->vm, jz4740_dma_table[DMAC_DSAR (channel) / 4],
        jz4740_dma_table[DMAC_DTAR (channel) / 4],
        get_dma_unit_size (channel,
            jz4740_dma_table[DMAC_DCMD (channel) / 4]) *
        jz4740_dma_table[DMAC_DTCR (channel) / 4]);
    /*we have finished dma */
    jz4740_dma_table[DMAC_DTCR (channel) / 4] = 0;

    /*set DIR QP */
    jz4740_dma_table[DMAC_DMAIPR / 4] |= 1 << channel;
    /*some cleanup work */
    /*clean AR TT GLOBAL AR */
    jz4740_dma_table[DMAC_DCCSR (channel) / 4] |= ~DMAC_DCCSR_AR;
    jz4740_dma_table[DMAC_DCCSR (channel) / 4] |= ~DMAC_DCCSR_TT;
    jz4740_dma_table[DMAC_DMACR / 4] |= ~DMAC_DMACR_AR;

    if (jz4740_dma_table[DMAC_DCMD (channel) / 4] & DMAC_DCMD_TIE) {
        cpu->vm->set_irq (cpu->vm, IRQ_DMAC);
    }

}

void dma_descriptor_trans (cpu_mips_t * cpu, struct jz4740_dma_data *d,
    int channel)
{
    jz_dma_desc *desc;
    m_pa_t desc_phy;
    m_uint32_t dummy_data;

    /*fetch the first descritpor */
    desc_phy = jz4740_dma_table[DMAC_DDA (channel) / 4];
    ASSERT ((desc_phy & 0xf) == 0, "DDA%d should be 16 bytes aligned\n",
        channel);
    desc = physmem_get_hptr (cpu->vm, desc_phy, 4, MTS_READ, &dummy_data);
    ASSERT (desc != NULL, "error descriptor phyaddress %x\n", desc_phy);
    while (1) {
        if (((desc->dcmd) & DMAC_DCMD_DES_VM)
            && (!((desc->dcmd) & DMAC_DCMD_DES_V))) {
            /*STOP DMA SET DCSN.INV=1 */
            jz4740_dma_table[DMAC_DCCSR (channel) / 4] |= DMAC_DCCSR_INV;
            return;
        }
        physmem_dma_transfer (cpu->vm, desc->dsadr,
            desc->dtadr,
            (desc->ddadr & 0xffffff) * get_dma_unit_size (channel,
                desc->dcmd));

        if ((desc->dcmd) & DMAC_DCMD_DES_VM) {
            /*clear v */
            desc->dcmd |= ~DMAC_DCMD_DES_V;
        }
        if ((desc->dcmd) & DMAC_DCMD_LINK) {
            /*set DCSN.CT=1 */
            jz4740_dma_table[DMAC_DCCSR (channel) / 4] |= DMAC_DCCSR_CT;
        } else {
            /*set DCSN.TT=1 */
            jz4740_dma_table[DMAC_DCCSR (channel) / 4] |= DMAC_DCCSR_TT;
        }

        if (desc->dcmd & DMAC_DCMD_TIE) {
            cpu->vm->set_irq (cpu->vm, IRQ_DMAC);
        }

        if ((desc->dcmd) & DMAC_DCMD_LINK) {
            /*fetch next descriptor */
            desc_phy = jz4740_dma_table[DMAC_DDA (channel) / 4] & 0xfffff000;
            desc_phy += (desc->dtadr & 0xff000000) >> 24;
            desc =
                physmem_get_hptr (cpu->vm, desc_phy, 4, MTS_READ,
                &dummy_data);
            ASSERT (desc != NULL, "error descriptor phyaddress %x\n",
                desc_phy);
        } else
            break;

    }

}

void enable_dma_channel (cpu_mips_t * cpu, struct jz4740_dma_data *d,
    int channel)
{
    if (jz4740_dma_table[DMAC_DMACR / 4] & DMAC_DMACR_DMAE) {
        if ((jz4740_dma_table[DMAC_DCCSR (channel) / 4] & DMAC_DCCSR_NDES)) {
            /*NON DESCRIPTOR */
            dma_non_descriptor_trans (cpu, d, channel);
        }
    }
}

void enable_global_dma (cpu_mips_t * cpu, struct jz4740_dma_data *d)
{
    int channel;
    for (channel = 0; channel < MAX_DMA_NUM; channel++) {
        if ((jz4740_dma_table[DMAC_DCCSR (channel) / 4] & DMAC_DCCSR_NDES)) {
            if ((jz4740_dma_table[DMAC_DCCSR (channel) /
                        4] & DMAC_DCCSR_NDES))
                dma_non_descriptor_trans (cpu, d, channel);     /*NON DESCRIPTOR */
        }
    }

}

void enable_dma_dbn (cpu_mips_t * cpu, struct jz4740_dma_data *d, int channel)
{
    /*DESCRIPTOR trans */
    if ((jz4740_dma_table[DMAC_DMACR / 4] & DMAC_DMACR_DMAE)
        && (jz4740_dma_table[DMAC_DCCSR (channel) / 4] & DMAC_DCCSR_NDES)) {
        dma_descriptor_trans (cpu, d, channel);
    }
}

void *dev_jz4740_dma_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_dma_data *d = dev->priv_data;
    int channel;

    if (offset >= d->jz4740_dma_size) {
        *data = 0;
        return NULL;
    }

    if (op_type == MTS_WRITE) {
        switch (offset) {
        case DMAC_DRSR0:
        case DMAC_DRSR1:
        case DMAC_DRSR2:
        case DMAC_DRSR3:
        case DMAC_DRSR4:
        case DMAC_DRSR5:
            /*only support AUTO request */
            ASSERT (((*data) & DMAC_DRSR_RS_MASK) == DMAC_DRSR_RS_AUTO,
                "only support AUTO request\n");
            return ((void *) (d->jz4740_dma_ptr + offset));

        case DMAC_DCCSR0:
        case DMAC_DCCSR1:
        case DMAC_DCCSR2:
        case DMAC_DCCSR3:
        case DMAC_DCCSR4:
        case DMAC_DCCSR5:
            channel = (offset - 0x10) / 0x20;
            jz4740_dma_table[DMAC_DCCSR (channel) / 4] = *data;
            *has_set_value = TRUE;
            if ((*data) & DMAC_DCCSR_EN) {
                enable_dma_channel (cpu, d, channel);
            }
            return NULL;

        case DMAC_DMACR:
            /*DMA Control register */
            jz4740_dma_table[DMAC_DMACR / 4] = *data;
            *has_set_value = TRUE;
            if ((*data) & DMAC_DMACR_DMAE) {
                enable_global_dma (cpu, d);
            }
            return NULL;

        case DMAC_DMADBR:
        case DMAC_DMADBSR:
            jz4740_dma_table[DMAC_DMADBR / 4] = *data;
            *has_set_value = TRUE;
            for (channel = 0; channel < 6; channel++) {
                if ((*data) & (1 << channel)) {
                    enable_dma_dbn (cpu, d, channel);
                    break;
                }

            }
            return NULL;
        default:
            return ((void *) (d->jz4740_dma_ptr + offset));
        }

    } else if (op_type == MTS_READ)
        return ((void *) (d->jz4740_dma_ptr + offset));
    else
        assert (0);

    return NULL;

}

void dev_jz4740_dma_init_defaultvalue ()
{
    memset (jz4740_dma_table, 0x0, sizeof (jz4740_dma_table));
}

void dev_jz4740_dma_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_dma_init_defaultvalue ();
}

int dev_jz4740_dma_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_dma_data *d;

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
    d->jz4740_dma_ptr = (m_uint8_t *) (&jz4740_dma_table[0]);
    d->jz4740_dma_size = len;
    d->dev->handler = dev_jz4740_dma_access;
    d->dev->reset_handler = dev_jz4740_dma_reset;

    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
