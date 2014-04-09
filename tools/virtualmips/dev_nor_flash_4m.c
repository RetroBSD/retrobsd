 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  */

/*
4M *byte* FLASH device simulation (device id=22F9h).
Most important part of flash simulation is CFI interface.
See flash datasheet for details.
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
#include<fcntl.h>

#include "device.h"
#include "mips_memory.h"

#define ROM_INIT_STATE              0

/* flash private data */
struct flash_data {
    struct vdevice *dev;
    m_uint8_t *flash_ptr;
    m_uint32_t flash_size;
    m_uint8_t *flash_file_name;
    m_uint32_t state;
    int flash_fd;
};
typedef struct flash_data flash_data_t;

#define BPTR(d,offset) (((char *)d->flash_ptr) + offset)

m_uint16_t vendorID = 0x01;     // target is little end   0x0001
m_uint16_t deviceID = 0x22f9;   // target is little end   0x22F9
m_uint16_t earse_ready = 0x80;  //  target is little end    0X0080
m_uint32_t last_offset = 0;

m_uint32_t dump_data;
m_uint32_t cfi_data[] =
    { 0x51, 0x52, 0x59, 0x2, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x27, 0x36,
        0x0, 0x0, 0x4,
    0x0, 0xa, 0x0, 0x5, 0x0, 0x4, 0x0, 0x16, 0x2, 0x0, 0x0, 0x0, 0x2, 0x7,
        0x0, 0x20,
    0x0, 0x3e, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0,
    0x50, 0x52, 0x49, 0x31, 0x31, 0x0, 0x2, 0x4, 0x1, 0x4, 0x0, 0x0, 0x0, 0xb5, 0xc5, 0x2,      //02 BOTTOM
};

static void secotor_info (m_uint32_t offset, m_uint32_t * sector_start,
    m_uint32_t * sector_size)
{
    if (offset <= 0x00FFF) {
        *sector_start = offset & 0xFFFFFE00;
        *sector_size = 0x2000;
        return;
    }
    *sector_start = offset & (0xffff0000);
    *sector_size = 0x10000;
}

void *dev_flash_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_uint32_t * data,
    m_uint8_t * has_set_value)
{
    flash_data_t *d = dev->priv_data;
    m_uint32_t r_offset, sector_size, sector_start;
    m_uint32_t last_sector_start, last_sector_size;

    if (offset >= d->flash_size) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ) {
        switch (d->state) {
        case 0:
            return (BPTR (d, offset));
        case 99:               /*cfi query */
            if ((offset >= 0x20) && (offset <= 0x9e)) {
                *data = (cfi_data[(offset - 0x20) / 2]);        //always littleend
                *has_set_value = TRUE;
            } else {
                d->state = 0;
                return (BPTR (d, offset));
            }

            break;
        case 0x6:
            d->state = 0;
            if (offset == 0X0)
                return &vendorID;
            if (offset == 0X2)
                return &deviceID;
            break;
        case 10:
            //last cycle is chip erase or sector erase
            secotor_info (offset, &sector_start, &sector_size);
            secotor_info (last_offset, &last_sector_start, &last_sector_size);
            d->state = 0;
            if (last_sector_start == sector_start)
                return &earse_ready;
            else
                return (BPTR (d, offset));
            break;

        default:
            cpu_log (cpu, dev->name, "read: unhandled state %d\n", d->state);
        }
        return NULL;
    }
    if (op_type == MTS_WRITE) {
        r_offset = offset;
        if ((op_size == MTS_HALF_WORD) && (offset == 0X554))
            offset = 0X555;

        switch (d->state) {
        case ROM_INIT_STATE:
            switch (offset) {
            case 0xAAA:
                if (((*data) & 0xff) == 0xAA)
                    d->state = 1;
                break;
            case 0XAA:
                if (((*data) & 0xff) == 0x98) {
                    d->state = 99;      //CFI QUERY
                }
                break;
            default:
                switch ((*data) & 0xff) {
                case 0xB0:
                    /* Erase/Program Suspend */
                    d->state = 0;
                    break;
                case 0x30:
                    /* Erase/Program Resume */
                    d->state = 0;
                    break;
                case 0xF0:
                case 0xFF:
                    /*Read/Reset */
                    d->state = 0;
                    break;
                default:
                    return ((void *) (d->flash_ptr + r_offset));
                }

            }
            break;

        case 99:
            if (((*data & 0xff) == 0xff) || ((*data & 0xff) == 0xf0))
                d->state = 0;
            else
                return ((void *) (d->flash_ptr + r_offset));
            break;
        case 1:
            if ((offset != 0x555) && ((*data & 0xff) != 0x55))
                d->state = 0;
            else
                d->state = 2;
            break;

        case 2:
            d->state = 0;

            if (offset == 0xAAA) {
                switch ((*data) & 0xff) {
                case 0x80:
                    d->state = 3;
                    break;
                case 0xA0:
                    /* Byte/Word program */
                    d->state = 4;
                    break;
                case 0x90:
                    /* Product ID Entry / Status of Block B protection */
                    d->state = 6;
                    break;
                }
            }
            break;

        case 3:
            if ((offset != 0xAAA) && (*data != 0xAA))
                d->state = 0;
            else
                d->state = 8;
            break;

        case 8:
            if ((offset != 0x555) && (*data != 0x55))
                d->state = 0;
            else
                d->state = 9;
            break;

        case 9:
            d->state = 10;
            last_offset = r_offset;

            switch ((*data) & 0xff) {
            case 0x10:
                /* Chip Erase */
                memset (BPTR (d, 0), 0, d->dev->phys_len);
                break;

            case 0x30:
                /* Sector Erase */
                secotor_info (r_offset, &sector_start, &sector_size);
                break;

            }
            break;

            /* Byte/Word Program */
        case 4:
            d->state = 0;
            return ((void *) (d->flash_ptr + r_offset));
            break;

        default:
            cpu_log (cpu, dev->name, "write: unhandled state %d\n", d->state);
        }
        return &dump_data;
    }
    assert (0);
}

static int dev_flash_load (char *flash_file_name, m_uint32_t flash_len,
    unsigned char **flash_data_hp, u_int create)
{
    int fd;
    struct stat sb;
    unsigned char *temp;

    fd = open (flash_file_name, O_RDWR);
    if ((fd < 0) && (create == 1)) {
        fprintf (stderr, "Can not open flash file. name %s\n",
            flash_file_name);
        fprintf (stderr, "creating flash file. name %s\n", flash_file_name);
        fd = open (flash_file_name, O_RDWR | O_CREAT,
            S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd < 0) {
            fprintf (stderr, "Can not create flash file. name %s\n",
                flash_file_name);
            return (-1);
        }
        temp = malloc (flash_len);
        assert (temp != NULL);
        memset (temp, 0xff, flash_len);
        lseek (fd, 0, SEEK_SET);
        write (fd, (void *) temp, flash_len);
        free (temp);
        fprintf (stderr, "create flash file success. name %s\n",
            flash_file_name);
        lseek (fd, 0, SEEK_SET);
    } else if (fd < 0) {
        fprintf (stderr, "%s does not exist and not allowed to create.\n",
            flash_file_name);
        return (-1);
    }
    assert (fd >= 0);
    fstat (fd, &sb);
    if (flash_len < sb.st_size) {
        fprintf (stderr,
            "Too large flash file. flash len:%d M, flash file name %s,"
            "flash file legth: %d bytes.\n", flash_len, flash_file_name,
            (int) sb.st_size);
        return (-1);
    }
    *flash_data_hp =
        mmap (NULL, sb.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (*flash_data_hp == MAP_FAILED) {
        fprintf (stderr, "errno %d\n", errno);
        fprintf (stderr, "failed\n");
        return (-1);
    }
    return 0;
}

/* Initialize a NOR Flash zone */
int dev_nor_flash_4m_init (vm_instance_t * vm, char *name)
{
    flash_data_t *d;
    unsigned char *flash_data_hp;

    /*load rom data */
    if (dev_flash_load (vm->flash_filename, vm->flash_size * 1048576,
            &flash_data_hp, 1) < 0)
        return (-1);

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "FLASH: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    d->flash_ptr = flash_data_hp;
    d->flash_size = vm->flash_size * 1048576;
    d->state = ROM_INIT_STATE;

    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->phys_addr = vm->flash_address;
    d->dev->phys_len = vm->flash_size * 1048576;
    d->dev->handler = dev_flash_access;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    /* Map this device to the VM */
    vm_bind_device (vm, d->dev);
    return (0);
  err_dev_create:
    free (d);
    return (-1);
}
