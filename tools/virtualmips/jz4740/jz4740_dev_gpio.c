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
#include "cpu.h"
#include "jz4740.h"

/*set to 0 to improve performance. set to 1 to debug gpio*/
#define VALIDE_GPIO_OPERATION 0

m_uint32_t jz4740_gpio_table[JZ4740_GPIO_INDEX_MAX];

struct jz4740_gpio_data {
    struct vdevice *dev;
    m_uint8_t *jz4740_gpio_ptr;
    m_uint32_t jz4740_gpio_size;
};

/* GPIO is in 4 groups. 32 per group*/
/*

48-79      0
80-111    1
112-143  2
144-175  3

#define IRQ_GPIO3	25
#define IRQ_GPIO2	26
#define IRQ_GPIO1	27
#define IRQ_GPIO0	28
#define IRQ_GPIO_0	48   48 to 175 for GPIO pin 0 to 127 */

void dev_jz4740_gpio_setirq (int irq)
{
    int group_no;
    int pin_no;

    ASSERT ((irq >= IRQ_GPIO_0)
        && (irq < IRQ_GPIO_0 + 128), "wrong gpio irq 0x%x\n", irq);

    group_no = (irq - IRQ_GPIO_0) / 32;
    pin_no = (irq - IRQ_GPIO_0) % 32;
    jz4740_gpio_table[GPIO_PXFLG (group_no) / 4] |= 1 << pin_no;
}

void dev_jz4740_gpio_clearirq (int irq)
{
    int group_no;
    int pin_no;

    ASSERT ((irq >= IRQ_GPIO_0)
        && (irq < IRQ_GPIO_0 + 128), "wrong gpio irq 0x%x\n", irq);

    group_no = (irq - IRQ_GPIO_0) / 32;
    pin_no = (irq - IRQ_GPIO_0) % 32;
    jz4740_gpio_table[GPIO_PXFLG (group_no) / 4] &= ~(1 << pin_no);
}

void *dev_jz4740_gpio_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t * data,
    m_uint8_t * has_set_value)
{

    struct jz4740_gpio_data *d = dev->priv_data;

    m_uint8_t group;
    m_uint32_t mask, mask_data, temp;

    if (offset >= d->jz4740_gpio_size) {
        *data = 0;
        return NULL;
    }
#if  VALIDE_GPIO_OPERATION
    if (op_type == MTS_WRITE) {
        ASSERT (offset != GPIO_PXPIN (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPIN (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPIN (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPIN (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXDAT (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDAT (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDAT (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDAT (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXIM (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIM (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIM (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIM (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXPE (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPE (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPE (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPE (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXFUN (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUN (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUN (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUN (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXSEL (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXSEL (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXSEL (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXSEL (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXDIR (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIR (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIR (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIR (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXTRG (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRG (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRG (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRG (3),
            "Write to read only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXFLG (0),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFLG (1),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFLG (2),
            "Write to read only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFLG (3),
            "Write to read only register in GPIO. offset %x\n", offset);

    }
    if (op_type == MTS_READ) {
        ASSERT (offset != GPIO_PXDATS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDATS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDATS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDATS (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXDATC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDATC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDATC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDATC (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXIMS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIMS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIMS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIMS (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXIMC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIMC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIMC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXIMC (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXPES (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPES (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPES (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPES (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXPEC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPEC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPEC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXPEC (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXFUNS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUNS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUNS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUNS (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXFUNC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUNC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUNC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFUNC (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXDIRS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIRS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIRS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIRS (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXDIRC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIRC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIRC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXDIRC (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXTRGS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRGS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRGS (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRGS (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXTRGC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRGC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRGC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXTRGC (0),
            "Read write only register in GPIO. offset %x\n", offset);

        ASSERT (offset != GPIO_PXFLGC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFLGC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFLGC (0),
            "Read write only register in GPIO. offset %x\n", offset);
        ASSERT (offset != GPIO_PXFLGC (0),
            "Read write only register in GPIO. offset %x\n", offset);

    }
#endif

    if (op_type == MTS_READ) {

#ifdef SIM_PAVO
/*PAVO GPIO(C) PIN 30  -> NAND FLASH R/B. */
        if (offset == GPIO_PXPIN (2)) {
            /*FOR NAND FLASH.PIN 30 ----|_____|------ */
            temp = jz4740_gpio_table[GPIO_PXPIN (2) / 4];
            temp &= 0x40000000;
            if (temp)
                temp &= ~0x40000000;
            else
                temp |= 0x40000000;
            jz4740_gpio_table[GPIO_PXPIN (2) / 4] = temp;
        }
#endif

        return ((void *) (d->jz4740_gpio_ptr + offset));
    } else if (op_type == MTS_WRITE) {
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
        case GPIO_PXDATS (0):
        case GPIO_PXDATS (1):
        case GPIO_PXDATS (2):
        case GPIO_PXDATS (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXDAT (group) / 4] |= mask_data;
            break;

        case GPIO_PXDATC (0):
        case GPIO_PXDATC (1):
        case GPIO_PXDATC (2):
        case GPIO_PXDATC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXDAT (group) / 4] &= mask_data;
            break;

        case GPIO_PXIMS (0):
        case GPIO_PXIMS (1):
        case GPIO_PXIMS (2):
        case GPIO_PXIMS (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXIM (group) / 4] |= mask_data;
            break;

        case GPIO_PXIMC (0):
        case GPIO_PXIMC (1):
        case GPIO_PXIMC (2):
        case GPIO_PXIMC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXIM (group) / 4] &= mask_data;
            break;

        case GPIO_PXPES (0):
        case GPIO_PXPES (1):
        case GPIO_PXPES (2):
        case GPIO_PXPES (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXPE (group) / 4] |= mask_data;
            break;

        case GPIO_PXPEC (0):
        case GPIO_PXPEC (1):
        case GPIO_PXPEC (2):
        case GPIO_PXPEC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXPE (group) / 4] &= mask_data;
            break;

        case GPIO_PXFUNS (0):
        case GPIO_PXFUNS (1):
        case GPIO_PXFUNS (2):
        case GPIO_PXFUNS (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXFUN (group) / 4] |= mask_data;
            break;

        case GPIO_PXFUNC (0):
        case GPIO_PXFUNC (1):
        case GPIO_PXFUNC (2):
        case GPIO_PXFUNC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXFUN (group) / 4] &= mask_data;
            break;

        case GPIO_PXSELS (0):
        case GPIO_PXSELS (1):
        case GPIO_PXSELS (2):
        case GPIO_PXSELS (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXSEL (group) / 4] |= mask_data;
            break;

        case GPIO_PXSELC (0):
        case GPIO_PXSELC (1):
        case GPIO_PXSELC (2):
        case GPIO_PXSELC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXSEL (group) / 4] &= mask_data;
            break;

        case GPIO_PXDIRS (0):
        case GPIO_PXDIRS (1):
        case GPIO_PXDIRS (2):
        case GPIO_PXDIRS (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXDIR (group) / 4] |= mask_data;
            break;

        case GPIO_PXDIRC (0):
        case GPIO_PXDIRC (1):
        case GPIO_PXDIRC (2):
        case GPIO_PXDIRC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXDIR (group) / 4] &= mask_data;
            break;

        case GPIO_PXTRGS (0):
        case GPIO_PXTRGS (1):
        case GPIO_PXTRGS (2):
        case GPIO_PXTRGS (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            jz4740_gpio_table[GPIO_PXTRG (group) / 4] |= mask_data;
            break;

        case GPIO_PXTRGC (0):
        case GPIO_PXTRGC (1):
        case GPIO_PXTRGC (2):
        case GPIO_PXTRGC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXTRG (group) / 4] &= mask_data;
            break;

        case GPIO_PXFLGC (0):
        case GPIO_PXFLGC (1):
        case GPIO_PXFLGC (2):
        case GPIO_PXFLGC (3):
            group = offset / 0x100;
            mask_data = (*data) & mask;
            mask_data = ~(mask_data);
            jz4740_gpio_table[GPIO_PXFLG (group) / 4] &= mask_data;
            break;

        default:
            cpu_log (cpu, "", "invalid offset in GPIO. offset %x\n", offset);
            return NULL;

        }
        *has_set_value = TRUE;
        return NULL;
    }

    return NULL;
}

void dev_jz4740_gpio_init_defaultvalue ()
{
    memset (jz4740_gpio_table, 0x0, sizeof (jz4740_gpio_table));

    jz4740_gpio_table[GPIO_PXIM (0) / 4] = 0xffffffff;
    jz4740_gpio_table[GPIO_PXIM (1) / 4] = 0xffffffff;
    jz4740_gpio_table[GPIO_PXIM (2) / 4] = 0xffffffff;
    jz4740_gpio_table[GPIO_PXIM (3) / 4] = 0xffffffff;

    //jz4740_gpio_table[GPIO_PXPIN(2)/4]=0x40000000;

}

void dev_jz4740_gpio_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    dev_jz4740_gpio_init_defaultvalue ();
}

int dev_jz4740_gpio_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct jz4740_gpio_data *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "jz4740_gpio: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = len;
    d->jz4740_gpio_ptr = (m_uint8_t *) (&jz4740_gpio_table[0]);
    d->jz4740_gpio_size = len;
    d->dev->handler = dev_jz4740_gpio_access;
    d->dev->reset_handler = dev_jz4740_gpio_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    vm_bind_device (vm, d->dev);
    //dev_jz4740_gpio_init_defaultvalue();

    return (0);

  err_dev_create:
    free (d);
    return (-1);
}
