/*
 * Instruction printing
 */
#include "defs.h"

/*
 * Flags of instruction formats.
 */
#define FRD1    (1 << 0)        /* rd, ... */
#define FRD2    (1 << 1)        /* .., rd, ... */
#define FRT1    (1 << 2)        /* rt, ... */
#define FRT2    (1 << 3)        /* .., rt, ... */
#define FRT3    (1 << 4)        /* .., .., rt */
#define FRS1    (1 << 5)        /* rs, ... */
#define FRS2    (1 << 6)        /* .., rs, ... */
#define FRS3    (1 << 7)        /* .., .., rs */
#define FRSB    (1 << 8)        /* ... (rs) */
#define FCODE   (1 << 9)        /* immediate shifted <<6 */
#define FOFF16  (1 << 11)       /* 16-bit relocatable offset */
#define FHIGH16 (1 << 12)       /* high 16-bit relocatable offset */
#define FOFF18  (1 << 13)       /* 18-bit PC-relative relocatable offset shifted >>2 */
#define FAOFF18 (1 << 14)       /* 18-bit PC-relative relocatable offset shifted >>2 */
#define FAOFF28 (1 << 15)       /* 28-bit absolute relocatable offset shifted >>2 */
#define FSA     (1 << 16)       /* 5-bit shift amount */
#define FSEL    (1 << 17)       /* optional 3-bit COP0 register select */
#define FSIZE   (1 << 18)       /* bit field size */
#define FMSB    (1 << 19)       /* bit field msb */
#define FRTD    (1 << 20)       /* set rt to rd number */
#define FCODE16 (1 << 21)       /* immediate shifted <<16 */
#define FSYS    (1 << 22)       /* syscall */

struct optable {
    unsigned opcode;
    unsigned mask;
    const char *name;
    unsigned type;
};

static const struct optable optable [] = {
    { 0x34000000, 0xffe00000,   "li",       FRT1 | FOFF16 },        // ori
    { 0x24000000, 0xffe00000,   "li",       FRT1 | FOFF16 },        // addiu
    { 0x00000021, 0xfc1f07ff,   "move",     FRD1 | FRS2 },          // addu
    { 0x00000040, 0xffffffff,   "ssnop",    0 },                    // nop
    { 0x00000020, 0xfc0007ff,   "add",      FRD1 | FRS2 | FRT3 },
    { 0x20000000, 0xfc000000,   "addi",     FRT1 | FRS2 | FOFF16 },
    { 0x24000000, 0xfc000000,   "addiu",    FRT1 | FRS2 | FOFF16 },
    { 0x00000021, 0xfc0007ff,   "addu",     FRD1 | FRS2 | FRT3 },
    { 0x00000024, 0xfc0007ff,   "and",      FRD1 | FRS2 | FRT3 },
    { 0x30000000, 0xfc000000,   "andi",     FRT1 | FRS2 | FOFF16 },
    { 0x10000000, 0xffff0000,   "b",        FAOFF18 },
    { 0x04110000, 0xffff0000,   "bal",      FAOFF18 },
    { 0x10000000, 0xfc000000,   "beq",      FRS1 | FRT2 | FOFF18 },
    { 0x50000000, 0xfc000000,   "beql",     FRS1 | FRT2 | FOFF18 },
    { 0x04010000, 0xfc1f0000,   "bgez",     FRS1 | FOFF18 },
    { 0x04110000, 0xfc1f0000,   "bgezal",   FRS1 | FOFF18 },
    { 0x04130000, 0xfc1f0000,   "bgezall",  FRS1 | FOFF18 },
    { 0x04030000, 0xfc1f0000,   "bgezl",    FRS1 | FOFF18 },
    { 0x1c000000, 0xfc1f0000,   "bgtz",     FRS1 | FOFF18 },
    { 0x5c000000, 0xfc1f0000,   "bgtzl",    FRS1 | FOFF18 },
    { 0x18000000, 0xfc1f0000,   "blez",     FRS1 | FOFF18 },
    { 0x58000000, 0xfc1f0000,   "blezl",    FRS1 | FOFF18 },
    { 0x04000000, 0xfc1f0000,   "bltz",     FRS1 | FOFF18 },
    { 0x04100000, 0xfc1f0000,   "bltzal",   FRS1 | FOFF18 },
    { 0x04120000, 0xfc1f0000,   "bltzall",  FRS1 | FOFF18 },
    { 0x04020000, 0xfc1f0000,   "bltzl",    FRS1 | FOFF18 },
    { 0x14000000, 0xfc000000,   "bne",      FRS1 | FRT2 | FOFF18 },
    { 0x54000000, 0xfc000000,   "bnel",     FRS1 | FRT2 | FOFF18 },
    { 0x0000000d, 0xfc00ffff,   "break",    FCODE16 },
    { 0x70000021, 0xfc0007ff,   "clo",      FRD1 | FRS2 | FRTD },
    { 0x70000020, 0xfc0007ff,   "clz",      FRD1 | FRS2 | FRTD },
    { 0x4200001f, 0xffffffff,   "deret",    0 },
    { 0x41606000, 0xffe0ffff,   "di",       FRT1 },
    { 0x0000001a, 0xfc00ffff,   "div",      FRS1 | FRT2 },
    { 0x0000001b, 0xfc00ffff,   "divu",     FRS1 | FRT2 },
    { 0x000000c0, 0xffffffff,   "ehb",      0 },
    { 0x41606020, 0xffe0ffff,   "ei",       FRT1 },
    { 0x42000018, 0xffffffff,   "eret",     0 },
    { 0x7c000000, 0xfc00003f,   "ext",      FRT1 | FRS2 | FSA | FSIZE },
    { 0x7c000004, 0xfc00003f,   "ins",      FRT1 | FRS2 | FSA | FMSB },
    { 0x08000000, 0xfc000000,   "j",        FAOFF28 },
    { 0x0c000000, 0xfc000000,   "jal",      FAOFF28 },
    { 0x00000009, 0xfc1f07ff,   "jalr",     FRD1 | FRS2 },
    { 0x00000409, 0xfc1f07ff,   "jalr.hb",  FRD1 | FRS2 },
    { 0x00000008, 0xfc1fffff,   "jr",       FRS1 },
    { 0x00000408, 0xfc1fffff,   "jr.hb",    FRS1 },
    { 0x80000000, 0xfc000000,   "lb",       FRT1 | FOFF16 | FRSB },
    { 0x90000000, 0xfc000000,   "lbu",      FRT1 | FOFF16 | FRSB },
    { 0x84000000, 0xfc000000,   "lh",       FRT1 | FOFF16 | FRSB },
    { 0x94000000, 0xfc000000,   "lhu",      FRT1 | FOFF16 | FRSB },
    { 0xc0000000, 0xfc000000,   "ll",       FRT1 | FOFF16 | FRSB },
    { 0x3c000000, 0xffe00000,   "lui",      FRT1 | FHIGH16 },
    { 0x8c000000, 0xfc000000,   "lw",       FRT1 | FOFF16 | FRSB },
    { 0x88000000, 0xfc000000,   "lwl",      FRT1 | FOFF16 | FRSB },
    { 0x98000000, 0xfc000000,   "lwr",      FRT1 | FOFF16 | FRSB },
    { 0x70000000, 0xfc00ffff,   "madd",     FRS1 | FRT2 },
    { 0x70000001, 0xfc00ffff,   "maddu",    FRS1 | FRT2 },
    { 0x40000000, 0xffe007f8,   "mfc0",     FRT1 | FRD2 | FSEL },
    { 0x00000010, 0xffff07ff,   "mfhi",     FRD1 },
    { 0x00000012, 0xffff07ff,   "mflo",     FRD1 },
    { 0x0000000b, 0xfc0007ff,   "movn",     FRD1 | FRS2 | FRT3 },
    { 0x0000000a, 0xfc0007ff,   "movz",     FRD1 | FRS2 | FRT3 },
    { 0x70000004, 0xfc00ffff,   "msub",     FRS1 | FRT2 },
    { 0x70000005, 0xfc00ffff,   "msubu",    FRS1 | FRT2 },
    { 0x40800000, 0xffe007f8,   "mtc0",     FRT1 | FRD2 | FSEL },
    { 0x00000011, 0xfc1fffff,   "mthi",     FRS1 },
    { 0x00000013, 0xfc1fffff,   "mtlo",     FRS1 },
    { 0x70000002, 0xfc0007ff,   "mul",      FRD1 | FRS2 | FRT3 },
    { 0x00000018, 0xfc00ffff,   "mult",     FRS1 | FRT2 },
    { 0x00000019, 0xfc00ffff,   "multu",    FRS1 | FRT2 },
    { 0x00000000, 0xffffffff,   "nop",      0 },
    { 0x00000027, 0xfc0007ff,   "nor",      FRD1 | FRS2 | FRT3 },
    { 0x00000025, 0xfc0007ff,   "or",       FRD1 | FRS2 | FRT3 },
    { 0x34000000, 0xfc000000,   "ori",      FRT1 | FRS2 | FOFF16 },
    { 0x7c00003b, 0xffe007ff,   "rdhwr",    FRT1 | FRD2 },
    { 0x41400000, 0xffe007ff,   "rdpgpr",   FRD1 | FRT2 },
    { 0x00200002, 0xffe0003f,   "ror",      FRD1 | FRT2 | FSA },
    { 0x00000046, 0xfc0007ff,   "rorv",     FRD1 | FRT2 | FRS3 },
    { 0xa0000000, 0xfc000000,   "sb",       FRT1 | FOFF16 | FRSB },
    { 0xe0000000, 0xfc000000,   "sc",       FRT1 | FOFF16 | FRSB },
    { 0x7000003f, 0xfc00003f,   "sdbbp",    FCODE },
    { 0x7c000420, 0xffe007ff,   "seb",      FRD1 | FRT2 },
    { 0x7c000620, 0xffe007ff,   "seh",      FRD1 | FRT2 },
    { 0xa4000000, 0xfc000000,   "sh",       FRT1 | FOFF16 | FRSB },
    { 0x00000000, 0xffe0003f,   "sll",      FRD1 | FRT2 | FSA },
    { 0x00000004, 0xfc0007ff,   "sllv",     FRD1 | FRT2 | FRS3 },
    { 0x0000002a, 0xfc0007ff,   "slt",      FRD1 | FRS2 | FRT3 },
    { 0x28000000, 0xfc000000,   "slti",     FRT1 | FRS2 | FOFF16 },
    { 0x2c000000, 0xfc000000,   "sltiu",    FRT1 | FRS2 | FOFF16 },
    { 0x0000002b, 0xfc0007ff,   "sltu",     FRD1 | FRS2 | FRT3 },
    { 0x00000003, 0xffe0003f,   "sra",      FRD1 | FRT2 | FSA },
    { 0x00000007, 0xfc0007ff,   "srav",     FRD1 | FRT2 | FRS3 },
    { 0x00000002, 0xffe0003f,   "srl",      FRD1 | FRT2 | FSA },
    { 0x00000006, 0xfc0007ff,   "srlv",     FRD1 | FRT2 | FRS3 },
    { 0x00000022, 0xfc0007ff,   "sub",      FRD1 | FRS2 | FRT3 },
    { 0x00000023, 0xfc0007ff,   "subu",     FRD1 | FRS2 | FRT3 },
    { 0xac000000, 0xfc000000,   "sw",       FRT1 | FOFF16 | FRSB },
    { 0xa8000000, 0xfc000000,   "swl",      FRT1 | FOFF16 | FRSB },
    { 0xb8000000, 0xfc000000,   "swr",      FRT1 | FOFF16 | FRSB },
    { 0x0000000f, 0xfffff83f,   "sync",     FCODE },
    { 0x0000000c, 0xfc00003f,   "syscall",  FSYS },
    { 0x00000034, 0xfc00ffff,   "teq",      FRS1 | FRT2 },
    { 0x040c0000, 0xfc1f0000,   "teqi",     FRS1 | FOFF16 },
    { 0x00000030, 0xfc00ffff,   "tge",      FRS1 | FRT2 },
    { 0x04080000, 0xfc1f0000,   "tgei",     FRS1 | FOFF16 },
    { 0x04090000, 0xfc1f0000,   "tgeiu",    FRS1 | FOFF16 },
    { 0x00000031, 0xfc00ffff,   "tgeu",     FRS1 | FRT2 },
    { 0x00000032, 0xfc00ffff,   "tlt",      FRS1 | FRT2 },
    { 0x040a0000, 0xfc1f0000,   "tlti",     FRS1 | FOFF16 },
    { 0x040b0000, 0xfc1f0000,   "tltiu",    FRS1 | FOFF16 },
    { 0x00000033, 0xfc00ffff,   "tltu",     FRS1 | FRT2 },
    { 0x00000036, 0xfc00ffff,   "tne",      FRS1 | FRT2 },
    { 0x040e0000, 0xfc1f0000,   "tnei",     FRS1 | FOFF16 },
    { 0x42000020, 0xfe00003f,   "wait",     FCODE },
    { 0x41c00000, 0xffe007ff,   "wrpgpr",   FRD1 | FRT2 },
    { 0x7c0000a0, 0xffe007ff,   "wsbh",     FRD1 | FRT2 },
    { 0x00000026, 0xfc0007ff,   "xor",      FRD1 | FRS2 | FRT3 },
    { 0x38000000, 0xfc000000,   "xori",     FRT1 | FRS2 | FOFF16 },
    { 0,          0,            0,          0 },
};

static const char *systab[] = {
    "indir",                /* 0 - indir*/
    "exit",
    "fork",
    "read",
    "write",
    "open",
    "close",
    "wait4",
    NULL,                   /* 8 - unused */
    "link",
    "unlink",               /* 10 */
    "execv",
    "chdir",
    "fchdir",
    "mknod",
    "chmod",
    "chown",
    "chflags",
    "fchflags",
    "lseek",
    "getpid",               /* 20 */
    "mount",
    "umount",
    "__sysctl",
    "getuid",
    "geteuid",              /* 25 */
    "ptrace",
    "getppid",
    NULL,                   /* 28 - unused */
    NULL,                   /* 29 - unused */
    NULL,                   /* 30 - unused */
    "sigaction",            /* 31 - sigaction */
    "sigprocmask",          /* 32 - sigprocmask */
    "access",
    "sigpending",           /* 34 - sigpending */
    "sigaltstack",          /* 35 - sigaltstack */
    "sync",
    "kill",
    "stat",
    "_getlogin",            /* 39 - _getlogin */
    "lstat",
    "dup",
    "pipe",
    "setlogin",             /* 43 - unused */
    "profil",
    "setuid",               /* 45 - setuid */
    "seteuid",              /* 46 - seteuid */
    "getgid",
    "getegid",
    "setgid",               /* 49 - setgid */
    "setegid",              /* 50 - setegid */
    "acct",
    "phys",
    "lock",
    "ioctl",
    "reboot",
    NULL,                   /* 56 - unused */
    "symlink",
    "readlink",
    "execve",
    "umask",
    "chroot",
    "fstat",
    NULL,                   /* 63 - unused */
    NULL,                   /* 64 - unused */
    "pselect",              /* 65 - pselect */
    "vfork",
    NULL,                   /* 67 - unused */
    NULL,                   /* 68 - unused */
    "sbrk",
    NULL,                   /* 70 - unused */
    NULL,                   /* 71 - unused */
    NULL,                   /* 72 - unused */
    NULL,                   /* 73 - unused */
    NULL,                   /* 74 - unused */
    NULL,                   /* 75 - unused */
    "vhangup",
    NULL,                   /* 77 - unused */
    NULL,                   /* 78 - unused */
    "getgroups",
    "setgroups",
    "getpgrp",
    "setpgrp",
    "setitimer",
    NULL,                   /* 84 - unused */
    NULL,                   /* 85 - unused */
    "getitimer",
    NULL,                   /* 87 - unused */
    NULL,                   /* 88 - unused */
    "getdtablesize",
    "dup2",
    NULL,                   /* 91 - unused */
    "fcntl",
    "select",
    NULL,                   /* 94 - unused */
    "fsync",
    "setpriority",
    "socket",
    "connect",
    "accept",
    "getpriority",
    "send",
    "recv",                 /* 102 - recv */
    "sigreturn",
    "bind",
    "setsockopt",
    "listen",
    "sigsuspend",           /* 107 - sigsuspend */
    NULL,                   /* 108 - unused */
    NULL,                   /* 109 - unused */
    NULL,                   /* 110 - unused */
    NULL,                   /* 111 - unused */
    "old sigstack",         /* 112 - sigstack COMPAT-43 for zork */
    "recvmsg",
    "sendmsg",
    NULL,                   /* 115 - unused */
    "gettimeofday",
    "getrusage",
    "getsockopt",
    NULL,                   /* 119 - unused */
    "readv",
    "writev",
    "settimeofday",
    "fchown",
    "fchmod",
    "recvfrom",
    NULL,                   /* 126 - unused */
    NULL,                   /* 127 - unused */
    "rename",
    "truncate",
    "ftruncate",
    "flock",
    NULL,                   /* 132 - unused */
    "sendto",
    "shutdown",
    "socketpair",
    "mkdir",
    "rmdir",
    "utimes",
    NULL,                   /* 139 - unused */
    "adjtime",
    "getpeername",
    NULL,                   /* 142 - unused */
    NULL,                   /* 143 - unused */
    "getrlimit",
    "setrlimit",
    "killpg",
    NULL,                   /* 147 - unused */
    "setquota",
    "quota",
    "getsockname",
    /*
     * 2.11BSD special calls
     */
    NULL,                   /* 151 - unused */
    "nostk",
    "fetchi",
    "ucall",
    "fperr",
};

#define NUMSYSCALLS     (sizeof (systab) / sizeof (char *))

static const char *const regname[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};

/*
 * Print the mips instruction at address MEMADDR in debugged memory.
 * Returns length of the instruction, in bytes, which is
 * always INSNLEN.
 */
void
printins (space, memaddr, word)
    unsigned memaddr, word;
{
    const struct optable *op;

    for (op=optable; op->mask; op++) {
        if ((word & op->mask) == op->opcode)
            break;
    }
    if (! op->mask) {
        print (".word%8t%x", word);
        return;
    }
    print ("%s%8t", op->name);

    /*
     * First register.
     */
    if (op->type & FRD1)                /* rd, ... */
        print ("%s", regname[word >> 11 & 31]);
    if (op->type & FRT1)                /* rt, ... */
        print ("%s", regname[word >> 16 & 31]);
    if (op->type & FRS1)                /* rs, ... */
        print ("%s", regname[word >> 21 & 31]);

    /*
     * Second register.
     */
    if (op->type & FRD2)                /* .., rd, ... */
        print (", %s", regname[word >> 11 & 31]);
    if (op->type & FRT2)                /* .., rt, ... */
        print (", %s", regname[word >> 16 & 31]);
    if (op->type & FRS2)                /* .., rs, ... */
        print (", %s", regname[word >> 21 & 31]);

    /*
     * Third register.
     */
    if (op->type & FRT3)                /* .., .., rt */
        print (", %s", regname[word >> 16 & 31]);
    if (op->type & FRS3)                /* .., .., rs */
        print (", %s", regname[word >> 21 & 31]);

    /*
     * Immediate argument.
     */
    if (op->type & FSEL) {
        /* optional COP0 register select */
        if (word & 7)
            print (", %x", word & 7);

    } else if (op->type & FSYS) {
        /* Syscall */
        unsigned code = (word & ~op->mask) >> 6;

        if (code < NUMSYSCALLS && systab[code])
            print("%s", systab[code]);
        else
            print("%d", code);

    } else if (op->type & FCODE) {
        /* Non-relocatable offset */
        print ("%x", (word & ~op->mask) >> 6);

    } else if (op->type & FCODE16) {
        print ("%x", (word & ~op->mask) >> 16);

    } else if (op->type & FSA) {
        print (", %x", word >> 6 & 0x1f);

    } else if (op->type & FOFF16) {
        /* Relocatable offset */
        print (", %x", word & 0xffff);

    } else if (op->type & FOFF18) {
        unsigned addr = ((signed short) (word & 0xffff) << 2) + memaddr + 4;
        print (", ");
        psymoff(addr, ISYM, "");

    } else if (op->type & FHIGH16) {
        print (", %x", word & 0xffff);

    } else if (op->type & FAOFF18) {
        unsigned addr = ((signed short) (word & 0xffff) << 2) + memaddr + 4;
        psymoff(addr, ISYM, "");

    } else if (op->type & FAOFF28) {
        unsigned addr = (word & 0x3ffffff) << 2;
        psymoff(addr, ISYM, "");
    }

    /*
     * Last argument.
     */
    if (op->type & FRSB)                /* ... (rs) */
        print ("(%s)", regname[word >> 21 & 31]);
    if (op->type & FSIZE)               /* bit field size */
        print (", %x", ((word >> 11) & 0x1f) + 1);
    if (op->type & FMSB)                /* bit field msb */
        print (", %x", ((word >> 11) & 0x1f) + 1 -
            ((word >> 6) & 0x1f));
}
