/*
 * Internal Flash memory of PIC32 microcontroller.

 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#include "device.h"
#include "mips_memory.h"
#include "pic32.h"

/* flash private data */
typedef struct flash_data {
    struct vdevice *dev;
    m_uint8_t *flash_ptr;
    m_uint32_t flash_size;
    m_uint8_t *flash_file_name;
} flash_data_t;

void *dev_flash_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_uint32_t * data, m_uint8_t * has_set_value)
{
    flash_data_t *d = dev->priv_data;

    if (offset >= d->flash_size) {
        printf ("-- flash: access %08x -- out of memory\n", dev->phys_addr + offset);
        *data = 0xff;
        *has_set_value = TRUE;
        return NULL;
    }
    if (op_type == MTS_READ) {
#if 0
        printf ("-- flash: read %08x -> %08x\n", dev->phys_addr + offset,
                *(unsigned *) (d->flash_ptr + offset));
#endif
        return d->flash_ptr + offset;
    }
    if (op_type == MTS_WRITE) {
        printf ("-- flash: write %08x ***\n", dev->phys_addr + offset);
        return NULL;
    }
    assert (0);
}

static int dev_flash_load (char *flash_file_name, unsigned flash_len,
    unsigned char **flash_data_hp)
{
    int fd;
    struct stat sb;

    fd = open (flash_file_name, O_RDONLY);
    if (fd < 0) {
        fprintf (stderr, "%s does not exist.\n", flash_file_name);
        return (-1);
    }
    fstat (fd, &sb);
    if (flash_len < sb.st_size) {
        fprintf (stderr,
            "Too large flash file.\nFlash len: %d kbytes, file name %s, "
            "file legth: %d bytes.\n", flash_len / 1024, flash_file_name,
            (int) sb.st_size);
        return (-1);
    }
    if (sb.st_size <= 0) {
        fprintf (stderr, "%s: empty flash file.\n", flash_file_name);
        return (-1);
    }
    *flash_data_hp = mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (*flash_data_hp == MAP_FAILED) {
        fprintf (stderr, "%s: ", flash_file_name);
        perror ("mmap");
        return (-1);
    }
    return 0;
}

static void dev_flash_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    //flash_data_t *d = dev->priv_data;

    //d->state = 0;
}

/*
 * Initialize a NOR Flash zone
 */
int dev_pic32_flash_init (vm_instance_t * vm, char *name,
    unsigned flash_kbytes, unsigned flash_address, char *filename)
{
    flash_data_t *d;
    unsigned char *flash_data_hp;

    /* load rom data */
    if (dev_flash_load (filename, flash_kbytes * 1024, &flash_data_hp) < 0)
        return (-1);

    /* allocate the private data structure */
    d = malloc (sizeof (*d));
    if (!d) {
        fprintf (stderr, "FLASH: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    d->flash_ptr = flash_data_hp;
    d->flash_size = flash_kbytes * 1024;

    d->dev = dev_create (name);
    if (!d->dev) {
        free (d);
        return (-1);
    }
    d->dev->priv_data = d;
    d->dev->phys_addr = flash_address;
    d->dev->phys_len = flash_kbytes * 1024;
    d->dev->handler = dev_flash_access;
    d->dev->reset_handler = dev_flash_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    /* Map this device to the VM */
    vm_bind_device (vm, d->dev);
    return (0);
}
