/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 */

 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <sys/types.h>

#include "utils.h"
#include "mips.h"
#include "vm.h"
#include "cpu.h"
/* Device Flags */
#define VDEVICE_FLAG_NO_MTS_MMAP  0x01  /* Prevent MMAPed access by MTS */
#define VDEVICE_FLAG_CACHING      0x02  /* Device does support caching */
#define VDEVICE_FLAG_REMAP        0x04  /* Physical address remapping */
//#define VDEVICE_FLAG_COW        0x08  /* Copy on write device  */

#define VDEVICE_PTE_DIRTY  0x01

/*device handler */
typedef void *(*dev_handler_t) (cpu_mips_t * cpu, vdevice_t * dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value);

typedef void (*dev_reset_handler_t) (cpu_mips_t * cpu, vdevice_t * dev);

/* Virtual Device */
struct vdevice {
    char *name;
    u_int id;
    m_pa_t phys_addr;
    m_uint32_t phys_len;
    m_iptr_t host_addr;
    void *priv_data;
    int flags;
    int fd;
    dev_handler_t handler;
    dev_reset_handler_t reset_handler;
    struct vdevice *next, **pprev;
};

/* device access function */
static forced_inline
    void *dev_access_fast (cpu_mips_t * cpu, u_int dev_id, m_uint32_t offset,
    u_int op_size, u_int op_type, m_reg_t * data, m_uint8_t * has_set_value)
{
    struct vdevice *dev = cpu->vm->dev_array[dev_id];

    if (unlikely (!dev)) {
        cpu_log (cpu, "dev_access_fast",
            "null  handler (dev_id=%u,offset=0x%x)\n", dev_id, offset);
        return NULL;
    }
    return (dev->handler (cpu, dev, offset, op_size, op_type, data,
            has_set_value));
}

struct vdevice *dev_lookup (vm_instance_t * vm, m_pa_t phys_addr);
void dev_init (struct vdevice *dev);
struct vdevice *dev_create (char *name);
struct vdevice *dev_create_ram (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
void dev_reset_all (vm_instance_t * vm);

#endif
