 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef __DEV_CS8900_H__
#define __DEV_CS8900_H__

#include "utils.h"
#include "cpu.h"
#include "vm.h"
#include "mips_memory.h"
#include "device.h"
#include "net.h"
#include "net_io.h"
#include "vp_timer.h"

#define CS8900_INTERNAL_RAM_SIZE   0x1000       /*4K */
/* CS8900 Data */
struct cs8900_data {
    char *name;
    m_uint32_t cs8900_size;

    /* Device information */
    struct vdevice *dev;

    /* Virtual machine */
    vm_instance_t *vm;

    /* NetIO descriptor */
    netio_desc_t *nio;          /*one nio can have multi listener */

    /*internal RAM 4K bytes */
    m_uint32_t internal_ram[CS8900_INTERNAL_RAM_SIZE / 4];
    m_uint32_t irq_no;

    m_uint16_t rx_read_index;
    m_uint16_t tx_send_index;

    vp_timer_t *cs8900_timer;

    //  m_uint8_t want_tx;
//    m_uint8_t want_rx;

};

int dev_cs8900_set_nio (struct cs8900_data *d, netio_desc_t * nio);
struct cs8900_data *dev_cs8900_init (vm_instance_t * vm, char *name,
    m_pa_t phys_addr, m_uint32_t phys_len, int irq);

#endif
