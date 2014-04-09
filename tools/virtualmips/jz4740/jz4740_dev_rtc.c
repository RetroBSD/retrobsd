 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*RTC. */

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
#include "vp_timer.h"

#define RTC_TIMEOUT  1000       //1000MS=1S
extern cpu_mips_t *current_cpu;
m_uint32_t jz4740_rtc_table[JZ4740_RTC_INDEX_MAX];

struct jz4740_rtc_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_rtc_ptr;
    m_uint32_t jz4740_rtc_size;
    vp_timer_t *rtc_timer;

};
static const unsigned int sum_monthday[13] = {
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    365
};
static const unsigned int yearday[5] =
    { 0, 366, 366 + 365, 366 + 365 * 2, 366 + 365 * 3 };
static const unsigned int sweekday = 6;
unsigned int forced_inline jz_mktime (int year, int mon, int day, int hour,
    int min, int sec)
{
    unsigned int seccounter;

    if (year < 2000)
        year = 2000;
    year -= 2000;
    seccounter = (year / 4) * (365 * 3 + 366);
    seccounter += yearday[year % 4];
    if (year % 4)
        seccounter += sum_monthday[mon - 1];
    else if (mon >= 3)
        seccounter += sum_monthday[mon - 1] + 1;        /* Feb is 29 days. */
    else
        seccounter += sum_monthday[mon - 1];
    seccounter += day - 1;
    seccounter *= 24;
    seccounter += hour;
    seccounter *= 60;
    seccounter += min;
    seccounter *= 60;
    seccounter += sec;

    return seccounter;
}

/*Set RTC Time. From Year 2000.*/
void dev_jz4740_rtc_init_defaultvalue ()
{
    time_t timep;
    struct tm *p;

    memset (jz4740_rtc_table, 0x0, sizeof (jz4740_rtc_table));
    /*Set RTC value to current time */
    time (&timep);
    p = localtime (&timep);
    jz4740_rtc_table[RTC_RSR / 4] =
        jz_mktime ((1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
        p->tm_hour, p->tm_min, p->tm_sec);

}

void dev_jz4740_rtc_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_rtc_init_defaultvalue ();
}

void dev_jz4740_active_rtc (struct jz4740_rtc_data *d)
{
    vp_mod_timer (d->rtc_timer, vp_get_clock (rt_clock) + RTC_TIMEOUT);
}

void dev_jz4740_unactive_rtc (struct jz4740_rtc_data *d)
{
    vp_del_timer (d->rtc_timer);
}

void *dev_jz4740_rtc_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_rtc_data *d = dev->priv_data;
    if (offset >= d->jz4740_rtc_size) {
        *data = 0;
        return NULL;
    }

    switch (offset) {
    case RTC_RCR:
        if (op_type == MTS_READ) {
            /*RTC_RCR (RTC_RCR_WRDY )=1 bit 7 */
            jz4740_rtc_table[RTC_RCR / 4] |= RTC_RCR_WRDY;
        } else if (op_type == MTS_WRITE) {
            if (*data & RTC_RCR_RTCE) {
                dev_jz4740_active_rtc (d);
            } else {
                dev_jz4740_unactive_rtc (d);
            }
        }
        break;

    }

    return ((void *) (d->jz4740_rtc_ptr + offset));

}

void dev_jz4740_rtc_cb (void *opaque)
{
    struct jz4740_rtc_data *d = opaque;
    time_t timep;
    struct tm *p;

    if (jz4740_rtc_table[RTC_RCR / 4] & RTC_RCR_RTCE) {
        //rtc enable
        jz4740_rtc_table[RTC_RCR / 4] |= RTC_RCR_1HZ;
        if (jz4740_rtc_table[RTC_RCR / 4] & RTC_RCR_1HZIE) {
            current_cpu->vm->set_irq (current_cpu->vm, IRQ_RTC);
        }

        time (&timep);
        p = localtime (&timep);
        /*always get the current time from host */
        jz4740_rtc_table[RTC_RSR / 4] =
            jz_mktime ((1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec);
        if (jz4740_rtc_table[RTC_RSR / 4] == jz4740_rtc_table[RTC_RSAR / 4]) {
            if (jz4740_rtc_table[RTC_RCR / 4] & RTC_RCR_AE) {
                jz4740_rtc_table[RTC_RCR / 4] |= RTC_RCR_AF;
                if (jz4740_rtc_table[RTC_RCR / 4] & RTC_RCR_AIE) {
                    current_cpu->vm->set_irq (current_cpu->vm, IRQ_RTC);
                }
            }
        }
    }
    dev_jz4740_active_rtc (d);

}

int dev_jz4740_rtc_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_rtc_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "jz4740_rtc: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->jz4740_rtc_ptr = (m_uint8_t *) (&jz4740_rtc_table[0]);
    d->jz4740_rtc_size = len;
    d->dev->handler = dev_jz4740_rtc_access;
    d->dev->reset_handler = dev_jz4740_rtc_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->rtc_timer = vp_new_timer (rt_clock, dev_jz4740_rtc_cb, d);

    vm_bind_device (vm, d->dev);
    dev_jz4740_rtc_init_defaultvalue ();

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
