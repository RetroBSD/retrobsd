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

extern cpu_mips_t *current_cpu;

#define TSMAXX 4096
#define TSMAXY 4096
#define TSMINX  0
#define TSMINY  0

#define TS_TIMEOUT  15          //MS
m_uint32_t jz4740_ts_table[JZ4740_TS_INDEX_MAX];

#define MOUSE_DOWN  1
#define MOUSE_UP       2
#define MOUSE_MOVE   3
struct jz4740_ts_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_ts_ptr;
    m_uint32_t jz4740_ts_size;
    vp_timer_t *ts_timer;

    m_uint8_t xyz;              /*xyz of cfg */
    m_uint8_t snum;             /*snum of cfg */

    m_uint8_t read_index;
    m_uint32_t x, y;

    /*mouse status */
    m_uint8_t mouse_status;

    struct DisplayState *ds;
};

void dev_jz4740_active_ts (struct jz4740_ts_data *d)
{
    vp_mod_timer (d->ts_timer, vp_get_clock (rt_clock) + TS_TIMEOUT);
}

void dev_jz4740_unactive_ts (struct jz4740_ts_data *d)
{
    vp_del_timer (d->ts_timer);
}

void dev_jz4740_ts_cb (void *opaque)
{
    struct jz4740_ts_data *d = opaque;
    SDL_Event *ts_ev;
    if ((jz4740_ts_table[SADC_ENA / 4] & SADC_ENA_TSEN)) {
        if ((jz4740_ts_table[SADC_CTRL / 4] & SADC_CTRL_PENDM) == 0) {
            ts_ev = sdl_getmouse_down ();
            /*Pen down interrupt */
            if (ts_ev != NULL) {
                d->x =
                    ((ts_ev->button.x) * (TSMAXX - TSMINX) / d->ds->width) +
                    TSMINX;
                d->y =
                    ((ts_ev->button.y) * (TSMAXY - TSMINY) / d->ds->height) +
                    TSMINY;
                if (d->x > TSMAXX)
                    d->x = TSMAXX;
                if (d->y > TSMAXY)
                    d->y = TSMAXY;

                /*Interrupt */
                current_cpu->vm->set_irq (current_cpu->vm, IRQ_SADC);
                jz4740_ts_table[SADC_STATE / 4] |=
                    (SADC_STATE_PEND | SADC_STATE_TSRDY);
            }
        }

        if ((jz4740_ts_table[SADC_CTRL / 4] & SADC_CTRL_PENUM) == 0) {
            /*Pen UP interrupt */
            if (d->read_index == 0) {
                /*
                 * TODO: Mouse up checking.
                 * We do not check whether mouse up. JUST assume when clicked, it is always mouseup.
                 * Can not use hand writing in qtopia.
                 */
                /*Interrupt */
                current_cpu->vm->set_irq (current_cpu->vm, IRQ_SADC);
                jz4740_ts_table[SADC_STATE / 4] |=
                    (SADC_STATE_PENU | SADC_STATE_TSRDY);

            }
        }

    }

    dev_jz4740_active_ts (d);

}

void dev_jz4740_ts_init_defaultvalue ()
{
    memset (jz4740_ts_table, 0x0, sizeof (jz4740_ts_table));

}

void dev_jz4740_ts_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_ts_init_defaultvalue ();
}

void *dev_jz4740_ts_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_ts_data *d = dev->priv_data;

    if ((offset >= d->jz4740_ts_size)) {
        *data = 0;
        return NULL;
    }
    switch (offset) {
    case SADC_ENA:
        if (op_type == MTS_WRITE) {
            ASSERT ((*data & SADC_ENA_TSEN),
                "Only touche screen model is support in SADC \n");
            if (*data & SADC_ENA_TSEN)
                dev_jz4740_active_ts (d);
            else
                dev_jz4740_unactive_ts (d);
        }
        break;
    case SADC_CFG:
        if (op_type == MTS_WRITE) {
            ASSERT ((*data & SADC_CFG_TS_DMA) == 0x0,
                "Only touche screen model is support in SADC \n");
            ASSERT ((*data & SADC_CFG_XYZ_MASK) != (3 << SADC_CFG_XYZ_BIT),
                "XYZ =3 is not support \n");
            ASSERT ((*data & SADC_CFG_XYZ_MASK) != (2 << SADC_CFG_XYZ_BIT),
                "XYZ =2 is not support \n");

            d->xyz = (*data & SADC_CFG_XYZ_MASK) >> SADC_CFG_XYZ_BIT;
            d->snum = ((*data & SADC_CFG_SNUM_MASK) >> SADC_CFG_SNUM_BIT) + 1;
            /*need read twice */
            if (d->xyz == 0x1)
                d->snum *= 2;

            d->read_index = 0;

        }
        break;
    case SADC_STATE:
        if (op_type == MTS_WRITE) {
            jz4740_ts_table[SADC_STATE / 4] = ~(*data);
            *has_set_value = TRUE;
            return NULL;
        } else {
            *data = jz4740_ts_table[SADC_STATE / 4] & 0x1f;
            *data |= SADC_CTRL_TSRDYM;
            *has_set_value = TRUE;
            return NULL;
        }
        break;

    case SADC_TSDAT:
        if (op_type == MTS_READ) {

            if ((d->read_index % 2) == 0) {
                *data = ((d->x) & 0x7fff) | ((d->y & 0x7ffff) << 16);
            } else {
                *data = ((500) & 0x7fff);
            }
            *has_set_value = TRUE;
            d->read_index++;
            if (d->read_index == d->snum)
                d->read_index = 0;
            return NULL;
        }
        break;

    }
    return ((void *) (d->jz4740_ts_ptr + offset));
}

int dev_jz4740_ts_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len, struct DisplayState *ds)
{
    struct jz4740_ts_data *d;

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
    d->jz4740_ts_ptr = (m_uint8_t *) (&jz4740_ts_table[0]);
    d->jz4740_ts_size = len;
    d->dev->handler = dev_jz4740_ts_access;
    d->dev->reset_handler = dev_jz4740_ts_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->ts_timer = vp_new_timer (rt_clock, dev_jz4740_ts_cb, d);
    d->ds = ds;

    vm_bind_device (vm, d->dev);

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}

#endif
