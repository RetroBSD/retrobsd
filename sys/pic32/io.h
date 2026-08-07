/*
 * Hardware register defines for MIPS32 architecture.
 *
 * Copyright (C) 2008-2010 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#ifdef PIC32MX4
#   include "machine/pic32mx.h"
#endif
#ifdef PIC32MX7
#   include "machine/pic32mx.h"
#endif
#ifdef __riscv
    #include "machine/riscv.h"
#endif


/*
 * Offsets of register values in saved context.
 */
#define FRAME_R1        0
#define FRAME_R2        1
#define FRAME_R3        2
#define FRAME_R4        3
#define FRAME_R5        4
#define FRAME_R6        5
#define FRAME_R7        6
#define FRAME_R8        7
#define FRAME_R9        8
#define FRAME_R10       9
#define FRAME_R11       10
#define FRAME_R12       11
#define FRAME_R13       12
#define FRAME_R14       13
#define FRAME_R15       14
#define FRAME_R16       15
#define FRAME_R17       16
#define FRAME_R18       17
#define FRAME_R19       18
#define FRAME_R20       19
#define FRAME_R21       20
#define FRAME_R22       21
#define FRAME_R23       22
#define FRAME_R24       23
#define FRAME_R25       24
#define FRAME_GP        25
#define FRAME_SP        26
#define FRAME_FP        27
#define FRAME_RA        28
#define FRAME_LO        29
#define FRAME_HI        30
#define FRAME_STATUS    31
#define FRAME_PC        32

#define FRAME_WORDS     33

#ifndef __ASSEMBLER__

#ifndef KERNEL
/*
 * 2BSD system call extensions: use with care.
 */

/*
 * Read peripheral register.
 */
unsigned ufetch (unsigned addr);

/*
 * Write peripheral register.
 */
unsigned ustore (unsigned addr, unsigned value);

/*
 * Call a kernel function.
 */
unsigned ucall (int priority, void *address, int arg1, int arg2);

#endif /* KERNEL */

/*
 * Set value of stack pointer register.
 */
static void inline __attribute__ ((always_inline))
mips_set_stack_pointer (void *x)
{
    #ifdef __riscv
    asm volatile (
    "mv   sp,a0"
    : : "r" (x) : "sp");
    #else
    asm volatile (
    "move   $sp, %0"
    : : "r" (x) : "sp");
    #endif
}

/*
 * Get value of stack pointer register.
 */
static inline __attribute__ ((always_inline))
void *mips_get_stack_pointer ()
{
    void *x;
    #ifdef __riscv
    asm volatile (
    "mv   a0,sp"
    : "=r" (x));
    #else
    asm volatile (
    "move   %0, $sp"
    : "=r" (x));
    #endif
    return x;
}

/*
 * Read C0 coprocessor register.
 */
#define mips_read_c0_register(reg,sel) \
    ({  int __value; \
        asm volatile ( \
        "mfc0   %0, $%1, %2" \
        : "=r" (__value) : "K" (reg), "K" (sel)); \
        __value; \
    })

/*
 * Write C0 coprocessor register.
 */
#define mips_write_c0_register(reg, sel, value) \
    do { \
        asm volatile ( \
        "mtc0   %z0, $%1, %2 \n ehb" \
        : : "r" ((unsigned int) (value)), "K" (reg), "K" (sel)); \
    } while (0)

/*
 * Disable the hardware interrupts,
 * saving the interrupt state into the supplied variable.
 */
static int inline __attribute__ ((always_inline))
mips_intr_disable ()
{
    int status;
    #ifdef __riscv
    asm volatile (
        "csrr   a0,mstatus\n" 
        "sfence.vma"
        : "=r" (status));
    #else
    asm volatile ("di   %0" : "=r" (status));
    #endif
    return status;
}

/*
 * Restore the hardware interrupt mode using the saved interrupt state.
 */
static void inline __attribute__ ((always_inline))
mips_intr_restore (int x)
{
    /* C0_STATUS */
    #ifdef __riscv
    asm volatile ("csrw mstatus,a0");
    #else
    mips_write_c0_register (12, 0, x);
    #endif
}

/*
 * Explicit hazard barrier.
 */
static void inline __attribute__ ((always_inline))
mips_ehb()
{
    #ifdef __riscv
    asm volatile ("fence");
    #else
    asm volatile ("ehb");
    #endif
}

/*
 * Enable hardware interrupts.
 */
static int inline __attribute__ ((always_inline))
mips_intr_enable ()
{
    int status;
    #ifdef __riscv
    asm volatile (
        "csrr a0, mstatus\n"
        "sfence.vma\n"
        "li a1, 0x0001\n"  // Enable interrupts (SIE bit)
        "or a0, a0,a1\n"
        "csrw mstatus, a0"
        : "=r" (status));
    #else
    asm volatile ("ei   %0" : "=r" (status));
    #endif
    return status;
}

/*
 * Count a number of leading (most significant) zero bits in a word.
 */
static int inline __attribute__ ((always_inline))
mips_clz (unsigned x)
{
    int n;
    #ifdef __riscv
    if (x != 0) {
        while ((x & (1 << n)) == 0) {
          n--;
        }
    }
    #else
    asm volatile ("clz  %0, %1"
        : "=r" (n) : "r" (x));
    #endif
    return n;
}

/*
 * Swap bytes in a word: ABCD to DCBA.
 */
static unsigned inline __attribute__ ((always_inline))
mips_bswap (unsigned x)
{
    int n;
    #ifdef __riscv
    asm volatile (
        "li t0, 0xff\n"     // Load immediate with all ones
        "sll t1, t0, 24\n"  // Move all ones to the most significant byte
        "srl t2, a0, 24\n"  // Extract the most significant byte
        "and t3, t1, t2\n"  // Combine all ones with the most significant byte
        "sra a0, a0, 24\n"  // Shift the remaining bytes right by 24 bits
        "ins a0, t3, 0, 8\n"// Insert the combined byte into the most significant position
        "sll t1, t0, 8\n"   // Move all ones to the second byte
        "srl t2, a0, 16\n"  // Extract the second byte
        "and t3, t1, t2\n"  // Combine all ones with the second byte
        "sra a0, a0, 16\n"  // Shift the remaining bytes right by 16 bits
        "ins a0, t3, 8, 8\n"  // Insert the combined byte into the second position
        "sll t1, t0, 0\n"     // Move all ones to the least significant byte
        "srl t2, a0, 8\n"     // Extract the least significant byte
        "and t3, t1, t2\n"    // Combine all ones with the least significant byte
        "sra a0, a0, 8\n"     // Shift the remaining byte right by 8 bits
        "ins a0, t3, 16, 8" // Insert the combined byte into the least significant position
        : "=r" (n) : "r" (x));

    #else
    asm volatile (
    "wsbh   %0, %1 \n"
    "rotr   %0, 16"
        : "=r" (n) : "r" (x));
    #endif
    return n;
}
#endif /* __ASSEMBLER__ */
