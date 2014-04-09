 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __DEV_NAND_FLASH_1G_H__
#define __DEV_NAND_FLASH_1G_H__

#include "utils.h"

#define NAND_FLASH_1G_BLOCK_PAGE_OFFSET  0x6    /*64 page in a blcok */
#define NAND_FLASH_1G_BLOCK_PAGE_MASK    0x3f
#define NAND_FLASH_1G_TOTAL_SIZE  0x42000000    /*1G data BYTES +32M bytes SPARE BYTES */
#define NAND_FLASH_1G_TOTAL_PLANE 4
#define NAND_FLASH_1G_TOTAL_PAGES 0x80000
#define NAND_FLASH_1G_TOTAL_BLOCKS 0x2000
#define NAND_FLASH_1G_PAGES_PER_BLOCK  0x40

#define NAND_FLASH_1G_PAGE_SIZE   0x840 /*2k bytes date size+64 bytes spare size */
#define NAND_FLASH_1G_SPARE_SIZE  0x40  /*64 bytes */
#define NAND_FLASH_1G_BLOCK_SIZE  0x21000       /*132k bytes */
#define NAND_FLASH_1G_PAGE_DATA_SIZE 0x800      /*2k bytes */
#define NAND_FLASH_1G_BLOCK_DATA_SIZE 0x20000   /*128k bytes */

#define NAND_FLASH_1G_BLOCK_OFFSET(x)    (NAND_FLASH_1G_BLOCK_SIZE*x)
#define NAND_FLASH_1G_PAGE_OFFSET(x)     (NAND_FLASH_1G_PAGE_SIZE*x)

#define NAND_FLASH_1G_FILE_PREFIX   	    "nandflash1GB"
#define NAND_FLASH_1G_FILE_DIR                  "nandflash1GB"

/*NAND_FLASH_STATE*/

#define STATE_INIT                                   0x0
#define STATE_READ_START                         0x1
#define STATE_RANDOM_READ_START         0x2
#define STATE_WRITE_START                       0x3
#define STATE_RANDOM_WRITE_START       0x4

#define STATE_READ_PAGE_FOR_COPY_WRITE       0x5
#define STATE_COPY_START                  0x6

#define STATE_ERASE_START                  0x7

#define NAND_DATAPORT_OFFSET	0x00000000
#define NAND_ADDRPORT_OFFSET	0x00010000
#define NAND_COMMPORT_OFFSET	0x00008000

/* nand flash private data */
struct nand_flash_1g_data {
    struct vdevice *dev;
    int state;
    unsigned char *flash_map[NAND_FLASH_1G_TOTAL_BLOCKS];

    unsigned char fake_block[NAND_FLASH_1G_BLOCK_SIZE];

    m_uint32_t row_addr;
    m_uint32_t col_addr;

    unsigned char write_buffer[NAND_FLASH_1G_PAGE_SIZE];        //for copy back
    m_uint32_t read_offset;
    m_uint32_t write_offset;
    m_uint32_t addr_offset;
    m_uint8_t has_issue_30h;
    /*for nand flash read */
    unsigned char *data_port_ipr;

};
typedef struct nand_flash_1g_data nand_flash_1g_data_t;

unsigned char *get_nand_flash_page_ptr (m_uint32_t row_addr,
    unsigned char *block_start);
int dev_nand_flash_1g_init (vm_instance_t * vm, char *name, m_pa_t phys_addr,
    m_uint32_t phys_len, nand_flash_1g_data_t ** nand_flash);

#endif
