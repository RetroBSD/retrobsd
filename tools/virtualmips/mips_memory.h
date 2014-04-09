/*
 * Cisco router simulation platform.
 * Copyright (c) 2006 Christophe Fillot (cf@utc.fr)
 */

 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef __MIPS64_MEM_H__
#define __MIPS64_MEM_H__

#include <sys/types.h>
#include "utils.h"
#include "system.h"
/* MTS operation */
#define MTS_READ        0
#define MTS_WRITE       1

#define MTS_BYTE        1
#define MTS_HALF_WORD   2
#define MTS_WORD        4

/* 0.5GB value */
#define MTS_SIZE_512M   0x20000000

/* MTS flag bits: D (device), ACC (memory access), C (chain) */
#define MTS_FLAG_BITS   4
#define MTS_FLAG_MASK   0x0000000fUL

/* Masks for MTS entries */
#define MTS_CHAIN_MASK  0x00000001
#define MTS_ACC_MASK    0x00000006
#define MTS_DEV_MASK    0x00000008
#define MTS_ADDR_MASK   (~MTS_FLAG_MASK)

/* Device ID mask and shift, device offset mask */
#define MTS_DEVID_MASK  0xfc000000
#define MTS_DEVID_SHIFT 26
#define MTS_DEVOFF_MASK 0x03fffff0

/* Memory access flags */
#define MTS_ACC_OK      0x00000000  /* Access OK */
#define MTS_ACC_AE      0x00000002  /* Address Error */
#define MTS_ACC_T       0x00000004  /* TLB Exception */
#define MTS_ACC_U       0x00000006  /* Unexistent */
#define MTS_ACC_M       0x00000008  /* TLB MODE */

/* Hash table size for MTS64 (default: [shift:16,bits:12]) */
#define MTS64_HASH_SHIFT        12
#define MTS64_HASH_BITS         14
#define MTS64_HASH_SIZE         (1 << MTS64_HASH_BITS)
#define MTS64_HASH_MASK         (MTS64_HASH_SIZE - 1)

/* MTS64 hash on virtual addresses */
#define MTS64_HASH(vaddr)       (((vaddr) >> MTS64_HASH_SHIFT) & MTS64_HASH_MASK)

/* Hash table size for MTS32 (default: [shift:15,bits:15]) */
#define MTS32_HASH_SHIFT        12
#define MTS32_HASH_BITS         14
#define MTS32_HASH_SIZE         (1 << MTS32_HASH_BITS)
#define MTS32_HASH_MASK         (MTS32_HASH_SIZE - 1)

/* MTS32 hash on virtual addresses */
#define MTS32_HASH(vaddr)       (((vaddr) >> MTS32_HASH_SHIFT) & MTS32_HASH_MASK)

/* Number of entries per chunk */
#define MTS64_CHUNK_SIZE        256
#define MTS32_CHUNK_SIZE        256

/* MTS64: chunk definition */
struct mts64_chunk {
    mts64_entry_t entry[MTS64_CHUNK_SIZE];
    struct mts64_chunk *next;
    u_int count;
};

/* MTS32: chunk definition */
struct mts32_chunk {
    mts32_entry_t entry[MTS32_CHUNK_SIZE];
    struct mts32_chunk *next;
    u_int count;
};

/*check whether vaddr need map*/
static int forced_inline vaddr_mapped (m_va_t vaddr)
{
    int zone = (vaddr >> 29) & 0x7;
    if ((zone == 0x4) || (zone == 0x5)) {
        return 0;
    } else {
        return 1;
    }
}

/* Shutdown the MTS subsystem */
void mips_mem_shutdown (cpu_mips_t * cpu);

/* Set the address mode */
int mips_set_addr_mode (cpu_mips_t * cpu, u_int addr_mode);

void physmem_dma_transfer (vm_instance_t * vm, m_pa_t src, m_pa_t dst,
    size_t len);
void *physmem_get_hptr (vm_instance_t * vm, m_pa_t paddr, u_int op_size,
    u_int op_type, m_uint32_t * data);

#endif
