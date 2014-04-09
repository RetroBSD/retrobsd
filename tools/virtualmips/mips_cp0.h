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

#ifndef __CP0_H__
#define __CP0_H__

#include "utils.h"
#include "system.h"

/* CP0 register names */
extern char *mips_cp0_reg_names[];

m_cp0_reg_t mips_cp0_get_reg (cpu_mips_t * cpu, u_int cp0_reg);
void fastcall mips_cp0_exec_mfc0_fastcall (cpu_mips_t * cpu,
    mips_insn_t insn);
void mips_cp0_exec_mfc0 (cpu_mips_t * cpu, u_int gp_reg, u_int cp0_reg,
    u_int sel);
void mips_cp0_exec_mtc0 (cpu_mips_t * cpu, u_int gp_reg, u_int cp0_reg,
    u_int sel);
void fastcall mips_cp0_exec_mtc0_fastcall (cpu_mips_t * cpu,
    mips_insn_t insn);
void mips_cp0_set_reg (cpu_mips_t * cpu, u_int cp0_reg, u_int sel,
    m_uint32_t val);
m_cp0_reg_t mips_cp0_get_vpn2_mask (cpu_mips_t * cpu);
void fastcall mips_cp0_exec_tlbp (cpu_mips_t * cpu);
void fastcall mips_cp0_exec_tlbwi (cpu_mips_t * cpu);
void fastcall mips_cp0_exec_tlbwr (cpu_mips_t * cpu);
void fastcall mips_cp0_exec_tlbr (cpu_mips_t * cpu);
int mips_cp0_tlb_lookup (cpu_mips_t * cpu, m_va_t vaddr, mts_map_t * res);
void mips_tlb_dump (cpu_mips_t * cpu);
void mips_tlb_raw_dump (cpu_mips_t * cpu);

#endif
