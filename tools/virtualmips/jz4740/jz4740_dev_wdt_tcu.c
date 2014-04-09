 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

 /* Watch dog and timer of JZ4740.
  * TODO:
  * 2. timer1-5
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
#include "vp_timer.h"
#include "vp_clock.h"

#define  VALIDE_WDT_TCU_OPERATION 0
extern cpu_mips_t *current_cpu;
m_uint32_t jz4740_wdt_tcu_table[JZ4740_WDT_INDEX_MAX];

/*4:EXT 2:RTC 1:PCK*/
//m_uint8_t jz4740_tcu_clock_source[JZ4740_WDT_INDEX_MAX];
//m_uint32_t jz4740_tcu_clock_prescale[JZ4740_WDT_INDEX_MAX];

/*clocksource/prescale*/
m_uint64_t jz4740_tcu_clock[JZ4740_WDT_INDEX_MAX];
m_uint64_t jz4740_wdt_clock;

struct jz4740_wdt_tcu_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_wdt_tcu_ptr;
    m_uint32_t jz4740_wdt_tcu_size;
    vp_timer_t *tcu_timer[JZ4740_WDT_INDEX_MAX];
    vp_timer_t *wdt_timer;
};

/*fire timer0 every 10ms*/
void dev_jz4740_tcu_active_timer0 (struct jz4740_wdt_tcu_data *d)
{
    d->tcu_timer[0]->set_time = vp_get_clock (rt_clock);
    vp_mod_timer (d->tcu_timer[0], vp_get_clock (rt_clock) + 10);
}

void dev_jz4740_tcu_unactive_timer0 (struct jz4740_wdt_tcu_data *d)
{
    vp_del_timer (d->tcu_timer[0]);
}

void dev_jz4740_active_wdt (struct jz4740_wdt_tcu_data *d)
{
    d->wdt_timer->set_time = vp_get_clock (rt_clock);
    vp_mod_timer (d->wdt_timer, vp_get_clock (rt_clock) + 10);
}

void dev_jz4740_unactive_wdt (struct jz4740_wdt_tcu_data *d)
{
    vp_del_timer (d->wdt_timer);
}

void *dev_jz4740_wdt_tcu_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value)
{

    struct jz4740_wdt_tcu_data *d = dev->priv_data;
    m_uint32_t mask_data, mask;
    int clock_index;
    //m_uint64_t clock;

    if (offset >= d->jz4740_wdt_tcu_size) {
        *data = 0;
        return NULL;
    }
#if  VALIDE_WDT_TCU_OPERATION
    if (op_type == MTS_WRITE) {
        ASSERT (offset != TCU_TSR,
            "Write to read only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TER,
            "Write to read only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TFR,
            "Write to read only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TMR,
            "Write to read only register in TCU. offset %x\n", offset);
    } else if (op_type == MTS_READ) {
        ASSERT (offset != TCU_TSSR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TSCR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TESR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TECR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TFSR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TFCR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TMSR,
            "Read write only register in TCU. offset %x\n", offset);
        ASSERT (offset != TCU_TMCR,
            "Read write only register in TCU. offset %x\n", offset);
    } else
        assert (0);
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
    case TCU_TSSR:             /*set */

        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        jz4740_wdt_tcu_table[TCU_TSR / 4] |= mask_data;
        *has_set_value = TRUE;
        if (jz4740_wdt_tcu_table[TCU_TSR / 4] & 0x01)
            dev_jz4740_tcu_unactive_timer0 (d);
        if (jz4740_wdt_tcu_table[TCU_TSR / 4] & WDT_TIMER_STOP)
            dev_jz4740_unactive_wdt (d);

        break;
    case TCU_TSCR:             /*clear */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        mask_data = ~(mask_data);
        jz4740_wdt_tcu_table[TCU_TSR / 4] &= mask_data;
        *has_set_value = TRUE;
        if ((!jz4740_wdt_tcu_table[TCU_TSR / 4] & 0x01)
            && (jz4740_wdt_tcu_table[TCU_TER / 4] & 0x01))
            dev_jz4740_tcu_active_timer0 (d);
        if (unlikely (jz4740_wdt_tcu_table[WDT_TCER / 4] & 0x01)
            && (!(jz4740_wdt_tcu_table[TCU_TSR / 4] & WDT_TIMER_STOP)))
            dev_jz4740_active_wdt (d);
        break;

    case TCU_TESR:             /*set */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        jz4740_wdt_tcu_table[TCU_TER / 4] |= mask_data;
        *has_set_value = TRUE;
        if ((!jz4740_wdt_tcu_table[TCU_TSR / 4] & 0x01)
            && (jz4740_wdt_tcu_table[TCU_TER / 4] & 0x01))
            dev_jz4740_tcu_active_timer0 (d);
        break;
    case TCU_TECR:             /*clear */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        mask_data = ~(mask_data);
        jz4740_wdt_tcu_table[TCU_TER / 4] &= mask_data;
        *has_set_value = TRUE;
        if (!(jz4740_wdt_tcu_table[TCU_TER / 4] & 0x01))
            dev_jz4740_tcu_unactive_timer0 (d);
        break;

    case TCU_TFSR:             /*set */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        jz4740_wdt_tcu_table[TCU_TFR / 4] |= mask_data;
        *has_set_value = TRUE;
        break;
    case TCU_TFCR:             /*clear */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        mask_data = ~(mask_data);
        jz4740_wdt_tcu_table[TCU_TFR / 4] &= mask_data;
        *has_set_value = TRUE;
        break;

    case TCU_TMSR:             /*set */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        jz4740_wdt_tcu_table[TCU_TMR / 4] |= mask_data;
        *has_set_value = TRUE;
        break;
    case TCU_TMCR:             /*clear */
        assert (op_type == MTS_WRITE);
        mask_data = (*data) & mask;
        mask_data = ~(mask_data);
        jz4740_wdt_tcu_table[TCU_TMR / 4] &= mask_data;
        *has_set_value = TRUE;
        break;
    case TCU_TCSR0:
    case TCU_TCSR1:
    case TCU_TCSR2:
    case TCU_TCSR3:
    case TCU_TCSR4:
    case TCU_TCSR5:
        if (op_type == MTS_WRITE) {
            clock_index = (offset - TCU_TCSR0) / 0x10;
            if (((*data) & TCU_CLOCK_SOUCE_MASK) == TCU_CLOCK_EXT) {
                jz4740_tcu_clock[clock_index] = EXT_CLOCK;

            } else if (((*data) & TCU_CLOCK_SOUCE_MASK) == TCU_CLOCK_RTC) {
                jz4740_tcu_clock[clock_index] = RTC_CLOCK;
            } else
                ASSERT (0, "RTC and EXT clock is supported \n");

            if ((((*data) & TCU_CLOCK_PRESCALE_MASK) >>
                    TCU_CLOCK_PRESCALE_OFFSET) == 0x1)
                jz4740_tcu_clock[clock_index] =
                    jz4740_tcu_clock[clock_index] / 4;
            else if ((((*data) & TCU_CLOCK_PRESCALE_MASK) >>
                    TCU_CLOCK_PRESCALE_OFFSET) == 0x2)
                jz4740_tcu_clock[clock_index] =
                    jz4740_tcu_clock[clock_index] / 16;
            else if ((((*data) & TCU_CLOCK_PRESCALE_MASK) >>
                    TCU_CLOCK_PRESCALE_OFFSET) == 0x3)
                jz4740_tcu_clock[clock_index] =
                    jz4740_tcu_clock[clock_index] / 64;
            else if ((((*data) & TCU_CLOCK_PRESCALE_MASK) >>
                    TCU_CLOCK_PRESCALE_OFFSET) == 0x4)
                jz4740_tcu_clock[clock_index] =
                    jz4740_tcu_clock[clock_index] / 256;
            else if ((((*data) & TCU_CLOCK_PRESCALE_MASK) >>
                    TCU_CLOCK_PRESCALE_OFFSET) == 0x5)
                jz4740_tcu_clock[clock_index] =
                    jz4740_tcu_clock[clock_index] / 1024;
            else if ((((*data) & TCU_CLOCK_PRESCALE_MASK) >>
                    TCU_CLOCK_PRESCALE_OFFSET) != 0x0)
                ASSERT (0, "INVALID PRESCALE\n");

        }
        return ((void *) (d->jz4740_wdt_tcu_ptr + offset));

    case WDT_TCSR:
        if (op_type == MTS_WRITE) {
            if (((*data) & WDT_CLOCK_SOUCE_MASK) == WDT_CLOCK_EXT) {
                jz4740_wdt_clock = EXT_CLOCK;
            } else if (((*data) & WDT_CLOCK_SOUCE_MASK) == WDT_CLOCK_RTC)
                jz4740_wdt_clock = RTC_CLOCK;
            else
                ASSERT (0, "RTC and EXT clock is supported \n");

            if ((((*data) & WDT_CLOCK_PRESCALE_MASK) >>
                    WDT_CLOCK_PRESCALE_OFFSET) == 0x1)
                jz4740_wdt_clock = jz4740_wdt_clock / 4;
            else if ((((*data) & WDT_CLOCK_PRESCALE_MASK) >>
                    WDT_CLOCK_PRESCALE_OFFSET) == 0x2)
                jz4740_wdt_clock = jz4740_wdt_clock / 16;
            else if ((((*data) & WDT_CLOCK_PRESCALE_MASK) >>
                    WDT_CLOCK_PRESCALE_OFFSET) == 0x3)
                jz4740_wdt_clock = jz4740_wdt_clock / 64;
            else if ((((*data) & WDT_CLOCK_PRESCALE_MASK) >>
                    WDT_CLOCK_PRESCALE_OFFSET) == 0x4)
                jz4740_wdt_clock = jz4740_wdt_clock / 256;
            else if ((((*data) & WDT_CLOCK_PRESCALE_MASK) >>
                    WDT_CLOCK_PRESCALE_OFFSET) == 0x5)
                jz4740_wdt_clock = jz4740_wdt_clock / 1024;
            else if ((((*data) & WDT_CLOCK_PRESCALE_MASK) >>
                    WDT_CLOCK_PRESCALE_OFFSET) != 0x0)
                ASSERT (0, "INVALID PRESCALE %x \n", *data);

        }
        return ((void *) (d->jz4740_wdt_tcu_ptr + offset));

    case WDT_TCER:
        if (op_type == MTS_WRITE) {
            jz4740_wdt_tcu_table[WDT_TCER / 4] = (*data) & 0x1;
            *has_set_value = TRUE;
            if (unlikely (jz4740_wdt_tcu_table[WDT_TCER / 4] & 0x01)
                && (!(jz4740_wdt_tcu_table[TCU_TSR / 4] & WDT_TIMER_STOP)))
                dev_jz4740_active_wdt (d);
            else
                dev_jz4740_unactive_wdt (d);
        } else
            return ((void *) (d->jz4740_wdt_tcu_ptr + offset));

    default:
        return ((void *) (d->jz4740_wdt_tcu_ptr + offset));

    }

    return NULL;

}

void dev_jz4740_wdt_tcu_init_defaultvalue ()
{

    memset (jz4740_wdt_tcu_table, 0x0, sizeof (jz4740_wdt_tcu_table));

    jz4740_wdt_tcu_table[TCU_TDFR0 / 4] = 0X7FF8;
    jz4740_wdt_tcu_table[TCU_TDHR0 / 4] = 0X7FF7;
    jz4740_wdt_tcu_table[TCU_TCNT0 / 4] = 0X7FF7;

    jz4740_wdt_tcu_table[TCU_TDFR1 / 4] = 0X7FF8;
    jz4740_wdt_tcu_table[TCU_TDHR1 / 4] = 0X7FF7;
    jz4740_wdt_tcu_table[TCU_TCNT1 / 4] = 0X7FF7;

    jz4740_wdt_tcu_table[TCU_TDFR2 / 4] = 0X7FF8;
    jz4740_wdt_tcu_table[TCU_TDHR2 / 4] = 0X7FF7;
    jz4740_wdt_tcu_table[TCU_TCNT2 / 4] = 0X7FF7;

    jz4740_wdt_tcu_table[TCU_TDFR3 / 4] = 0X7FF8;
    jz4740_wdt_tcu_table[TCU_TDHR3 / 4] = 0X7FF7;
    jz4740_wdt_tcu_table[TCU_TCNT3 / 4] = 0X7FF7;

    jz4740_wdt_tcu_table[TCU_TDFR4 / 4] = 0X7FF8;
    jz4740_wdt_tcu_table[TCU_TDHR4 / 4] = 0X7FF7;
    jz4740_wdt_tcu_table[TCU_TCNT4 / 4] = 0X7FF7;

    jz4740_wdt_tcu_table[TCU_TDFR5 / 4] = 0X7FF8;
    jz4740_wdt_tcu_table[TCU_TDHR5 / 4] = 0X7FF7;
    jz4740_wdt_tcu_table[TCU_TCNT5 / 4] = 0X7FF7;
}

void dev_jz4740_wdt_tcu_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_wdt_tcu_init_defaultvalue ();
}

int gasdf;
int reset_request = 0;

/*10ms*/
void dev_jz4740_wdt_cb (void *opaque)
{
    m_int64_t current;
    m_uint32_t past_time;

    struct jz4740_wdt_tcu_data *d = (struct jz4740_wdt_tcu_data *) opaque;
    if (unlikely (jz4740_wdt_tcu_table[WDT_TCER / 4] & 0x01)
        && (!(jz4740_wdt_tcu_table[TCU_TSR / 4] & WDT_TIMER_STOP))) {
        current = vp_get_clock (rt_clock);
        past_time = current - d->tcu_timer[0]->set_time;

        {
            jz4740_wdt_tcu_table[WDT_TCNT / 4] += (jz4740_wdt_clock) / 100;

            if (jz4740_wdt_tcu_table[WDT_TCNT / 4] >=
                jz4740_wdt_tcu_table[WDT_TDR / 4]) {
                /*RESET soc */
                cpu_restart (current_cpu);
                jz4740_reset (current_cpu->vm);
            }
        }
    }
    dev_jz4740_active_wdt (d);

}

/*10ms
Linux uses 100HZ timer, so we fire tcu every 10 ms.
1ms ->  jz4740_wdt_tcu_table[TCU_TCNT0/4]+= jz4740_tcu_clock[0])/1000; */
void dev_jz4740_tcu_cb (void *opaque)
{
    struct jz4740_wdt_tcu_data *d = (struct jz4740_wdt_tcu_data *) opaque;

    m_int64_t current;
    m_uint32_t past_time;
    current = vp_get_clock (rt_clock);
    past_time = current - d->tcu_timer[0]->set_time;

    jz4740_wdt_tcu_table[TCU_TCNT0 / 4] +=
        (past_time * jz4740_tcu_clock[0]) / 1000;
    jz4740_wdt_tcu_table[TCU_TCNT0 / 4] &= 0xffff;

    if (jz4740_wdt_tcu_table[TCU_TCNT0 / 4] >=
        jz4740_wdt_tcu_table[TCU_TDHR0 / 4]) {
        /*set TFR */
        jz4740_wdt_tcu_table[TCU_TFR / 4] |= 1 << 16;
        if (!(jz4740_wdt_tcu_table[TCU_TMR / 4] & (1 << 16)))
            current_cpu->vm->set_irq (current_cpu->vm, IRQ_TCU0);
    }
    if (jz4740_wdt_tcu_table[TCU_TCNT0 / 4] >=
        jz4740_wdt_tcu_table[TCU_TDFR0 / 4]) {
        jz4740_wdt_tcu_table[TCU_TFR / 4] |= 1;
        if (!(jz4740_wdt_tcu_table[TCU_TMR / 4] & (0x1))) {
            current_cpu->vm->set_irq (current_cpu->vm, IRQ_TCU0);
        }
        jz4740_wdt_tcu_table[TCU_TCNT0 / 4] = 0;
    }

    dev_jz4740_tcu_active_timer0 (d);

}

int dev_jz4740_wdt_tcu_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_wdt_tcu_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "jz4740_wdt_tcu: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->jz4740_wdt_tcu_ptr = (m_uint8_t *) (&jz4740_wdt_tcu_table[0]);
    d->jz4740_wdt_tcu_size = len;
    d->dev->handler = dev_jz4740_wdt_tcu_access;
    d->dev->reset_handler = dev_jz4740_wdt_tcu_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    /*only emulate timer0 */
    d->tcu_timer[0] = vp_new_timer (rt_clock, dev_jz4740_tcu_cb, d);
    d->wdt_timer = vp_new_timer (rt_clock, dev_jz4740_wdt_cb, d);

    vm_bind_device (vm, d->dev);

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}

#if 0

/*-------------Virtual Timer and WDT Timer----------------------*/
m_uint32_t past_instructions = 0;
//m_uint32_t   past_instructions[6];
/*TODO:need to adjust*/
#define COUNT_PER_INSTRUCTION   0x80    //0X180

/*JUST TIMER 0*/
void forced_inline virtual_jz4740_timer (cpu_mips_t * cpu)
{

    if (unlikely (jz4740_wdt_tcu_table[TCU_TSR / 4] & 0x01)) {
        return;
    }
    if (likely (jz4740_wdt_tcu_table[TCU_TER / 4] & 0x01)) {
        //allow counter
        past_instructions++;
        if (past_instructions == COUNT_PER_INSTRUCTION) {
            jz4740_wdt_tcu_table[TCU_TCNT0 / 4] += 1;
            if (jz4740_wdt_tcu_table[TCU_TCNT0 / 4] ==
                jz4740_wdt_tcu_table[TCU_TDHR0 / 4]) {
                /*set TFR */
                jz4740_wdt_tcu_table[TCU_TFR / 4] |= 1 << 16;
                if (!(jz4740_wdt_tcu_table[TCU_TMR / 4] & (1 << 16)))
                    cpu->vm->set_irq (cpu->vm, IRQ_TCU0);
            }
            if (jz4740_wdt_tcu_table[TCU_TCNT0 / 4] ==
                jz4740_wdt_tcu_table[TCU_TDFR0 / 4]) {
                jz4740_wdt_tcu_table[TCU_TFR / 4] |= 1;
                if (!(jz4740_wdt_tcu_table[TCU_TMR / 4] & (0x1))) {
                    cpu->vm->set_irq (cpu->vm, IRQ_TCU0);
                }

                jz4740_wdt_tcu_table[TCU_TCNT0 / 4] = 0;
            }
            past_instructions = 0;

        }
    }

}

m_uint32_t wdt_past_instructions = 0;

void forced_inline virtual_jz4740_wdt (cpu_mips_t * cpu)
{

    if (likely (jz4740_wdt_tcu_table[TCU_TSR / 4] & WDT_TIMER_STOP)) {
        return;
    }

    if (unlikely (jz4740_wdt_tcu_table[WDT_TCER / 4] & 0x01)) {

        wdt_past_instructions++;
        if (wdt_past_instructions >= COUNT_PER_INSTRUCTION) {
            jz4740_wdt_tcu_table[WDT_TCNT / 4] += 1;
            wdt_past_instructions = 0;
            if (jz4740_wdt_tcu_table[WDT_TCNT / 4] & 0xffff0000)
                jz4740_wdt_tcu_table[WDT_TCNT / 4] = 0;

            if (jz4740_wdt_tcu_table[WDT_TCNT / 4] >=
                jz4740_wdt_tcu_table[WDT_TDR / 4]) {
                /*RESET soc */
                cpu_stop (cpu);
                cpu->cpu_thread_running = FALSE;
                jz4740_reset (cpu->vm);

            }
        }

    }

}

void forced_inline virtual_timer (cpu_mips_t * cpu)
{
    virtual_jz4740_timer (cpu);
    virtual_jz4740_wdt (cpu);
}
#endif
