 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifdef SIM_LCD
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
#include "vp_sdl.h"
#include "vp_timer.h"
#include "utils.h"

int dev_jz4740_ts_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len, struct DisplayState *ds);

#define LCD_TIMEOUT  500        //MS

#define LCD_WIDTH  480
#define LCD_HEIGHT  272
#define LCD_BPP  32             /*32 bit per pixel */

extern cpu_mips_t *current_cpu;

m_uint32_t jz4740_lcd_table[JZ4740_LCD_INDEX_MAX];
struct jz_fb_dma_descriptor {
    m_uint32_t fdadr;           /* Frame descriptor address register */
    m_uint32_t fsadr;           /* Frame source address register */
    m_uint32_t fidr;            /* Frame ID register */
    m_uint32_t ldcmd;           /* Command register */

};

struct jz4740_lcd_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_lcd_ptr;
    m_uint32_t jz4740_lcd_size;
    vp_timer_t *lcd_timer;
    struct DisplayState *ds;
    m_uint16_t vde, vds, hds, hde;
};

void dev_jz4740_lcd_init_defaultvalue ()
{
    memset (jz4740_lcd_table, 0x0, sizeof (jz4740_lcd_table));

}

void dev_jz4740_active_lcd (struct jz4740_lcd_data *d)
{
    vp_mod_timer (d->lcd_timer, vp_get_clock (rt_clock) + LCD_TIMEOUT);
}

void dev_jz4740_unactive_lcd (struct jz4740_lcd_data *d)
{
    vp_del_timer (d->lcd_timer);
}

void dev_jz4740_lcd_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    struct jz4740_lcd_data *d = dev->priv_data;
    dev_jz4740_lcd_init_defaultvalue ();
    dev_jz4740_unactive_lcd (d);
}

void *dev_jz4740_lcd_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_lcd_data *d = dev->priv_data;

    if ((offset >= d->jz4740_lcd_size)) {
        *data = 0;
        return NULL;
    }

    switch (offset) {
    case LCD_CTRL:
        if (op_type == MTS_WRITE) {
            if (*data & LCD_CTRL_ENA) {
                dev_jz4740_active_lcd (d);
            } else {
                dev_jz4740_unactive_lcd (d);
                jz4740_lcd_table[LCD_STATE / 4] |= LCD_STATE_QD;
                 /*INTERRUPT*/ if ((jz4740_lcd_table[LCD_CTRL /
                            4] & LCD_CTRL_QDM) == 0x0) {
                    current_cpu->vm->set_irq (current_cpu->vm, IRQ_LCD);
                }

            }

            if (*data & LCD_CTRL_DIS) {
                dev_jz4740_unactive_lcd (d);
                jz4740_lcd_table[LCD_STATE / 4] |= LCD_STATE_LDD;
                 /*INTERRUPT*/ if ((jz4740_lcd_table[LCD_CTRL /
                            4] & LCD_CTRL_LDDM) == 0x0) {
                    current_cpu->vm->set_irq (current_cpu->vm, IRQ_LCD);
                }

            }

            if ((*data & LCD_CTRL_BPP_MASK) == LCD_CTRL_BPP_16) {
                d->ds->depth = 16;
                dpy_resize (d->ds, d->ds->width, d->ds->height);

            } else if ((*data & LCD_CTRL_BPP_MASK) == LCD_CTRL_BPP_18_24) {
                d->ds->depth = 32;
                dpy_resize (d->ds, d->ds->width, d->ds->height);
            } else {
                ASSERT (0, "errror bpp \n");
            }

        }
        break;
    case LCD_DAH:
        if (op_type == MTS_WRITE) {
            d->hde = (*data & LCD_DAH_HDE_MASK) >> LCD_DAH_HDE_BIT;
            d->hds = (*data & LCD_DAH_HDS_MASK) >> LCD_DAH_HDS_BIT;
            ASSERT (d->hde > d->hds, "hde < hds hde %x hds %x \n", d->hde,
                d->hds);
            d->ds->width = d->hde - d->hds;
            ASSERT (d->ds->width > 0, "d->ds->width<=0  %x  \n",
                d->ds->width);
            ASSERT (d->ds->height > 0, "d->ds->height<=0  %x  \n",
                d->ds->height);
            dpy_resize (d->ds, d->ds->width, d->ds->height);

        }
        break;
    case LCD_DAV:
        if (op_type == MTS_WRITE) {
            d->vde = (*data & LCD_DAV_VDE_MASK) >> LCD_DAV_VDE_BIT;
            d->vds = (*data & LCD_DAV_VDS_MASK) >> LCD_DAV_VDS_BIT;
            ASSERT (d->vde > d->vds, "vde < vds vde %x vds %x \n", d->vde,
                d->vds);
            d->ds->height = d->vde - d->vds;
            ASSERT (d->ds->width > 0, "d->ds->width<=0  %x  \n",
                d->ds->width);
            ASSERT (d->ds->height > 0, "d->ds->height<=0  %x  \n",
                d->ds->height);
            dpy_resize (d->ds, d->ds->width, d->ds->height);
        }
        break;

    }

    return ((void *) (d->jz4740_lcd_ptr + offset));

}

void dev_jz4740_lcd_cb (void *opaque)
{
    struct jz4740_lcd_data *d = opaque;
    struct jz_fb_dma_descriptor *lcda0_desc = NULL;
    m_uint8_t *src_data;
    m_uint32_t dummy;

    lcda0_desc =
        (struct jz_fb_dma_descriptor *) physmem_get_hptr (current_cpu->vm,
        jz4740_lcd_table[LCD_DA0 / 4], 0, MTS_READ, &dummy);
    src_data =
        physmem_get_hptr (current_cpu->vm, lcda0_desc->fsadr, 0, MTS_READ,
        &dummy);

    ASSERT (src_data != NULL,
        "dev_jz4740_lcd_cb can not get framebuffer src data\n");
    jz4740_lcd_table[LCD_SA0 / 4] = lcda0_desc->fsadr;
    jz4740_lcd_table[LCD_FID0 / 4] = lcda0_desc->fidr;
    jz4740_lcd_table[LCD_CMD0 / 4] = lcda0_desc->ldcmd;

    ASSERT ((((jz4740_lcd_table[LCD_CTRL / 4] & LCD_CTRL_BPP_MASK) ==
                LCD_CTRL_BPP_16)
            || ((jz4740_lcd_table[LCD_CTRL / 4] & LCD_CTRL_BPP_MASK) ==
                LCD_CTRL_BPP_18_24)), "Only 15/16/18/24 bpp supproted\n");
    memcpy (d->ds->data, src_data,
        d->ds->height * d->ds->width * d->ds->depth / 8);
    dpy_update (d->ds, 0, 0, 0, 0);
    jz4740_lcd_table[LCD_STATE / 4] |= LCD_STATE_EOF;
    if ((jz4740_lcd_table[LCD_CMD0 / 4] & LCD_CMD_EOFINT)
        && ((jz4740_lcd_table[LCD_CTRL / 4] & LCD_CTRL_EOFM) == 0x0)) {
        current_cpu->vm->set_irq (current_cpu->vm, IRQ_LCD);
    }
    dev_jz4740_active_lcd (d);

}

int dev_jz4740_lcd_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_lcd_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "jz4740_lcd: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->jz4740_lcd_ptr = (m_uint8_t *) (&jz4740_lcd_table[0]);
    d->jz4740_lcd_size = len;
    d->dev->handler = dev_jz4740_lcd_access;
    d->dev->reset_handler = dev_jz4740_lcd_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->lcd_timer = vp_new_timer (rt_clock, dev_jz4740_lcd_cb, d);

    d->ds = malloc (sizeof (*d->ds));
    if (d->ds == NULL)
        goto err_dev_create;
    d->ds->width = LCD_WIDTH;
    d->ds->height = LCD_HEIGHT;
    d->ds->depth = 32;
    sdl_display_init (d->ds, 0);

    vm_bind_device (vm, d->dev);

    /*Init jz4740 internal TS */
    if (dev_jz4740_ts_init (vm, "JZ4740 TS", JZ4740_TS_BASE, JZ4740_TS_SIZE,
            d->ds) == -1)
        goto err_dev_create;

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}

#endif
