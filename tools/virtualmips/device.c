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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>

#include "cpu.h"
#include "vm.h"
#include "mips_memory.h"
#include "device.h"

#define DEBUG_DEV_ACCESS  0

/* Get device by ID */
struct vdevice *dev_get_by_id (vm_instance_t * vm, u_int dev_id)
{
    if (!vm || (dev_id >= VM_DEVICE_MAX))
        return NULL;

    return (vm->dev_array[dev_id]);
}

/* Get device by name */
struct vdevice *dev_get_by_name (vm_instance_t * vm, char *name)
{
    struct vdevice *dev;

    if (!vm)
        return NULL;

    for (dev = vm->dev_list; dev; dev = dev->next)
        if (!strcmp (dev->name, name))
            return dev;

    return NULL;
}

/* Device lookup by physical address */
//struct vdevice *dev_lookup(vm_instance_t *vm,m_pa_t phys_addr,int cached)
struct vdevice *dev_lookup (vm_instance_t * vm, m_pa_t phys_addr)
{
    struct vdevice *dev;

    if (!vm)
        return NULL;

    for (dev = vm->dev_list; dev; dev = dev->next) {
        // if (cached && !(dev->flags & VDEVICE_FLAG_CACHING))
        //    continue;

        if ((phys_addr >= dev->phys_addr)
            && ((phys_addr - dev->phys_addr) < dev->phys_len))
            return dev;
    }

    return NULL;
}

/* Find the next device after the specified address */
//struct vdevice *dev_lookup_next(vm_instance_t *vm,m_pa_t phys_addr,
//                                struct vdevice *dev_start,int cached)
struct vdevice *dev_lookup_next (vm_instance_t * vm, m_pa_t phys_addr,
    struct vdevice *dev_start)
{
    struct vdevice *dev;

    if (!vm)
        return NULL;

    dev = (dev_start != NULL) ? dev_start : vm->dev_list;
    for (; dev; dev = dev->next) {
        //if (cached && !(dev->flags & VDEVICE_FLAG_CACHING))
        //   continue;

        if (dev->phys_addr > phys_addr)
            return dev;
    }

    return NULL;
}

/* Initialize a device */
void dev_init (struct vdevice *dev)
{
    memset (dev, 0, sizeof (*dev));
    dev->fd = -1;
}

/* reset all devices */
void dev_reset_all (vm_instance_t * vm)
{
    struct vdevice *dev;
    for (dev = vm->dev_list; dev; dev = dev->next) {
        ASSERT (dev->reset_handler != NULL,
            "reset_handler is NULL. name %s\n", dev->name);
        dev->reset_handler (vm->boot_cpu, dev);
    }
}

/* Allocate a device */
struct vdevice *dev_create (char *name)
{
    struct vdevice *dev;

    if (!(dev = malloc (sizeof (*dev)))) {
        fprintf (stderr,
            "dev_create: insufficient memory to " "create device '%s'.\n",
            name);
        return NULL;
    }

    dev_init (dev);
    dev->name = name;
    return dev;
}

/* Remove a device */
void dev_remove (vm_instance_t * vm, struct vdevice *dev)
{
    if (dev == NULL)
        return;

    vm_unbind_device (vm, dev);

    vm_log (vm, "DEVICE",
        "Removal of device %s, fd=%d, host_addr=0x%" LL "x, flags=%d\n",
        dev->name, dev->fd, (m_uint64_t) dev->host_addr, dev->flags);

    if (dev->flags & VDEVICE_FLAG_REMAP) {
        dev_init (dev);
        return;
    }

    if (dev->fd != -1) {
        /* Unmap memory mapped file */
        if (dev->host_addr) {

            vm_log (vm, "MMAP", "unmapping of device '%s', "
                "fd=%d, host_addr=0x%" LL "x, len=0x%x\n",
                dev->name, dev->fd, (m_uint64_t) dev->host_addr,
                dev->phys_len);
            munmap ((void *) dev->host_addr, dev->phys_len);
        }

        close (dev->fd);
    } else {
        /* Use of malloc'ed host memory: free it */
        if (dev->host_addr)
            free ((void *) dev->host_addr);
    }

    /* reinitialize the device to a clean state */
    dev_init (dev);
}

/* Show properties of a device */
void dev_show (struct vdevice *dev)
{
    if (!dev)
        return;

    printf ("   %-18s: 0x%12.12" LL "x (0x%8.8x)\n", dev->name,
        dev->phys_addr, dev->phys_len);
}

/* Show the device list */
void dev_show_list (vm_instance_t * vm)
{
    struct vdevice *dev;

    printf ("\nVM \"%s\" () Device list:\n", vm->name);

    for (dev = vm->dev_list; dev; dev = dev->next)
        dev_show (dev);

    printf ("\n");
}

/* Remap a device at specified physical address */
struct vdevice *dev_remap (char *name, struct vdevice *orig, m_pa_t paddr,
    m_uint32_t len)
{
    struct vdevice *dev;

    if (!(dev = dev_create (name)))
        return NULL;

    dev->phys_addr = paddr;
    dev->phys_len = len;
    dev->flags = orig->flags | VDEVICE_FLAG_REMAP;
    dev->fd = orig->fd;
    dev->host_addr = orig->host_addr;
    dev->handler = orig->handler;
    //dev->sparse_map = orig->sparse_map;
    return dev;
}

/* Create a RAM device */
struct vdevice *dev_create_ram (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len)
{
    struct vdevice *dev;

    if (!(dev = dev_create (name)))
        return NULL;

    dev->phys_addr = paddr;
    dev->phys_len = len;
    //dev->flags = VDEVICE_FLAG_CACHING;
    dev->host_addr = (m_iptr_t) m_memalign (4096, dev->phys_len);

    if (!dev->host_addr) {
        free (dev);
        return NULL;
    }

    vm_bind_device (vm, dev);
    return dev;
}
