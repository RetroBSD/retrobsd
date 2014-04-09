/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 */

 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <libelf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mips_memory.h"
#include "mips_exec.h"
#include "mips.h"
#include "vm.h"
#include "utils.h"
#include "system.h"
#include "mips_cp0.h"
#include "mips_jit.h"

#define GDB_SR            32
#define GDB_LO            33
#define GDB_HI            34
#define GDB_BAD         35
#define GDB_CAUSE    36
#define GDB_PC           37

/* MIPS general purpose registers names */
char *mips_gpr_reg_names[MIPS64_GPR_NR] = {
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

/* Cacheability and Coherency Attribute */
static int cca_cache_status[8] = {
    1, 1, 0, 1, 0, 1, 0, 0,
};

/* Get register index given its name */
int mips_get_reg_index (char *name)
{
    int i;

    for (i = 0; i < MIPS64_GPR_NR; i++)
        if (!strcmp (mips_gpr_reg_names[i], name))
            return (i);

    return (-1);
}

/* Get cacheability info */
int mips_cca_cached (m_uint8_t val)
{
    return (cca_cache_status[val & 0x03]);
}

/* Set a register */
void mips_reg_set (cpu_mips_t * cpu, u_int reg, m_reg_t val)
{
    if (reg == 0 || reg >= MIPS64_GPR_NR)
        return;
    cpu->gpr[reg] = val;

    if (cpu->vm->debug_level > 2 || (cpu->vm->debug_level > 1 &&
        (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_UM) &&
        ! (cpu->cp0.reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)))
    {
        /* Print GPR values in user mode. */
        printf ("        $%d := %08x \n", reg, val);
    }
}

/*get register value giving index. For GDB*/
int mips_reg_get (cpu_mips_t * cpu, u_int reg, m_reg_t * val)
{
    if (reg < MIPS64_GPR_NR) {
        *val = cpu->gpr[reg];
        return SUCCESS;
    } else {
        switch (reg) {
        case GDB_SR:
            *val = cpu->cp0.reg[MIPS_CP0_STATUS];
            break;
        case GDB_LO:
            *val = cpu->lo;
            break;
        case GDB_HI:
            *val = cpu->hi;
            break;
        case GDB_BAD:
            *val = cpu->cp0.reg[MIPS_CP0_BADVADDR];
            break;
        case GDB_CAUSE:
            *val = cpu->cp0.reg[MIPS_CP0_CAUSE];
            break;
        case GDB_PC:
            *val = cpu->pc;
            break;
        default:
            return FAILURE;
        }

    }

    return SUCCESS;

}

/* Delete a MIPS64 processor */
void mips_delete (cpu_mips_t * cpu)
{
    if (cpu) {
        mips_mem_shutdown (cpu);
    }
}

/* Reset a MIPS64 CPU */
int mips_reset (cpu_mips_t * cpu)
{
    cpu->cp0.reg[MIPS_CP0_STATUS] = MIPS_CP0_STATUS_BEV;
    cpu->cp0.reg[MIPS_CP0_CAUSE] = 0;
    cpu->cp0.ebase_reg = 0x80000000;

    /* Clear the complete TLB */
    memset (&cpu->cp0.tlb, 0, MIPS64_TLB_MAX_ENTRIES * sizeof (tlb_entry_t));

    /* Restart the MTS subsystem */
    mips_set_addr_mode (cpu, 32 /*64 */ );      /* zzz */
    cpu->mts_rebuild (cpu);

    /* Flush JIT structures */
    //mips_jit_flush(cpu,0);
    return (0);
}

/* Initialize a MIPS64 processor */
int mips_init (cpu_mips_t * cpu)
{
    /* Set the CPU methods */
    cpu->reg_get = (void *) mips_reg_get;
    cpu->reg_set = (void *) mips_reg_set;

    /* Set the startup parameters */
    mips_reset (cpu);
    return (0);
}

/* Load an ELF image into the simulated memory. Using libelf*/
int mips_load_elf_image (cpu_mips_t * cpu, char *filename,
    m_va_t * entry_point)
{
    m_va_t vaddr;
    m_uint32_t remain;
    void *haddr;
    Elf32_Ehdr *ehdr;
    Elf32_Shdr *shdr;
    Elf_Scn *scn;
    Elf *img_elf;
    size_t len, clen;
    int i, fd;
    FILE *bfd;

    if (! filename)
        return (-1);

#ifdef __CYGWIN__
    fd = open (filename, O_RDONLY | O_BINARY);
#else
    fd = open (filename, O_RDONLY);
#endif
    printf ("Loading ELF file '%s'...\n", filename);
    if (fd == -1) {
        perror ("load_elf_image: open");
        return (-1);
    }

    if (elf_version (EV_CURRENT) == EV_NONE) {
        fprintf (stderr, "load_elf_image: library out of date\n");
        return (-1);
    }

    if (!(img_elf = elf_begin (fd, ELF_C_READ, NULL))) {
        fprintf (stderr, "load_elf_image: elf_begin: %s\n",
            elf_errmsg (elf_errno ()));
        return (-1);
    }

    if (!(ehdr = elf32_getehdr (img_elf))) {
        fprintf (stderr, "load_elf_image: invalid ELF file\n");
        return (-1);
    }

    bfd = fdopen (fd, "rb");

    if (!bfd) {
        perror ("load_elf_image: fdopen");
        return (-1);
    }
// if (!skip_load) {
    for (i = 0; i < ehdr->e_shnum; i++) {
        scn = elf_getscn (img_elf, i);

        shdr = elf32_getshdr (scn);
        len = shdr->sh_size;

        if (!(shdr->sh_flags & SHF_ALLOC) || !len)
            continue;

        fseek (bfd, shdr->sh_offset, SEEK_SET);
        vaddr = sign_extend (shdr->sh_addr, 32);

        if (cpu->vm->debug_level > 0) {
            printf ("   * Adding section at virtual address 0x%8.8" LL "x "
                "(len=0x%8.8lx)\n", vaddr & 0xFFFFFFFF, (u_long) len);
        }

        while (len > 0) {
            haddr = cpu->mem_op_lookup (cpu, vaddr);

            if (!haddr) {
                fprintf (stderr,
                    "load_elf_image: invalid load address 0x%" LL "x\n",
                    vaddr);
                return (-1);
            }

            if (len > MIPS_MIN_PAGE_SIZE)
                clen = MIPS_MIN_PAGE_SIZE;
            else
                clen = len;

            remain = MIPS_MIN_PAGE_SIZE;
            remain -= (vaddr - (vaddr & MIPS_MIN_PAGE_SIZE));

            clen = m_min (clen, remain);

            if (fread ((u_char *) haddr, clen, 1, bfd) < 1)
                break;

            vaddr += clen;
            len -= clen;
        }
    }

    printf ("ELF entry point: 0x%x\n", ehdr->e_entry);

    if (entry_point)
        *entry_point = ehdr->e_entry;

    elf_end (img_elf);
    fclose (bfd);
    return (0);
}

/* Update the IRQ flag (inline) */
static forced_inline fastcall int mips_update_irq_flag_fast (cpu_mips_t *
    cpu)
{
    mips_cp0_t *cp0 = &cpu->cp0;
    m_uint32_t cause;
    cpu->irq_pending = FALSE;

    cause = cp0->reg[MIPS_CP0_CAUSE] & ~MIPS_CP0_CAUSE_IMASK;
    cp0->reg[MIPS_CP0_CAUSE] = cause | cpu->irq_cause;

/*printf ("(%08x-%08x) ", cpu->pc, cp0->reg[MIPS_CP0_STATUS]); fflush (stdout);*/
    if ((cp0->reg[MIPS_CP0_STATUS] & (MIPS_CP0_STATUS_IE |
        MIPS_CP0_STATUS_EXL | MIPS_CP0_STATUS_ERL)) == MIPS_CP0_STATUS_IE) {
#ifdef SIM_PIC32
        m_uint32_t current_ipl = cp0->reg[MIPS_CP0_STATUS] >> 10 & 63;
        m_uint32_t requested_ipl = cp0->reg[MIPS_CP0_CAUSE] >> 10 & 63;
/*printf ("(%d-%d) ", requested_ipl, current_ipl); fflush (stdout);*/
        if (unlikely (requested_ipl > current_ipl)) {
            cpu->irq_pending = TRUE;
            return (TRUE);
        }
#else
        m_uint32_t imask = cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_IMASK;
        if (unlikely (cp0->reg[MIPS_CP0_CAUSE] & imask)) {
            cpu->irq_pending = TRUE;
            return (TRUE);
        }
#endif
    }
    return (FALSE);
}

/* Update the IRQ flag */
int fastcall mips_update_irq_flag (cpu_mips_t * cpu)
{
    return mips_update_irq_flag_fast (cpu);
}

#if SIM_PIC32
static void print_arg (val) {
    if (val & 0xff000000)
        printf ("%08x", val);
    else
        printf ("%u", val);
}

static void print_args (narg, arg0, arg1, arg2, arg3, arg4, arg5)
{
    print_arg (arg0);
    if (narg > 1) { printf (", "); print_arg (arg1); }
    if (narg > 2) { printf (", "); print_arg (arg2); }
    if (narg > 3) { printf (", "); print_arg (arg3); }
    if (narg > 4) { printf (", "); print_arg (arg4); }
    if (narg > 5) { printf (", "); print_arg (arg5); }
}

/*
 * Print trace information for exception.
 * For syscalls, display name and arguments.
 */
static void print_exception (cpu_mips_t * cpu, u_int exc_code)
{
	const char *code = 0;
#include "bsd_syscalls.h"

	if (exc_code == MIPS_CP0_CAUSE_SYSCALL) {
	    mips_insn_t code;

        if (mips_fetch_instruction (cpu, cpu->pc, &code) != 0) {
            printf ("--- syscall at %08x: cannot fetch instruction opcode\n", cpu->pc);
            return;
        }
        /* bottom 8 bits are index */
		code = (code >> 6) & 0377;
if (code == 97) cpu->vm->debug_level = 2;
if (code == 1) cpu->vm->debug_level = 1;
        cpu->trace_syscall = code;
		if (code >= sizeof (bsd_syscalls) / sizeof (bsd_syscalls[0])) {
            printf ("--- syscall: #%d at %08x\n", (int)code, cpu->pc);
            return;
		}
        printf ("--- syscall: %s (", bsd_syscalls[code].name);
        if (bsd_syscalls[code].narg > 0) {
            m_reg_t arg4 = 0, arg5 = 0;
            void *haddr;
            u_int exc;
            m_uint8_t has_set_value;

            if (bsd_syscalls[code].narg >= 4) {
                has_set_value = FALSE;
                haddr = mips_mts32_access (cpu, cpu->gpr[MIPS_GPR_SP] + 16,
                    MIPS_MEMOP_LW, 4, MTS_READ, &arg4, &exc, &has_set_value, 0);
                if (exc || (! haddr && ! has_set_value))
                    arg4 = 0;
                else if (! has_set_value)
                    arg4 = vmtoh32 (*(m_uint32_t *) haddr);
            }
            if (bsd_syscalls[code].narg >= 5) {
                has_set_value = FALSE;
                haddr = mips_mts32_access (cpu, cpu->gpr[MIPS_GPR_SP] + 20,
                    MIPS_MEMOP_LW, 4, MTS_READ, &arg5, &exc, &has_set_value, 0);
                if (exc || (! haddr && ! has_set_value))
                    arg5 = 0;
                else if (! has_set_value)
                    arg5 = vmtoh32 (*(m_uint32_t *) haddr);
            }
            print_args (bsd_syscalls[code].narg,
                cpu->gpr[MIPS_GPR_A0], cpu->gpr[MIPS_GPR_A1],
                cpu->gpr[MIPS_GPR_A2], cpu->gpr[MIPS_GPR_A3], arg4, arg5);
        }
        printf (") at %08x\n", cpu->pc);
	    return;
	}
    printf ("\n*** 0x%08x: exception ", cpu->pc);

	switch (exc_code) {
	case MIPS_CP0_CAUSE_INTERRUPT:	code = "Interrupt"; break;
	case MIPS_CP0_CAUSE_ADDR_LOAD:	code = "Address Load"; break;
	case MIPS_CP0_CAUSE_ADDR_SAVE:	code = "Address Save"; break;
	case MIPS_CP0_CAUSE_BUS_INSTR:	code = "Bus fetch"; break;
	case MIPS_CP0_CAUSE_BUS_DATA:	code = "Bus load/store"; break;
	case MIPS_CP0_CAUSE_SYSCALL:	code = "Syscall"; break;
	case MIPS_CP0_CAUSE_BP:         code = "Breakpoint"; break;
	case MIPS_CP0_CAUSE_ILLOP:      code = "Reserved Instruction"; break;
	case MIPS_CP0_CAUSE_CP_UNUSABLE:code = "Coprocessor Unusable"; break;
	case MIPS_CP0_CAUSE_OVFLW:      code = "Arithmetic Overflow"; break;
	case MIPS_CP0_CAUSE_TRAP:       code = "Trap"; break;
	}
	if (code)
		printf ("'%s'\n", code);
	else
		printf ("%d\n", exc_code);

	switch (exc_code) {
	case MIPS_CP0_CAUSE_ADDR_LOAD:
	case MIPS_CP0_CAUSE_ADDR_SAVE:
        printf ("*** badvaddr = 0x%08x\n", cpu->cp0.reg[MIPS_CP0_BADVADDR]);
        break;
    }
	printf ("                t0 = %8x   s0 = %8x   t8 = %8x   lo = %8x\n",
		cpu->gpr[8], cpu->gpr[16], cpu->gpr[24], cpu->lo);
	printf ("at = %8x   t1 = %8x   s1 = %8x   t9 = %8x   hi = %8x\n",
		cpu->gpr[1], cpu->gpr[9], cpu->gpr[17], cpu->gpr[25], cpu->hi);
	printf ("v0 = %8x   t2 = %8x   s2 = %8x               status = %8x\n",
		cpu->gpr[2], cpu->gpr[10], cpu->gpr[18], cpu->cp0.reg[MIPS_CP0_STATUS]);
	printf ("v1 = %8x   t3 = %8x   s3 = %8x                cause = %8x\n",
		cpu->gpr[3], cpu->gpr[11],cpu->gpr[19], cpu->cp0.reg[MIPS_CP0_CAUSE]);
	printf ("a0 = %8x   t4 = %8x   s4 = %8x   gp = %8x  epc = %8x\n",
		cpu->gpr[4], cpu->gpr[12], cpu->gpr[20], cpu->gpr[MIPS_GPR_GP], cpu->pc);
	printf ("a1 = %8x   t5 = %8x   s5 = %8x   sp = %8x\n",
		cpu->gpr[5], cpu->gpr[13], cpu->gpr[21], cpu->gpr[MIPS_GPR_SP]);
	printf ("a2 = %8x   t6 = %8x   s6 = %8x   fp = %8x\n",
		cpu->gpr[6], cpu->gpr[14], cpu->gpr[22], cpu->gpr[MIPS_GPR_FP]);
	printf ("a3 = %8x   t7 = %8x   s7 = %8x   ra = %8x\n",
		cpu->gpr[7], cpu->gpr[15], cpu->gpr[23], cpu->gpr[MIPS_GPR_RA]);
}
#endif

/* Generate an exception */
void mips_trigger_exception (cpu_mips_t * cpu, u_int exc_code, int bd_slot)
{
    mips_cp0_t *cp0 = &cpu->cp0;
    m_uint64_t new_pc;
    m_reg_t cause;

    /* keep IM, set exception code and bd slot */
    cause = cp0->reg[MIPS_CP0_CAUSE];

    /* we don't set EPC if EXL is set */
    if (!(cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)) {
        cp0->reg[MIPS_CP0_EPC] = cpu->pc;
        /*Cause BD is not update. MIPS VOLUME  V3 P65 */
        cause &= ~MIPS_CP0_CAUSE_BD_SLOT;       //clear bd
        if (bd_slot)
            cause |= MIPS_CP0_CAUSE_BD_SLOT;
        else
            cause &= ~MIPS_CP0_CAUSE_BD_SLOT;

    }

    cause &= ~MIPS_CP0_CAUSE_EXC_MASK;  //clear exec-code
    cause |= (exc_code << 2);
    cp0->reg[MIPS_CP0_CAUSE] = cause;
    if (exc_code == MIPS_CP0_CAUSE_INTERRUPT)
        cpu->irq_cause = 0;

    /* Set EXL bit in status register */
    /*TODO: RESET SOFT RESET AND NMI EXCEPTION */
    cp0->reg[MIPS_CP0_STATUS] |= MIPS_CP0_STATUS_EXL;

    /* clear ERL bit in status register */
    /*TODO: RESET SOFT RESET AND NMI EXCEPTION */
    cp0->reg[MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_ERL;

    if (cpu->vm->debug_level > 2) {
        printf ("        exception %u at %08x\n", exc_code, cpu->pc);
    }
    if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_BEV) {
        if ((exc_code == MIPS_CP0_CAUSE_TLB_LOAD)
            || (exc_code == MIPS_CP0_CAUSE_TLB_SAVE)) {
            if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)
                new_pc = 0xffffffffbfc00380ULL;
            else
                new_pc = 0xffffffffbfc00200ULL;
        } else if (exc_code == MIPS_CP0_CAUSE_INTERRUPT) {
            if (cp0->reg[MIPS_CP0_CAUSE] & MIPS_CP0_CAUSE_IV)
                new_pc = 0xffffffffbfc00400ULL;
            else
                new_pc = 0xffffffffbfc00380ULL;

        } else
            new_pc = 0xffffffffbfc00380ULL;

    } else {
#if SIM_PIC32
        /* TODO */
        new_pc = cp0->ebase_reg + 0x200;
        if (exc_code != MIPS_CP0_CAUSE_INTERRUPT &&
            (exc_code != MIPS_CP0_CAUSE_SYSCALL || cpu->vm->debug_level > 0))
            print_exception (cpu, exc_code);
#else
        if ((exc_code == MIPS_CP0_CAUSE_TLB_LOAD)
            || (exc_code == MIPS_CP0_CAUSE_TLB_SAVE)) {
            if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)
                new_pc = 0xffffffff80000180ULL;
            else
                new_pc = 0xffffffff80000000ULL;
        } else if (exc_code == MIPS_CP0_CAUSE_INTERRUPT) {
            if (cp0->reg[MIPS_CP0_CAUSE] & MIPS_CP0_CAUSE_IV)
                new_pc = 0xffffffff80000200ULL;
            else
                new_pc = 0xffffffff80000180ULL;
        } else
            new_pc = 0xffffffff80000180ULL;
#endif
    }

    cpu->pc = (m_va_t) new_pc;

    /* Clear the pending IRQ flag */
    cpu->irq_pending = 0;
}

/* Execute fpu instruction */
void fastcall mips_exec_soft_fpu (cpu_mips_t * cpu)
{
    mips_cp0_t *cp0 = &cpu->cp0;
    cp0->reg[MIPS_CP0_CAUSE] |= 0x10000000;     //CE=1
    mips_trigger_exception (cpu, MIPS_CP0_CAUSE_CP_UNUSABLE,
        cpu->is_in_bdslot);

}

/* Execute ERET instruction */
void fastcall mips_exec_eret (cpu_mips_t * cpu)
{
    mips_cp0_t *cp0 = &cpu->cp0;

    if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_ERL) {
        cp0->reg[MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_ERL;
        cpu->pc = cp0->reg[MIPS_CP0_ERR_EPC];

    } else {
        cp0->reg[MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_EXL;
        cpu->pc = cp0->reg[MIPS_CP0_EPC];
    }
    /* We have to clear the LLbit */
    cpu->ll_bit = 0;

}

/* Execute BREAK instruction */
void fastcall mips_exec_break (cpu_mips_t * cpu, u_int code)
{
    //mips_dump_regs(cpu);
    printf ("exec break cpu->pc %x\n", cpu->pc);

    /* XXX TODO: Branch Delay slot */
    mips_trigger_exception (cpu, MIPS_CP0_CAUSE_BP, 0);

}

/* Trigger a Trap Exception */
void fastcall mips_trigger_trap_exception (cpu_mips_t * cpu)
{
    /* XXX TODO: Branch Delay slot */
    printf ("MIPS64: TRAP exception, CPU=%p\n", cpu);
    mips_trigger_exception (cpu, MIPS_CP0_CAUSE_TRAP, 0);
}

/* Execute SYSCALL instruction */
void fastcall mips_exec_syscall (cpu_mips_t * cpu)
{
#if DEBUG_SYSCALL
    printf ("MIPS: SYSCALL at PC=0x%" LL "x (RA=0x%" LL "x)\n"
        "   a0=0x%" LL "x, a1=0x%" LL "x, a2=0x%" LL "x, a3=0x%" LL "x\n",
        cpu->pc, cpu->gpr[MIPS_GPR_RA],
        cpu->gpr[MIPS_GPR_A0], cpu->gpr[MIPS_GPR_A1], cpu->gpr[MIPS_GPR_A2],
        cpu->gpr[MIPS_GPR_A3]);
#endif

    if (cpu->is_in_bdslot == 0)
        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_SYSCALL, 0);
    else
        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_SYSCALL, 1);
}

/* Trigger IRQs */
void forced_inline fastcall mips_trigger_irq (cpu_mips_t * cpu)
{
    if (mips_update_irq_flag (cpu))
        mips_trigger_exception (cpu, MIPS_CP0_CAUSE_INTERRUPT, 0);
}

/* Set an IRQ */
void mips_set_irq (cpu_mips_t * cpu, m_uint8_t irq)
{
    m_uint32_t m;
    m = (1 << (irq + MIPS_CP0_CAUSE_ISHIFT)) & MIPS_CP0_CAUSE_IMASK;
    //atomic_or(&cpu->irq_cause,m);
    cpu->irq_cause |= m;
}

/* Clear an IRQ */
void mips_clear_irq (cpu_mips_t * cpu, m_uint8_t irq)
{
    m_uint32_t m;

    m = (1 << (irq + MIPS_CP0_CAUSE_ISHIFT)) & MIPS_CP0_CAUSE_IMASK;
    cpu->irq_cause &= ~m;
    //atomic_and(&cpu->irq_cause,~m);

    if (! cpu->irq_cause)
        cpu->irq_pending = 0;
}
