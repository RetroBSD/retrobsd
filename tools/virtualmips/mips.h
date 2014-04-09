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

#ifndef __MIPS_H__
#define __MIPS_H__
#include "system.h"
#include "utils.h"

/*
 * MIPS General Purpose Registers
 */
#define MIPS_GPR_ZERO        0  /*  zero  */
#define MIPS_GPR_AT          1  /*  at  */
#define MIPS_GPR_V0          2  /*  v0  */
#define MIPS_GPR_V1          3  /*  v1  */
#define MIPS_GPR_A0          4  /*  a0  */
#define MIPS_GPR_A1          5  /*  a1  */
#define MIPS_GPR_A2          6  /*  a2  */
#define MIPS_GPR_A3          7  /*  a3  */
#define MIPS_GPR_T0          8  /*  t0  */
#define MIPS_GPR_T1          9  /*  t1  */
#define MIPS_GPR_T2          10 /*  t2  */
#define MIPS_GPR_T3          11 /*  t3  */
#define MIPS_GPR_T4          12 /*  t4  */
#define MIPS_GPR_T5          13 /*  t5  */
#define MIPS_GPR_T6          14 /*  t6  */
#define MIPS_GPR_T7          15 /*  t7  */
#define MIPS_GPR_S0          16 /*  s0  */
#define MIPS_GPR_S1          17 /*  s1  */
#define MIPS_GPR_S2          18 /*  s2  */
#define MIPS_GPR_S3          19 /*  s3  */
#define MIPS_GPR_S4          20 /*  s4  */
#define MIPS_GPR_S5          21 /*  s5  */
#define MIPS_GPR_S6          22 /*  s6  */
#define MIPS_GPR_S7          23 /*  s7  */
#define MIPS_GPR_T8          24 /*  t8  */
#define MIPS_GPR_T9          25 /*  t9  */
#define MIPS_GPR_K0          26 /*  k0  */
#define MIPS_GPR_K1          27 /*  k1  */
#define MIPS_GPR_GP          28 /*  gp  */
#define MIPS_GPR_SP          29 /*  sp  */
#define MIPS_GPR_FP          30 /*  fp  */
#define MIPS_GPR_RA          31 /*  ra  */

/*
 * Coprocessor 0 (System Coprocessor) Register definitions
 */
#define MIPS_CP0_INDEX       0  /* TLB Index           */
#define MIPS_CP0_RANDOM      1  /* TLB Random          */
#define MIPS_CP0_TLB_LO_0    2  /* TLB Entry Lo0       */
#define MIPS_CP0_TLB_LO_1    3  /* TLB Entry Lo1       */
#define MIPS_CP0_CONTEXT     4  /* Kernel PTE pointer  */
#define MIPS_CP0_PAGEMASK    5  /* TLB Page Mask       */
#define MIPS_CP0_WIRED       6  /* TLB Wired           */
#define MIPS_CP0_INFO        7  /* Info (RM7000)       */
#define MIPS_CP0_BADVADDR    8  /* Bad Virtual Address */
#define MIPS_CP0_COUNT       9  /* Count               */
#define MIPS_CP0_TLB_HI      10 /* TLB Entry Hi        */
#define MIPS_CP0_COMPARE     11 /* Timer Compare       */
#define MIPS_CP0_STATUS      12 /* Status              */
#define MIPS_CP0_CAUSE       13 /* Cause               */
#define MIPS_CP0_EPC         14 /* Exception PC        */
#define MIPS_CP0_PRID        15 /* Proc Rev ID         */
#define MIPS_CP0_CONFIG      16 /* Configuration       */
#define MIPS_CP0_LLADDR      17 /* Load/Link address   */
#define MIPS_CP0_WATCHLO     18 /* Low Watch address   */
#define MIPS_CP0_WATCHHI     19 /* High Watch address  */
#define MIPS_CP0_XCONTEXT    20 /* Extended context    */
#define MIPS_CP0_ECC         26 /* ECC and parity      */
#define MIPS_CP0_CACHERR     27 /* Cache Err/Status    */
#define MIPS_CP0_TAGLO       28 /* Cache Tag Lo        */
#define MIPS_CP0_TAGHI       29 /* Cache Tag Hi        */
#define MIPS_CP0_ERR_EPC     30 /* Error exception PC  */

/*
 * CP0 Status Register
 */
#define MIPS_CP0_STATUS_CU0         0x10000000
#define MIPS_CP0_STATUS_CU1         0x20000000
#define MIPS_CP0_STATUS_BEV         0x00400000
#define MIPS_CP0_STATUS_TS          0x00200000
#define MIPS_CP0_STATUS_SR          0x00100000
#define MIPS_CP0_STATUS_CH          0x00040000
#define MIPS_CP0_STATUS_CE          0x00020000
#define MIPS_CP0_STATUS_DE          0x00010000
#define MIPS_CP0_STATUS_RP          0x08000000
#define MIPS_CP0_STATUS_FR          0x04000000
#define MIPS_CP0_STATUS_RE          0x02000000
#define MIPS_CP0_STATUS_KX          0x00000080
#define MIPS_CP0_STATUS_SX          0x00000040
#define MIPS_CP0_STATUS_UX          0x00000020
#define MIPS_CP0_STATUS_KSU         0x00000018
#define MIPS_CP0_STATUS_ERL         0x00000004
#define MIPS_CP0_STATUS_EXL         0x00000002
#define MIPS_CP0_STATUS_IE          0x00000001
#define MIPS_CP0_STATUS_IMASK7      0x00008000
#define MIPS_CP0_STATUS_IMASK6      0x00004000
#define MIPS_CP0_STATUS_IMASK5      0x00002000
#define MIPS_CP0_STATUS_IMASK4      0x00001000
#define MIPS_CP0_STATUS_IMASK3      0x00000800
#define MIPS_CP0_STATUS_IMASK2      0x00000400
#define MIPS_CP0_STATUS_IMASK1      0x00000200
#define MIPS_CP0_STATUS_IMASK0      0x00000100

#define MIPS_CP0_STATUS_DS_MASK     0x00770000
#define MIPS_CP0_STATUS_CU_MASK     0xF0000000
#define MIPS_CP0_STATUS_IMASK       0x0000FF00

/* Addressing mode: Kernel, Supervisor and User */
#define MIPS_CP0_STATUS_KSU_SHIFT   0x03
#define MIPS_CP0_STATUS_KSU_MASK    0x03

#define MIPS_CP0_STATUS_KM          0x00
#define MIPS_CP0_STATUS_SM          0x01
#define MIPS_CP0_STATUS_UM          0x10

/*
 * CP0 Cause register
 */

#define MIPS_CP0_CAUSE_BD_SLOT      0x80000000

#define MIPS_CP0_CAUSE_MASK         0x0000007C
#define MIPS_CP0_CAUSE_CEMASK       0x30000000
#ifdef SIM_PIC32
#define MIPS_CP0_CAUSE_IMASK        0x0000FC00  /* mips r2 */
#else
#define MIPS_CP0_CAUSE_IMASK        0x0000FF00  /* mips r1 */
#endif
#define MIPS_CP0_CAUSE_IV           0x00800000
#define MIPS_CP0_CAUSE_SHIFT        2
#define MIPS_CP0_CAUSE_CESHIFT      28
#define MIPS_CP0_CAUSE_ISHIFT       8
#define MIPS_CP0_CAUSE_EXC_MASK     0x0000007C

#define MIPS_CP0_CAUSE_INTERRUPT    0
#define MIPS_CP0_CAUSE_TLB_MOD      1
#define MIPS_CP0_CAUSE_TLB_LOAD     2
#define MIPS_CP0_CAUSE_TLB_SAVE     3
#define MIPS_CP0_CAUSE_ADDR_LOAD    4 /* ADEL */
#define MIPS_CP0_CAUSE_ADDR_SAVE    5 /* ADES */
#define MIPS_CP0_CAUSE_BUS_INSTR    6
#define MIPS_CP0_CAUSE_BUS_DATA     7
#define MIPS_CP0_CAUSE_SYSCALL      8
#define MIPS_CP0_CAUSE_BP           9
#define MIPS_CP0_CAUSE_ILLOP        10
#define MIPS_CP0_CAUSE_CP_UNUSABLE  11
#define MIPS_CP0_CAUSE_OVFLW        12
#define MIPS_CP0_CAUSE_TRAP         13
#define MIPS_CP0_CAUSE_VC_INSTR     14        /* Virtual Coherency */
#define MIPS_CP0_CAUSE_FPE          15
#define MIPS_CP0_CAUSE_WATCH        23
#define MIPS_CP0_CAUSE_VC_DATA      31        /* Virtual Coherency */

#define MIPS_CP0_CAUSE_IBIT7        0x00008000
#define MIPS_CP0_CAUSE_IBIT6        0x00004000
#define MIPS_CP0_CAUSE_IBIT5        0x00002000
#define MIPS_CP0_CAUSE_IBIT4        0x00001000
#define MIPS_CP0_CAUSE_IBIT3        0x00000800
#define MIPS_CP0_CAUSE_IBIT2        0x00000400
#define MIPS_CP0_CAUSE_IBIT1        0x00000200
#define MIPS_CP0_CAUSE_IBIT0        0x00000100

/* cp0 context */
#define MIPS_CP0_CONTEXT_PTEBASE_MASK  0xff800000
#define MIPS_CP0_CONTEXT_BADVPN2_MASK  0x0007ffff0

/* TLB masks and shifts */
#define MIPS_TLB_PAGE_MASK     0x01ffe000
#define MIPS_TLB_PAGE_SHIFT    13
//#define MIPS_TLB_VPN2_MASK        0xffffe000
#define MIPS_TLB_VPN2_MASK_32  0xffffe000
#define MIPS_TLB_VPN2_MASK_64  0xc00000ffffffe000ULL
#define MIPS_TLB_PFN_MASK      0x3fffffc0
#define MIPS_TLB_ASID_MASK     0x000000ff       /* "asid" in EntryHi */
#define MIPS_TLB_G_MASK        0x00001000       /* "Global" in EntryHi */
#define MIPS_TLB_V_MASK        0x2      /* "Valid" in EntryLo */
#define MIPS_TLB_D_MASK        0x4      /* "Dirty" in EntryLo */
#define MIPS_TLB_C_MASK        0x38     /* Page Coherency Attribute */
#define MIPS_TLB_C_SHIFT       3
#define MIPS_TLB_V_SHIT          1
#define MIPS_TLB_D_SHIT          2

#define MIPS_CP0_LO_G_MASK     0x00000001       /* "Global" in Lo0/1 reg */
#define MIPS_CP0_HI_SAFE_MASK  0xffffe0ff       /* Safety mask for Hi reg */
#define MIPS_CP0_LO_SAFE_MASK  0x7fffffff       /* Safety mask for Lo reg */

/* MIPS "jr ra" instruction */
#define MIPS_INSN_JR_RA        0x03e00008

#ifdef SIM_PIC32
#define MIPS_MIN_PAGE_SHIFT    8
#define MIPS_MIN_PAGE_SIZE     (1 << MIPS_MIN_PAGE_SHIFT)
#define MIPS_MIN_PAGE_IMASK    (MIPS_MIN_PAGE_SIZE - 1)
#define MIPS_MIN_PAGE_MASK     0xffffffffffffff00ULL
#else
/* Minimum page size: 4 Kb */
#define MIPS_MIN_PAGE_SHIFT    12
#define MIPS_MIN_PAGE_SIZE     (1 << MIPS_MIN_PAGE_SHIFT)
#define MIPS_MIN_PAGE_IMASK    (MIPS_MIN_PAGE_SIZE - 1)
#define MIPS_MIN_PAGE_MASK     0xfffffffffffff000ULL
#endif

/* Addressing mode: Kernel, Supervisor and User */
#define MIPS_MODE_KERNEL  00

/* Segments in 32-bit User mode */
#define MIPS_USEG_BASE    0x00000000
#define MIPS_USEG_SIZE    0x80000000

/* Segments in 32-bit Supervisor mode */
#define MIPS_SUSEG_BASE   0x00000000
#define MIPS_SUSEG_SIZE   0x80000000
#define MIPS_SSEG_BASE    0xc0000000
#define MIPS_SSEG_SIZE    0x20000000

/* Segments in 32-bit Kernel mode */
#define MIPS_KUSEG_BASE   0x00000000
#define MIPS_KUSEG_SIZE   0x80000000

#define MIPS_KSEG0_BASE   0x80000000
#define MIPS_KSEG0_SIZE   0x20000000

#define MIPS_KSEG1_BASE   0xa0000000
#define MIPS_KSEG1_SIZE   0x20000000

#define MIPS_KSSEG_BASE   0xc0000000
#define MIPS_KSSEG_SIZE   0x20000000

#define MIPS_KSEG3_BASE   0xe0000000
#define MIPS_KSEG3_SIZE   0x20000000

/* xkphys mask (36-bit physical address) */
#define MIPS64_XKPHYS_ZONE_MASK    0xF800000000000000ULL
#define MIPS64_XKPHYS_PHYS_SIZE    (1ULL << 36)
#define MIPS64_XKPHYS_PHYS_MASK    (MIPS64_XKPHYS_PHYS_SIZE - 1)
#define MIPS64_XKPHYS_CCA_SHIFT    59

/* Number of GPR (general purpose registers) */
#define MIPS64_GPR_NR  32

/* Number of registers in CP0 */
#define MIPS64_CP0_REG_NR   32

/*8 configure register in cp0. sel:0-7*/
#define MIPS64_CP0_CONFIG_REG_NR  8

/* Number of registers in CP1 */
#define MIPS64_CP1_REG_NR   32

/* Number of TLB entries */
#define MIPS64_TLB_STD_ENTRIES  48
#define MIPS64_TLB_MAX_ENTRIES  64
#define MIPS64_TLB_IDX_MASK     0x3f    /* 6 bits */

/* Enable the 64 TLB entries for R7000 CPU */
#define MIPS64_R7000_TLB64_ENABLE   0x20000000

/* Number of instructions per page */
#define MIPS_INSN_PER_PAGE (MIPS_MIN_PAGE_SIZE/sizeof(mips_insn_t))

/* MIPS CPU Identifiers */
#define MIPS_PRID_R4600    0x00002012
#define MIPS_PRID_R4700    0x00002112
#define MIPS_PRID_R5000    0x00002312
#define MIPS_PRID_R7000    0x00002721
#define MIPS_PRID_R527x    0x00002812
#define MIPS_PRID_BCM1250  0x00040102

enum {
    MIPS_KUSEG = 0,
    MIPS_KSEG0,
    MIPS_KSEG1,
    MIPS_KSEG2,
};

/* Memory operations */
enum {
    MIPS_MEMOP_LOOKUP = 0,

    MIPS_MEMOP_LB,
    MIPS_MEMOP_LBU,
    MIPS_MEMOP_LH,
    MIPS_MEMOP_LHU,
    MIPS_MEMOP_LW,
    MIPS_MEMOP_LWU,
    MIPS_MEMOP_LD,
    MIPS_MEMOP_SB,
    MIPS_MEMOP_SH,
    MIPS_MEMOP_SW,
    MIPS_MEMOP_SD,

    MIPS_MEMOP_LWL,
    MIPS_MEMOP_LWR,
    MIPS_MEMOP_LDL,
    MIPS_MEMOP_LDR,
    MIPS_MEMOP_SWL,
    MIPS_MEMOP_SWR,
    MIPS_MEMOP_SDL,
    MIPS_MEMOP_SDR,

    MIPS_MEMOP_LL,
    MIPS_MEMOP_SC,

    MIPS_MEMOP_LDC1,
    MIPS_MEMOP_SDC1,

    MIPS_MEMOP_CACHE,

    MIPS_MEMOP_MAX,
};

/* Maximum number of breakpoints */
#define MIPS64_MAX_BREAKPOINTS  8

#define CPU_INTERRUPT_EXIT   0x01       /* wants exit from main loop */
#define CPU_INTERRUPT_HARD   0x02       /* hardware interrupt pending */
#define CPU_INTERRUPT_EXITTB 0x04       /* exit the current TB (use for x86 a20 case) */
#define CPU_INTERRUPT_TIMER  0x08       /* internal timer exception pending */
#define CPU_INTERRUPT_FIQ    0x10       /* Fast interrupt pending.  */
#define CPU_INTERRUPT_HALT   0x20       /* CPU halt wanted */
#define CPU_INTERRUPT_SMI    0x40       /* (x86 only) SMI interrupt pending */

/* MIPS CPU type */
//typedef struct cpu_mips cpu_mips_t;

/* Memory operation function prototype */
typedef u_int fastcall (*mips_memop_fn) (cpu_mips_t * cpu, m_va_t vaddr,
    u_int reg);

/* TLB entry definition */
typedef struct {
    m_va_t mask;
    m_va_t hi;
    m_va_t lo0;
    m_va_t lo1;
} tlb_entry_t;

/* System Coprocessor (CP0) definition */
typedef struct {
    m_cp0_reg_t reg[MIPS64_CP0_REG_NR];
    /*because configure has sel 0-7, seperate it to reg */
    m_cp0_reg_t config_reg[MIPS64_CP0_CONFIG_REG_NR];
    m_cp0_reg_t intctl_reg;
    m_cp0_reg_t ebase_reg;
    m_uint8_t config_usable;    /*if configure register sel N is useable, set the bit in config_usable to 1 */

    tlb_entry_t tlb[MIPS64_TLB_MAX_ENTRIES];

    /* Number of TLB entries */
    u_int tlb_entries;

} mips_cp0_t;

/* mips CPU definition */
struct cpu_mips {
    /* CPU identifier for MP systems */
    u_int id;
    u_int type;

    m_va_t pc, ret_pc;
    m_va_t jit_pc;
    m_reg_t gpr[MIPS64_GPR_NR];
    m_reg_t lo, hi;
    /* VM instance */
    vm_instance_t *vm;
    /* Next CPU in group */
    cpu_mips_t *next;
    /* System coprocessor (CP0) */
    mips_cp0_t cp0;

    /* CPU states */
    volatile m_uint32_t state, prev_state;

    /* Thread running this CPU */
    pthread_t cpu_thread;
    int cpu_thread_running;

    /*pause request. INTERRUPT will pause cpu */
    m_uint32_t pause_request;

    /* Methods */
    int (*reg_get) (cpu_mips_t * cpu, u_int reg, m_reg_t * val);
    void (*reg_set) (cpu_mips_t * cpu, u_int reg_index, m_reg_t val);
    void (*mts_rebuild) (cpu_mips_t * cpu);
         u_int (*mips_mts_gdb_lb) (cpu_mips_t * cpu, m_va_t vaddr, void *cur);

    /* MTS32/MTS64 caches */
    union {
        mts32_entry_t *mts32_cache;
        mts64_entry_t *mts64_cache;
    } mts_u;

    /* General Purpose Registers, Pointer Counter, LO/HI, IRQ */
    m_uint32_t irq_pending, irq_cause, ll_bit;

    /* Virtual address to physical page translation */
    int (*translate) (cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t * phys_page);
    /* Memory access functions */
    mips_memop_fn mem_op_fn[MIPS_MEMOP_MAX];
    /* Memory lookup function (to load ELF image,...) */
    void *fastcall (*mem_op_lookup) (cpu_mips_t * cpu, m_va_t vaddr);

    /* Address bus mask for physical addresses */
    m_va_t addr_bus_mask;

    /* MTS map/unmap/rebuild operations */
    void (*mts_map) (cpu_mips_t * cpu, m_va_t vaddr, m_pa_t paddr,
        m_uint32_t len, int cache_access, int tlb_index);
    void (*mts_unmap) (cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t len,
        m_uint32_t val, int tlb_index);
    void (*mts_shutdown) (cpu_mips_t * cpu);
    /* MTS cache statistics */
    m_uint64_t mts_misses, mts_lookups;

    /* Address mode (32 or 64 bits) */
    u_int addr_mode;

    int is_in_bdslot;
    int trace_syscall;

    /* Current exec page (non-JIT) info */
    m_va_t njm_exec_page;
    mips_insn_t *njm_exec_ptr;

#ifdef _USE_JIT_
    /* JIT flush method */
    u_int jit_flush_method;

    /* Number of compiled pages */
    u_int compiled_pages;

    /* Code page translation cache */
    mips_jit_tcb_t **exec_blk_map;
    void *exec_page_area;
    size_t exec_page_area_size; /*M bytes */
    size_t exec_page_count, exec_page_alloc;
    insn_exec_page_t *exec_page_free_list;
    insn_exec_page_t *exec_page_array;
    /* Current and free lists of translated code blocks */
    mips_jit_tcb_t *tcb_list, *tcb_last, *tcb_free_list;
    /* Direct block jump.Optimization */
    u_int exec_blk_direct_jump;

#endif

};

/* Register names */
extern char *mips_gpr_reg_names[];

#define MAJOR_OP(_inst) (((uint)_inst >> 26) & 0x3f )

int mips_load_elf_image (cpu_mips_t * cpu, char *filename,
    m_va_t * entry_point);

int mips_get_reg_index (char *name);
int mips_cca_cached (m_uint8_t val);
void mips_dump_regs (cpu_mips_t * cpu);
void mips_delete (cpu_mips_t * cpu);
int mips_reset (cpu_mips_t * cpu);
int mips_init (cpu_mips_t * cpu);
int mips_load_elf_image (cpu_mips_t * cpu, char *filename,
    m_va_t * entry_point);
void mips_delete (cpu_mips_t * cpu);
int fastcall mips_update_irq_flag (cpu_mips_t * cpu);
void mips_trigger_exception (cpu_mips_t * cpu, u_int exc_code, int bd_slot);
void fastcall mips_exec_soft_fpu (cpu_mips_t * cpu);
void fastcall mips_exec_eret (cpu_mips_t * cpu);
void fastcall mips_exec_break (cpu_mips_t * cpu, u_int code);
void fastcall mips_trigger_trap_exception (cpu_mips_t * cpu);
void fastcall mips_exec_syscall (cpu_mips_t * cpu);
void fastcall mips_trigger_irq (cpu_mips_t * cpu);
void mips_set_irq (cpu_mips_t * cpu, m_uint8_t irq);
void mips_clear_irq (cpu_mips_t * cpu, m_uint8_t irq);

/* Control timer interrupt */
void set_timer_irq (cpu_mips_t *cpu);
void clear_timer_irq (cpu_mips_t *cpu);

/* Print the mips instruction at address MEMADDR in debugged memory. */
int print_insn_mips (unsigned memaddr, unsigned long int word, FILE *stream);

const char *cp0reg_name (unsigned cp0reg, unsigned sel);

int mips_fetch_instruction (cpu_mips_t * cpu,
    m_va_t pc, mips_insn_t * insn);
void *mips_mts32_access (cpu_mips_t * cpu, m_va_t vaddr,
    u_int op_code, u_int op_size,
    u_int op_type, m_reg_t * data, u_int * exc, m_uint8_t * has_set_value,
    u_int is_fromgdb);

#endif
