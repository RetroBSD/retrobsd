/*
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 *
 */

/*
JIT Support.
In order to improve the simulation speed, JIT is always used in simulators.
Unlike the JIT of high language, virtualmips does not have basic block.
It translates one page(4K) code one time and then run it.

Problems:

1. code fetch exception
2. delay slot
3. interrupt emulation
4. Self-modify code
5. debug JIT

How to debug JIT is the most tricky problem.
I do not have better method, but just logging and comparing with
interpreter. Thanks to the file diff tools.

If you know how to debug JIT, please tell me.

Most of the code is stripped from Dynamips.  Thanks Christophe Fillot.

Many optimation chances exist.
1. add asid as a hash key of translated block so that translated block is not fushed too often.
2. A better way to find translated block. red-block tree??
3. peephole optimation???
4. Use profile tools such as vtune to find the hot path and then optimize it.


*/

#ifndef __MIPS64_JIT_H__
#define __MIPS64_JIT_H__

#include "system.h"
#include "utils.h"
#include "sbox.h"
#include "mips.h"
#include "mips_memory.h"

#ifndef _USE_JIT_
#define JIT_SUPPORT 0
#endif

#ifdef _USE_JIT_

#define JIT_SUPPORT 1

/* Size of executable page area (in Mb) */
#ifndef __CYGWIN__
#define MIPS_EXEC_AREA_SIZE  64
#else
#define MIPS_EXEC_AREA_SIZE  16
#endif

/* Buffer size for JIT code generation */
#define MIPS_JIT_BUFSIZE     32768

/* Maximum number of X86 chunks */
#define MIPS_JIT_MAX_CHUNKS  32

/* Size of hash for PC lookup */
#define MIPS_JIT_PC_HASH_BITS   16
#define MIPS_JIT_PC_HASH_MASK   ((1 << MIPS_JIT_PC_HASH_BITS) - 1)
#define MIPS_JIT_PC_HASH_SIZE   (1 << MIPS_JIT_PC_HASH_BITS)

/* Instruction jump patch */
struct mips_insn_patch {
    u_char *jit_insn;
    m_uint64_t mips_pc;
};

/* Instruction patch table */
#define MIPS64_INSN_PATCH_TABLE_SIZE  32

struct mips_jit_patch_table {
    struct mips_insn_patch patches[MIPS64_INSN_PATCH_TABLE_SIZE];
    u_int cur_patch;
    struct mips_jit_patch_table *next;
};

/* Host executable page */
struct insn_exec_page {
    u_char *ptr;
    insn_exec_page_t *next;
};

/* MIPS64 translated code block */
struct mips_jit_tcb {
    /*start pc in tcb */
    m_va_t start_pc;
    m_uint32_t asid;
    m_uint32_t acc_count;
    /*guest pc to host pc mapping table */
    u_char **jit_insn_ptr;
    /*guest code of this tcb */
    mips_insn_t *mips_code;
    u_int mips_trans_pos;
    u_int jit_chunk_pos;
    /*used in translating */
    u_char *jit_ptr;
    insn_exec_page_t *jit_buffer;
    insn_exec_page_t *jit_chunks[MIPS_JIT_MAX_CHUNKS];
    struct mips_jit_patch_table *patch_table;
    mips_jit_tcb_t *prev, *next;
};

int mips_jit_init (cpu_mips_t * cpu);
int mips_jit_flush (cpu_mips_t * cpu, m_uint32_t threshold);
void mips_jit_shutdown (cpu_mips_t * cpu);
int mips_jit_fetch_and_emit (cpu_mips_t * cpu,
    mips_jit_tcb_t * block, int delay_slot);

int mips_jit_tcb_record_patch (mips_jit_tcb_t * block, u_char * jit_ptr,
    m_va_t vaddr);
void mips_jit_tcb_free (cpu_mips_t * cpu, mips_jit_tcb_t * block,
    int list_removal);

void *mips_jit_run_cpu (cpu_mips_t * cpu);

/*-----------inline functions-----------------------------*/
/* Get the JIT instruction pointer in a translated block */
static forced_inline
    u_char * mips_jit_tcb_get_host_ptr (mips_jit_tcb_t * b, m_va_t vaddr)
{
    m_uint32_t offset;

    offset = ((m_uint32_t) vaddr & MIPS_MIN_PAGE_IMASK) >> 2;
    return (b->jit_insn_ptr[offset]);
}

/* Check if the specified address belongs to the specified block */
static forced_inline
    int mips_jit_tcb_local_addr (mips_jit_tcb_t * block, m_va_t vaddr,
    u_char ** jit_addr)
{
    if ((vaddr & MIPS_MIN_PAGE_MASK) == block->start_pc) {
        *jit_addr = mips_jit_tcb_get_host_ptr (block, vaddr);
        return (1);
    }

    return (0);
}

extern int test33;

/* Check if PC register matches the compiled block virtual address */
static forced_inline
    int mips_jit_tcb_match (cpu_mips_t * cpu, mips_jit_tcb_t * block,
    m_va_t vaddr)
{
    m_va_t vpage;

    // vpage = cpu->pc & ~(m_va_t)MIPS_MIN_PAGE_IMASK;
    vpage = vaddr & ~(m_va_t) MIPS_MIN_PAGE_IMASK;
    if (block->start_pc != vpage)
        return 0;
    /*block->start_pc == vpage */
    if (!vaddr_mapped (vaddr))
        return 1;
    /*block->start_pc == vpage and mapped.check asid */
    int asid = cpu->cp0.reg[MIPS_CP0_TLB_HI] & MIPS_TLB_ASID_MASK;

    return (block->asid == asid);
}

/* Compute the hash index for the specified PC value */
/*TODO: Add asid as a hash key.
Currently same pc of different asid will get the same hash value.
*/
static forced_inline m_uint32_t mips_jit_get_pc_hash (cpu_mips_t * cpu,
    m_va_t pc)
{
    m_uint32_t page_hash;

    page_hash = sbox_u32 (pc >> MIPS_MIN_PAGE_SHIFT);
    return ((page_hash ^ (page_hash >> 12)) & MIPS_JIT_PC_HASH_MASK);

}

/*if the code write to code region, flush the translated page*/
static forced_inline void jit_handle_self_write (cpu_mips_t * cpu,
    m_va_t vaddr)
{
    m_uint32_t pc_hash;
    mips_jit_tcb_t *block;

    pc_hash = mips_jit_get_pc_hash (cpu, vaddr);
    block = cpu->exec_blk_map[pc_hash];
    if (block != NULL) {
        if (unlikely (mips_jit_tcb_match (cpu, block, vaddr))) {
            mips_jit_tcb_free (cpu, block, TRUE);
            cpu->exec_blk_map[pc_hash] = NULL;
        }
    }
}

/*whether instruction is a jump instruction*/
static forced_inline int insn_is_jmp (unsigned int insn)
{
    /*can insn be in delay slot */
    int op = MAJOR_OP (insn);
    uint16_t special_func;
    switch (op) {
    case 0x0:
        {
            special_func = bits (insn, 0, 5);
            switch (special_func) {
            case 0x8:
            case 0x9:
                return 1;
            default:
                return 0;
            }

        }
    case 0x1:
        {
            special_func = bits (insn, 16, 20);
            switch (special_func) {
            case 0x0:
            case 0x1:
            case 0x2:
            case 0x3:
            case 0x4:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
                return 1;
            default:
                return 0;
            }

        }
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
        return 1;
    default:
        return 0;

    }

}

/* Check if an instruction is in a delay slot or not */
static forced_inline int mips_jit_is_delay_slot (mips_jit_tcb_t * b,
    m_va_t pc)
{
    m_uint32_t offset, insn;

    offset = (pc - b->start_pc) >> 2;

    if (!offset)
        return (FALSE);
    ASSERT (b->mips_code != NULL, "b->mips_code can not be NULL\n");
    /* Fetch the previous instruction to determine if it is a jump */
    insn = vmtoh32 (b->mips_code[offset - 1]);
    return insn_is_jmp (insn);
}
#endif

#endif
