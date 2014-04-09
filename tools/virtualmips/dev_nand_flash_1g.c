 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*
1G bytes nand flash emulation. Samsung K9F8G08 1GB
1G bytes nand flash are stored in file nandflash8g.0-nandflash8g.8191 (8192 blocks).
The flash file only be created when writing to block (copy on write).
Please use tool/mknandflash to create init nand file of u-boot image.
*/

/*

supported operation:
  READ
  Read for copy back
  Read ID
  Reset
  Page program
  Copy-back
  Block erase
  Random data input
  Random data output

*/

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
#include <dirent.h>

#include "device.h"
#include "mips_memory.h"
#include "dev_nand_flash_1g.h"

m_uint8_t id_info[5] = { 0xec, 0xd3, 0x51, 0x95, 0x58 };

#define NAND_STATUS_READY       0x40
#define NAND_STATUS_WP            0x80

/*status ready & not write protected*/
/*I am always ready. haha. Better than real nand flash :)*/
m_uint8_t nand_status = NAND_STATUS_READY | NAND_STATUS_WP;

#ifdef CPU_LOG
#undef CPU_LOG
#endif

//#define DEBUG_FLASH_ACCESS
#ifdef DEBUG_FLASH_ACCESS
#define CPU_LOG(arg1) cpu_log arg1
#else
#define CPU_LOG(arg1)
#endif

/*Create nand flash file. 1 block 1 file.*/
static unsigned char *create_nand_flash_file (m_uint32_t block_no)
{
    char file_path[64];
    char page[NAND_FLASH_1G_PAGE_SIZE];
    int i, n;
    int fd;
    unsigned char *ret;

    /*create nand flash file when writing */
    snprintf (file_path, sizeof (file_path), "%s/%s.%d",
        NAND_FLASH_1G_FILE_DIR, NAND_FLASH_1G_FILE_PREFIX, block_no);
    fd = open (file_path, O_RDWR | O_CREAT,
        S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    assert (fd >= 0);

    for (i = 0; i < NAND_FLASH_1G_PAGES_PER_BLOCK; i++) {
        memset (page, 0xff, NAND_FLASH_1G_PAGE_SIZE);
        n = write (fd, page, NAND_FLASH_1G_PAGE_SIZE);
        assert (n == NAND_FLASH_1G_PAGE_SIZE);
    }
    ret = memzone_map_file (fd, NAND_FLASH_1G_BLOCK_SIZE);
    assert (ret != NULL);
    return ret;
}

/*get the page pointer given row addr and block start address*/
unsigned char *get_nand_flash_page_ptr (m_uint32_t row_addr,
    unsigned char *block_start)
{

    m_uint32_t block_no = row_addr >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
    m_uint32_t page_no = row_addr & NAND_FLASH_1G_BLOCK_PAGE_MASK;
    assert (block_no < NAND_FLASH_1G_TOTAL_BLOCKS);
    assert (block_start != NULL);
    return (block_start + page_no * NAND_FLASH_1G_PAGE_SIZE);

}

static void nand_flash_erase_block (unsigned char *block_start)
{
    memset (block_start, 0xff, NAND_FLASH_1G_BLOCK_SIZE);
}

/*write data to nand file (1 page)*/
static void write_nand_fiash_page_file (m_uint32_t row_addr,
    unsigned char *block_start, unsigned char *write_data)
{
    unsigned char *page_ptr;
    int i;

    page_ptr = get_nand_flash_page_ptr (row_addr, block_start);
    /*we only copy different data into page */
    for (i = 0; i < NAND_FLASH_1G_PAGE_SIZE; i++) {
        if (*(write_data + i) == 0XFF)
            continue;
        if ((*(page_ptr + i)) != (*(write_data + i))) {
            *(page_ptr + i) = *(write_data + i);
        }
    }

}

char *state_string[8] = { "STATE_INIT",
    "STATE_READ_START",
    "STATE_RANDOM_READ_START",
    "STATE_WRITE_START",
    "STATE_RANDOM_WRITE_START",
    "STATE_READ_PAGE_FOR_COPY_WRITE",
    "STATE_COPY_START",
    "STATE_ERASE_START",
};

void *dev_nand_flash_1g_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_uint32_t * data, m_uint8_t * has_set_value)
{
    nand_flash_1g_data_t *d = dev->priv_data;
    m_uint32_t block_no;
    void *ret;
    /*COMMAND PORT */
    if (offset == NAND_COMMPORT_OFFSET) {
        /*clear addr offset */
        d->addr_offset = 0;
        switch (d->state) {
        case STATE_INIT:
            if (((*data) & 0xff) == 0x00)
                d->state = STATE_READ_START;
            else if (((*data) & 0xff) == 0x80) {
                memset (d->write_buffer, 0xff, NAND_FLASH_1G_PAGE_SIZE);
                d->state = STATE_WRITE_START;
            } else if (((*data) & 0xff) == 0x05) {
                assert (d->has_issue_30h == 1);
                d->state = STATE_RANDOM_READ_START;
                d->has_issue_30h = 0;
            } else if (((*data) & 0xff) == 0xFF) {
                /*reset */
                d->state = STATE_INIT;
            } else if (((*data) & 0xff) == 0x90) {
                /*read ID */
                d->data_port_ipr = id_info;
                d->state = STATE_INIT;
                d->read_offset = 0;
            } else if (((*data) & 0xff) == 0x60) {
                /*ERASE */
                d->state = STATE_ERASE_START;
                /*erase only need row address. adjust addr_offset */
                d->addr_offset = 2;
            } else if (((*data) & 0xff) == 0x70) {
                /*READ STATUS */
                d->state = STATE_INIT;
                d->read_offset = 0;
                d->data_port_ipr = &nand_status;
            } else
                ASSERT (0, "*data %x\n", *data);
            break;

        case STATE_ERASE_START:
            if (((*data) & 0xff) == 0xd0) {
                //erase blcok
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                if (d->flash_map[block_no] == NULL)
                    d->flash_map[block_no] =
                        create_nand_flash_file (block_no);
                nand_flash_erase_block (d->flash_map[block_no]);
                d->state = STATE_INIT;
            } else
                assert (0);
            break;
        case STATE_READ_START:
            if (((*data) & 0xff) == 0x30) {
                d->has_issue_30h = 1;
                d->state = STATE_INIT;
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                if (d->flash_map[block_no] == NULL) {
                    CPU_LOG ((cpu, "",
                            "block_no %x is null. redirect to fake page.",
                            block_no));
                    d->data_port_ipr =
                        get_nand_flash_page_ptr (d->row_addr, d->fake_block);
                } else
                    d->data_port_ipr =
                        get_nand_flash_page_ptr (d->row_addr,
                        d->flash_map[block_no]);

                d->read_offset = d->col_addr;
                CPU_LOG ((cpu, "", "d->read_offset %x d->col_addr %x.",
                        d->read_offset, d->col_addr));
            } else if (((*data) & 0xff) == 0x35) {
                d->state = STATE_READ_PAGE_FOR_COPY_WRITE;
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                memset (d->write_buffer, 0xff, NAND_FLASH_1G_PAGE_SIZE);
            } else if (((*data) & 0xff) == 0xFF) {
                /*reset */
                d->state = STATE_INIT;

            } else
                assert (0);
            break;

        case STATE_RANDOM_READ_START:
            if (((*data) & 0xff) == 0xe0) {
                d->state = STATE_INIT;
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                if (d->flash_map[block_no] == NULL) {
                    CPU_LOG ((cpu, "",
                            "block_no %x is null. redirect to fake page.",
                            block_no));
                    d->data_port_ipr =
                        get_nand_flash_page_ptr (d->row_addr, d->fake_block);
                } else
                    d->data_port_ipr =
                        get_nand_flash_page_ptr (d->row_addr,
                        d->flash_map[block_no]);
                d->read_offset = d->col_addr;
            } else if (((*data) & 0xff) == 0x05) {
                d->state = STATE_RANDOM_READ_START;
            } else
                assert (0);
            break;

        case STATE_WRITE_START:
            if (((*data) & 0xff) == 0x10) {
                d->state = STATE_INIT;
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                if (d->flash_map[block_no] == NULL)
                    d->flash_map[block_no] =
                        create_nand_flash_file (block_no);

                write_nand_fiash_page_file (d->row_addr,
                    d->flash_map[block_no], d->write_buffer);
                d->write_offset = 0;
            } else if (((*data) & 0xff) == 0x85) {
                d->write_offset = 0;
                d->state = STATE_RANDOM_WRITE_START;
            } else
                assert (0);
            break;

        case STATE_RANDOM_WRITE_START:
            if (((*data) & 0xff) == 0x10) {
                d->state = STATE_INIT;
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                if (d->flash_map[block_no] == NULL)
                    d->flash_map[block_no] =
                        create_nand_flash_file (block_no);
                write_nand_fiash_page_file (d->row_addr,
                    d->flash_map[block_no], d->write_buffer);
                d->write_offset = 0;
            } else if (((*data) & 0xff) == 0x85) {
                d->write_offset = 0;
                d->state = STATE_RANDOM_WRITE_START;
            } else
                assert (0);
            break;

        case STATE_READ_PAGE_FOR_COPY_WRITE:
            if (((*data) & 0xff) == 0x85) {
                d->write_offset = 0;

                d->state = STATE_COPY_START;
            } else
                assert (0);
            break;

        case STATE_COPY_START:
            if (((*data) & 0xff) == 0x10) {
                d->state = STATE_INIT;
                block_no = (d->row_addr) >> NAND_FLASH_1G_BLOCK_PAGE_OFFSET;
                if (d->flash_map[block_no] == NULL)
                    d->flash_map[block_no] =
                        create_nand_flash_file (block_no);
                write_nand_fiash_page_file (d->row_addr,
                    d->flash_map[block_no], d->write_buffer);
                d->write_offset = 0;
            } else if (((*data) & 0xff) == 0x85) {
                d->write_offset = 0;
                d->state = STATE_RANDOM_WRITE_START;
            } else
                assert (0);
            break;

        default:
            assert (0);
        }

        CPU_LOG ((cpu, "", " state %s\n", state_string[d->state]));

    } else if (offset == NAND_DATAPORT_OFFSET) {
        *has_set_value = FALSE;
        if (op_type == MTS_READ) {
            /*data port */
            CPU_LOG ((cpu, "",
                    "pc %x data %x d->read_offset %x d->data_port_ipr %x \n",
                    cpu->pc, *(d->data_port_ipr + d->read_offset),
                    d->read_offset, d->data_port_ipr));
            ret = (void *) (d->data_port_ipr + d->read_offset);
            d->read_offset++;
            return ret;

        } else if (op_type == MTS_WRITE) {
            ret = (void *) (d->write_buffer + d->col_addr + d->write_offset);
            d->write_offset++;

            return ret;
        }
        assert (0);

    } else if (offset == NAND_ADDRPORT_OFFSET) {
        CPU_LOG ((cpu, "", "ADDRESS  pc %x d->addr_offset %x *data %x \n",
                cpu->pc, d->addr_offset, *data));
        /*ADDRESS PORT */
        assert (op_type == MTS_WRITE);
        *has_set_value = TRUE;
        switch (d->addr_offset) {
        case 0x0:
            d->col_addr = (*data) & 0xff;
            break;
        case 0x01:
            d->col_addr += ((*data) & 0xff) << 8;
            break;
        case 0x2:
            d->row_addr = (*data) & 0xff;
            break;
        case 0x03:
            d->row_addr += ((*data) & 0xff) << 8;
            break;
        case 0x04:
            d->row_addr += ((*data) & 0xff) << 16;
            break;
        default:
            assert (0);
        }
        CPU_LOG ((cpu, "", "col_addr  %x row_addr %x\n", d->col_addr,
                d->row_addr));
        d->addr_offset++;
    }

    *has_set_value = TRUE;
    return NULL;

}

static int load_nand_flash_file (nand_flash_1g_data_t * d)
{
    int i, j = 0;
    struct dirent *ent = NULL;
    DIR *p_dir;
    char file_path[64];
    char *file_name;
    char block_number[16];
    int fd;
    //nand_flash_1g_data_t *d=*nand_flash;

    memset (d->flash_map, 0x0,
        NAND_FLASH_1G_TOTAL_BLOCKS * sizeof (d->flash_map[0]));
    p_dir = opendir (NAND_FLASH_1G_FILE_DIR);
    if (NULL == p_dir) {
        fprintf (stderr,
            "NAND FLASH: Can not open nand flash file directory \"%s\".\n",
            NAND_FLASH_1G_FILE_DIR);
        goto err_flash_map_create;
    }
    while (NULL != (ent = readdir (p_dir))) {
        //we only take care file
        if (ent->d_type == DT_DIR)
            continue;
        snprintf (file_path, sizeof (file_path), "%s/%s",
            NAND_FLASH_1G_FILE_DIR, ent->d_name);
        if (get_file_size (file_path) != NAND_FLASH_1G_BLOCK_SIZE)
            continue;
        file_name = strdup (ent->d_name);
        for (i = strlen (file_name) - 1; i >= 0; i--) {
            if (file_name[i] == '.')
                break;
        }
        if (i == -1) {
            //not a valid flash file
            continue;
        }
        file_name[i] = '\0';
        if (strcmp (file_name, NAND_FLASH_1G_FILE_PREFIX) != 0) {
            continue;
        }
        free (file_name);
        file_name = strdup (ent->d_name);
        //get the block number
        strncpy (block_number, file_name + i + 1, strlen (file_name) - i - 1);
        block_number[strlen (file_name) - i - 1] = '\0';
        i = atoi (block_number);
        fd = open (file_path, O_RDWR);
        if (fd < 0)
            goto err_open_flash_file;
        d->flash_map[i] = memzone_map_file (fd, NAND_FLASH_1G_BLOCK_SIZE);
        if (d->flash_map[i] == NULL)
            goto err_map_flash_file;
        close (fd);
        free (file_name);
        j++;

    }
    closedir (p_dir);
    printf ("\nloaded %d nand flash file from directory \"%s\". \n", j,
        NAND_FLASH_1G_FILE_DIR);
    return (0);
  err_map_flash_file:
    close (fd);
  err_open_flash_file:
    free (file_name);
  err_flash_map_create:
    return (-1);

}

void dev_nand_flash_1g_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    nand_flash_1g_data_t *d = dev->priv_data;
    memset (d->fake_block, 0xff, NAND_FLASH_1G_BLOCK_SIZE);
    d->state = STATE_INIT;
    memset (d->write_buffer, 0xff, NAND_FLASH_1G_PAGE_SIZE);

}

int dev_nand_flash_1g_init (vm_instance_t * vm, char *name, m_pa_t phys_addr,
    m_uint32_t phys_len, nand_flash_1g_data_t ** nand_flash)
{

    nand_flash_1g_data_t *d;

    /* allocate the private data structure */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "NAND FLASH: unable to create device.\n");
        return (-1);
    }
    memset (d, 0, sizeof (*d));

    /*load all flash data to d->flash_map */
    if (load_nand_flash_file (d) == -1)
        return (-1);
    /*set fake_page
     * We only create nand flash file when writing to a blcok.
     * When reading from a block which has not been written, give it the fake_page.
     */
    memset (d->fake_block, 0xff, NAND_FLASH_1G_BLOCK_SIZE);

    d->state = STATE_INIT;
    memset (d->write_buffer, 0xff, NAND_FLASH_1G_PAGE_SIZE);

    if (!(d->dev = dev_create (name)))
        goto err_dev_create;
    d->dev->priv_data = d;
    d->dev->handler = dev_nand_flash_1g_access;
    d->dev->reset_handler = dev_nand_flash_1g_reset;
    /*NAND COMMPORT AND DATA PORT ADDRESS */
    d->dev->phys_addr = phys_addr;
    d->dev->phys_len = phys_len;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    /* Map this device to the VM */
    vm_bind_device (vm, d->dev);

    *nand_flash = d;

    return (0);
  err_dev_create:
    free (d);
    return (-1);
}
