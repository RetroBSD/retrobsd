/*
Copyright (c) 2013, Alexey Frunze
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

/*****************************************************************************/
/*                                                                           */
/*                               MIPS icache                                 */
/*                                                                           */
/*****************************************************************************/

// the For best performance, this file should be compiled using gcc's -O3 option.

// Define this macro to enable statistics
//#define STATS

// Define this macro to do a more rigorous check
// for invalid/unsupported instructions
// (at the expense of performance, of course).
//#define CHECK_INVALID_INSTR

#define STATIC static

// Rename non-static functions and variables
// to prevent name collisions with other code
// when compiling together with it.
#define DoSysCall       _icDoSysCall_
#define main            _icmain_
#define Regs            _icRegs_
#define HostRegs        _icHostRegs_

typedef unsigned char uchar, uint8;
typedef signed char schar, int8;
typedef unsigned short ushort, uint16;
typedef short int16;
typedef unsigned uint, uint32, size_t;
typedef int int32, ssize_t, off_t;
typedef unsigned long ulong;
typedef long long longlong, int64;
typedef unsigned long long ulonglong, uint64;

#define C_ASSERT(expr) extern char CAssertExtern[(expr)?1:-1]

//C_ASSERT(CHAR_BIT == 8);
C_ASSERT(sizeof(int) == 4);
C_ASSERT(sizeof(long) == 4);
C_ASSERT(sizeof(longlong) == 8);
C_ASSERT(sizeof(void*) == 4);
C_ASSERT(sizeof(void(*)()) == 4);
C_ASSERT(sizeof(size_t) == 4);
C_ASSERT(sizeof(ssize_t) == 4);
C_ASSERT(sizeof(off_t) == 4);
C_ASSERT(sizeof(uint16) == 2);
C_ASSERT(sizeof(uint32) == 4);
C_ASSERT(sizeof(uint64) == 8);


#pragma pack(push,1)

typedef struct
{
  uint32 a_magic;   /* magic number */
#define OMAGIC 0407 /* old impure format */

  uint32 a_text;    /* size of text segment */
  uint32 a_data;    /* size of initialized data */
  uint32 a_bss;     /* size of uninitialized data */
  uint32 a_reltext; /* size of text relocation info */
  uint32 a_reldata; /* size of data relocation info */
  uint32 a_syms;    /* size of symbol table */
  uint32 a_entry;   /* entry point */
} AoutHdr;

#pragma pack(pop)

C_ASSERT(sizeof(AoutHdr) == 32);


static inline
void* memset(void* dst, int ch, size_t size)
{
  unsigned char *p = dst;
  while (size--)
    *p++ = ch;
  return dst;
}

// flags for RetroBSD's open():
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_APPEND 0x0008
#define O_CREAT  0x0200
#define O_TRUNC  0x0400
#define O_TEXT   0x0000
#define O_BINARY 0x0000

// flags for RetroBSD's lseek():
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

static
void exit(int code)
{
  asm volatile ("move $4, %0\n"
                "syscall 1" // SYS_exit
                :
                : "r" (code)
                : "$2", "$4");
}

static
int open(const char* name, int oflags)
{
  int handle;
  asm volatile ("move $4, %1\n"
                "move $5, %2\n"
                "syscall 5\n" // SYS_open
                "nop\n"
                "nop\n"
                "move %0, $2\n"
                : "=r" (handle)
                : "r" (name), "r" (oflags)
                : "$2", "$4", "$5");
  return handle;
}

static
ssize_t read(int handle, void* buf, size_t size)
{
  ssize_t sz;
  asm volatile ("move $4, %1\n"
                "move $5, %2\n"
                "move $6, %3\n"
                "syscall 3\n" // SYS_read
                "nop\n"
                "nop\n"
                "move %0, $2\n"
                : "=r" (sz)
                : "r" (handle), "r" (buf), "r" (size)
                : "$2", "$4", "$5", "$6", "memory");
  return sz;
}

static
ssize_t write(int handle, const void* buf, size_t size)
{
  ssize_t sz;
  asm volatile ("move $4, %1\n"
                "move $5, %2\n"
                "move $6, %3\n"
                "syscall 4\n" // SYS_write
                "nop\n"
                "nop\n"
                "move %0, $2\n"
                : "=r" (sz)
                : "r" (handle), "r" (buf), "r" (size)
                : "$2", "$4", "$5", "$6", "memory"); // WTF? I shouldn't need "memory" here !!!
  return sz;
}

static
off_t lseek(int handle, off_t pos, int whence)
{
  off_t p;
  asm volatile ("move $4, %1\n"
                "move $5, %2\n"
                "move $6, %3\n"
                "syscall 19\n" // SYS_lseek
                "nop\n"
                "nop\n"
                "move %0, $2\n"
                : "=r" (p)
                : "r" (handle), "r" (pos), "r" (whence)
                : "$2", "$4", "$5", "$6");
  return p;
}

#if 0
// close() is unused. We rely on the system to close
// all files/handles on process termination.
static
int close(int handle)
{
  int err;
  asm volatile ("move $4, %1\n"
                "syscall 6\n" // SYS_close
                "nop\n"
                "nop\n"
                "move %0, $2\n"
                : "=r" (err)
                : "r" (handle)
                : "$2", "$4");
  return err;
}
#endif

static
void printchr(int ch)
{
  char c = ch;
  write(1, &c, 1);
}

static
void printstr(const char* s)
{
  while (*s != '\0')
    write(1, s++, 1);
}

#ifdef STATS
static
void printdec(int n)
{
  unsigned un = n;
  if (n < 0)
  {
    un = -un;
    printchr('-');
  }
  if (un >= 10)
    printdec(un / 10);
  printchr('0' + un % 10);
}

static
char* Bin64ToDec(uint64 n)
{
  // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
  static char s[64 / 3 + 1 + 1];
  char* p = s;
  int i;

  memset(s, '0', sizeof s - 1);
  s[sizeof s - 1] = '\0';

  for (i = 0; i < 64; i++)
  {
    int j, carry;

    // Extract the most significant bit of n into carry
    carry = (n >> 63) & 1;
    // Shift n left
    n <<= 1;

    // Add s[] to itself in decimal, doubling it,
    // and add carry to it
    for (j = sizeof s - 2; j >= 0; j--)
    {
      s[j] += s[j] - '0' + carry;

      carry = s[j] > '9';

      if (carry)
        s[j] -= 10;
    }
  }

  while ((*p == '0') && (p < &s[sizeof s - 2]))
    p++;

  return p;
}

static
void printdec64(int64 n)
{
  uint64 un = n;
  if (n < 0)
  {
    un = -un;
    printchr('-');
  }
  printstr(Bin64ToDec(un));
}
#endif

static
void printhex(unsigned n)
{
  int i;
  for (i = 0; i < 8; i++)
    printchr("0123456789ABCDEF"[(n >> 28) & 15]), n <<= 4;
}

#ifdef STATS
uint64 EmulateCnt = 0;
uint64 CacheHits = 0;
uint64 CacheHits2 = 0;
uint64 CacheMisses = 0;
#endif


#define REG_GP 28
#define REG_SP 29
#define REG_RA 31
#define REG_LO 32
#define REG_HI 33
#define REG_PC 34
uint32 Regs[32 + 3];
uint32 HostRegs[32 + 3];


#define CACHE_BYTES_PER_INSTR  4
#ifndef CACHE_INSTRS_PER_ENTRY
#define CACHE_INSTRS_PER_ENTRY 8
#endif
#define CACHE_BYTES_PER_ENTRY  (CACHE_BYTES_PER_INSTR * CACHE_INSTRS_PER_ENTRY)

#ifndef CACHE_ENTRIES_PER_WAY
#define CACHE_ENTRIES_PER_WAY  64
#endif
#ifndef CACHE_WAYS
#define CACHE_WAYS             4
#endif
#define CACHE_ENTRIES_TOTAL    (CACHE_ENTRIES_PER_WAY * CACHE_WAYS)

#define CACHE_SIZE             (CACHE_BYTES_PER_ENTRY * CACHE_ENTRIES_TOTAL)

STATIC uint32 Cache[CACHE_ENTRIES_PER_WAY][CACHE_WAYS][CACHE_INSTRS_PER_ENTRY];
STATIC uint32 CacheTagAndValid[CACHE_ENTRIES_PER_WAY][CACHE_WAYS];

STATIC int CachedCnt = 0;
STATIC uint32* CachedInstr = &Cache[0][0][0];


extern void _icstart_(int argc, char** argv, char** env);
STATIC void Emulate(void);

STATIC int ExeHandle = -1;
STATIC uint32 ExeOffs = 0;

int main(int argc, char** argv, char** env)
{
  AoutHdr aoutHdr;

  if ((ExeHandle = open(argv[0], O_BINARY | O_RDONLY)) < 0)
  {
    printstr("Can't open "); printstr(argv[0]); printchr('\n');
    exit(-1);
  }

  if (read(ExeHandle, &aoutHdr, sizeof aoutHdr) != sizeof aoutHdr)
  {
    printstr("Can't read "); printstr(argv[0]); printchr('\n');
    exit(-1);
  }

  if (aoutHdr.a_magic != OMAGIC)
  {
    printstr(argv[0]); printstr(" is not an a.out file\n");
    exit(-1);
  }

  ExeOffs = sizeof aoutHdr + aoutHdr.a_text + aoutHdr.a_data;

  memset(CacheTagAndValid, 0xFF, sizeof CacheTagAndValid); // invalidate cache
  Regs[4] = argc;
  Regs[5] = (uint32)argv;
  Regs[6] = (uint32)env;
  // Regs[REG_SP] = ...; // _icstart() has done this
  // Regs[REG_GP] = (uint32)&_gp; // _start() will do this if needed
  Regs[REG_PC] = (uint32)&_icstart_;
  Emulate(); // isn't supposed to return...
  exit(-1); // ... but let's exit explicitly just in case
  return -1;
}

extern const char _text_begin;
extern const char _text_end;

STATIC inline
uint32 FetchProgramWord(uint32 Addr)
{
  uint32 ofs, idx, tag, way;

  if (CachedCnt > 0)
  {
    CachedCnt--;
    // Cache hit
#ifdef STATS
    CacheHits++;
    CacheHits2++;
#endif
    return *++CachedInstr;
  }

  if (Addr < (uint32)&_text_begin ||
      Addr >= (uint32)&_text_end)
  {
    CachedCnt = 0;
    CachedInstr = (uint32*)Addr;
    return *CachedInstr;
  }

  ofs = Addr % CACHE_BYTES_PER_ENTRY / CACHE_BYTES_PER_INSTR;
  idx = Addr / CACHE_BYTES_PER_ENTRY % CACHE_ENTRIES_PER_WAY;
  tag = Addr / CACHE_BYTES_PER_ENTRY / CACHE_ENTRIES_PER_WAY;
  for (way = 0; way < CACHE_WAYS; way++)
  {
    if (CacheTagAndValid[idx][way] == tag)
    {
      // Cache hit
#ifdef STATS
      CacheHits++;
#endif
      CachedCnt = CACHE_INSTRS_PER_ENTRY - 1 - ofs;
      CachedInstr = &Cache[idx][way][ofs];
      return *CachedInstr;
    }
  }

  // Cache miss
#ifdef STATS
  CacheMisses++;
#endif

  for (way = 0; way < CACHE_WAYS; way++)
  {
    if (CacheTagAndValid[idx][way] & 0x80000000)
    {
      // Use an invalid entry
      goto lreuse;
    }
  }

  // Reuse a valid entry (need to choose one for eviction and refill,
  // preferably not penalizing the same entry over and over again)
  {
    static uint32 w = CACHE_WAYS - 1;
    w = (w + 1) % CACHE_WAYS; // pseudo-LRU
    way = w;
  }

lreuse:

  if (lseek(ExeHandle,
            ExeOffs + (Addr / CACHE_BYTES_PER_ENTRY * CACHE_BYTES_PER_ENTRY - (uint32)&_text_begin),
            SEEK_SET) < 0 ||
      read(ExeHandle, Cache[idx][way], CACHE_BYTES_PER_ENTRY) != CACHE_BYTES_PER_ENTRY)
  {
    printstr("\nCan't read into the cache from the executable file\n");
    exit(-1);
  }

  CacheTagAndValid[idx][way] = tag;

  CachedCnt = CACHE_INSTRS_PER_ENTRY - 1 - ofs;
  CachedInstr = &Cache[idx][way][ofs];
  return *CachedInstr;
}

extern int DoSysCall(uint32 instr);

#ifdef STATS
STATIC
int DoSysCall2(uint32 instr)
{
  uint32 code = (instr >> 6) & 0xFFFFF;

  // intercept exit() syscall
  if ((code == 0 && Regs[2] == 10) || // SPIM's exit()
      (code == /*SYS_exit*/1)) // RetroBSD's exit()
  {
    uint32 idx, way, used = 0;
    for (idx = 0; idx < CACHE_ENTRIES_PER_WAY; idx++)
      for (way = 0; way < CACHE_WAYS; way++)
        used += CacheTagAndValid[idx][way] < 0x80000000;
    printstr("\n"); printdec64(EmulateCnt); printstr(" instruction(s) emulated\n");
    printdec((int)used); printchr('/'); printdec(CACHE_ENTRIES_TOTAL); printstr(" cache entries used\n");
    printdec64(CacheHits); printchr('(');
    printdec64(CacheHits2); printstr(")/");
    printdec64(CacheMisses); printstr(" cache hits(hits2)/misses\n");
  }
  return DoSysCall(instr);
}
#undef DoSysCall
#define DoSysCall DoSysCall2
#endif

static inline uint8 ReadByte(uint32 Addr)
{
  return *(uint8*)Addr;
}
static inline void WriteByte(uint32 Addr, uint8 Val)
{
  *(uint8*)Addr = Val;
}

static inline uint16 ReadHalfWord(uint32 Addr)
{
  return *(uint16*)Addr;
}
static inline void WriteHalfWord(uint32 Addr, uint16 Val)
{
  *(uint16*)Addr = Val;
}

static inline uint32 ReadWord(uint32 Addr)
{
  return *(uint32*)Addr;
}
static inline void WriteWord(uint32 Addr, uint32 Val)
{
  *(uint32*)Addr = Val;
}

/*
  Supported instructions:

    add, addi, addiu, addu, and, andi,
    bal, beq, beql, bgez, bgezal, bgezall,
    bgezl, bgtz, bgtzl, blez, blezl, bltz,
    bltzal, bltzall, bltzl, bne, bnel, break,
    clo, clz,
    div, divu,
    ext,
    ins,
    j, jal, jalr, jr,
    lb, lbu, lh, lhu, lui, lw, lwl, lwr,
    madd, maddu, mfhi, mflo, movn, movz, msub,
    msubu, mthi, mtlo, mul, mult, multu,
    nop, nor,
    or, ori,
    rotr, rotrv,
    sb, seb, seh, sh, sll, sllv, slt, slti, sltiu,
    sltu, sra, srav, srl, srlv, sub, subu, sw,
    swl, swlr, syscall,
    teq, teqi, tge, tgei, tgeiu, tgeu, tlt, tlti,
    tltiu, tltu, tne, tnei,
    wsbh,
    xor, xori

  Unsupported instructions:

    bc2f, bc2fl, bc2t, bc2tl,
    cache, cfc2, cop0, cop2, ctc2,
    deret, di,
    ehb, ei, eret,
    jalr.hb, jr.hb,
    ll, lwc2,
    mfc0, mfc2, mtc0, mtc2, mthc2,
    pref,
    rdhwr, rdpgpr,
    sc, sdbbp, ssnop, swc2, sync, synci,
    wait, wrpgpr
*/

STATIC void DoBreak(uint32 instr);
STATIC void DoTrap(uint32 instr);
STATIC void DoOverflow(void);
STATIC void DoInvalidInstruction(uint32 instr);

STATIC
uint32 CountLeadingZeroes(uint32 n)
{
#if 0
  uint32 c = 0;
  if (n == 0)
    return 32;
  while (n < 0x80000000)
    n <<= 1, c++;
#else
  uint32 c;
  asm volatile("clz %0, %1" : "=r" (c) : "r" (n));
#endif
  return c;
}

STATIC
uint32 CountLeadingOnes(uint32 n)
{
#if 0
  uint32 c = 0;
  while (n >= 0x80000000)
    n <<= 1, c++;
#else
  uint32 c;
  asm volatile("clo %0, %1" : "=r" (c) : "r" (n));
#endif
  return c;
}

STATIC
uint32 ShiftRightArithm(uint32 n, uint32 c)
{
#if 0
  uint32 s = -(n >> 31);
  n >>= c;
  n |= s << (31 - c) << 1;
  return n;
#else
  uint32 nn;
  asm volatile("srav %0, %1, %2" : "=r" (nn) : "r" (n), "r" (c));
  return nn;
#endif
}

STATIC
uint32 RotateRight(uint32 n, uint32 c)
{
#if 0
  return (n >> c) | (n << (31 - c) << 1);
#else
  uint32 nn;
  asm volatile("rotrv %0, %1, %2" : "=r" (nn) : "r" (n), "r" (c));
  return nn;
#endif
}

STATIC
void Emulate(void)
{
  int delaySlot = 0;
  uint32 postDelaySlotPc = 0;
  uint32 instr = 0;

  for (;;)
  {
    const uint32 pc = Regs[REG_PC];
    uint32 nextPc = pc + 4;
    /*const uint32*/ instr = FetchProgramWord(pc);
#if 0
    const uint32 op = instr >> 26;
    const uint32 r1 = (instr >> 21) & 0x1F;
    const uint32 r2 = (instr >> 16) & 0x1F;
    const uint32 r3 = (instr >> 11) & 0x1F;
    const uint32 shft = (instr >> 6) & 0x1F;
    const uint32 fxn = instr & 0x3F;
    const uint32 imm16 = instr & 0xFFFF;
    const uint32 simm16 = (int16)imm16;
    const uint32 jtgt = instr & 0x3FFFFFF;
#else
#define op     (instr >> 26)
#define r1     ((instr >> 21) & 0x1F)
#define r2     ((instr >> 16) & 0x1F)
#define r3     ((instr >> 11) & 0x1F)
#define shft   ((instr >> 6) & 0x1F)
#define fxn    (instr & 0x3F)
#define imm16  (instr & 0xFFFF)
#define simm16 ((int16)imm16)
#define jtgt   (instr & 0x3FFFFFF)
#endif

    switch (op)
    {
    case 0:
      switch (fxn)
      {
      case 0:
#ifdef CHECK_INVALID_INSTR
        if (r1)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r2] << shft;
        break; // sll d,w,shft
      case 2:
        switch (r1)
        {
        case 0: Regs[r3] = Regs[r2] >> shft; break; // srl d,w,shft
        case 1: Regs[r3] = RotateRight(Regs[r2], shft); break; // rotr d,w,shft
        default: goto lInvalidInstruction;
        }
        break;
      case 3:
#ifdef CHECK_INVALID_INSTR
        if (r1)
          goto lInvalidInstruction;
#endif
        Regs[r3] = ShiftRightArithm(Regs[r2], shft);
        break; // sra d,w,shft
      case 4:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r2] << (Regs[r1] & 31);
        break; // sllv d,w,s
      case 6:
        switch (shft)
        {
        case 0: Regs[r3] = Regs[r2] >> (Regs[r1] & 31); break; // srlv d,w,s
        case 1: Regs[r3] = RotateRight(Regs[r2], Regs[r1] & 31); break; // rotrv d,w,s
        default: goto lInvalidInstruction;
        }
        break;
      case 7:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = ShiftRightArithm(Regs[r2], Regs[r1] & 31);
        break; // srav d,w,s
      case 8:
#ifdef CHECK_INVALID_INSTR
        if (r2 | r3 | shft)
          goto lInvalidInstruction;
#endif
        nextPc = Regs[r1]; delaySlot = 1;
        break; // jr s
      case 9:
#ifdef CHECK_INVALID_INSTR
        if (r2 | shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = nextPc + 4; nextPc = Regs[r1]; delaySlot = 1;
        break; // jalr [d,] s
      case 10:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        if (Regs[r2] == 0) Regs[r3] = Regs[r1];
        break; // movz d,s,t
      case 11:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        if (Regs[r2]) Regs[r3] = Regs[r1];
        break; // movn d,s,t
      case 12:
        {
          // RetroBSD may advance PC on returning from a syscall handler,
          // skipping 2 instructions that follow the syscall instruction.
          // Those 2 instructions typically set C's errno variable and
          // are either executed on error or skipped on success.
          // Account for this peculiarity.
          uint32 skip = DoSysCall(instr);
          nextPc += skip * 4; CachedCnt -= skip; CachedInstr += skip;
        }
        break; // syscall code
      case 13: goto lBreak; break; // break code
      case 16:
#ifdef CHECK_INVALID_INSTR
        if (r1 | r2 | shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[REG_HI];
        break; // mfhi d
      case 17:
#ifdef CHECK_INVALID_INSTR
        if (r2 | r3 | shft)
          goto lInvalidInstruction;
#endif
        Regs[REG_HI] = Regs[r1];
        break; // mthi s
      case 18:
#ifdef CHECK_INVALID_INSTR
        if (r1 | r2 | shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[REG_LO];
        break; // mflo d
      case 19:
#ifdef CHECK_INVALID_INSTR
        if (r2 | r3 | shft)
          goto lInvalidInstruction;
#endif
        Regs[REG_LO] = Regs[r1];
        break; // mtlo s
      case 24:
#ifdef CHECK_INVALID_INSTR
        if (r3 | shft)
          goto lInvalidInstruction;
#endif
        {
          int64 p = (int64)(int32)Regs[r1] * (int32)Regs[r2];
          Regs[REG_LO] = (uint32)p;
          Regs[REG_HI] = (uint32)(p >> 32);
        }
        break; // mult s,t
      case 25:
#ifdef CHECK_INVALID_INSTR
        if (r3 | shft)
          goto lInvalidInstruction;
#endif
        {
          uint64 p = (uint64)Regs[r1] * Regs[r2];
          Regs[REG_LO] = (uint32)p;
          Regs[REG_HI] = (uint32)(p >> 32);
        }
        break; // multu s,t
      case 26:
#ifdef CHECK_INVALID_INSTR
        if (r3 | shft)
          goto lInvalidInstruction;
#endif
        if (!(Regs[r2] == 0 || (Regs[r1] == 0x80000000 && Regs[r2] == 0xFFFFFFFF)))
          Regs[REG_LO] = (int32)Regs[r1] / (int32)Regs[r2], Regs[REG_HI] = (int32)Regs[r1] % (int32)Regs[r2];
        break; // div s,t
      case 27:
#ifdef CHECK_INVALID_INSTR
        if (r3 | shft)
          goto lInvalidInstruction;
#endif
        if (Regs[r2])
          Regs[REG_LO] = Regs[r1] / Regs[r2], Regs[REG_HI] = Regs[r1] % Regs[r2];
        break; // divu s,t
      case 32:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        {
          uint32 sum = Regs[r1] + Regs[r2];
          if (((Regs[r1] ^ Regs[r2] ^ 0x80000000) & 0x80000000) &&
              ((sum ^ Regs[r1]) & 0x80000000))
            goto lOverflow;
          Regs[r3] = sum;
        }
        break; // add d,s,t
      case 33:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r1] + Regs[r2];
        break; // addu d,s,t
      case 34:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        {
          uint32 diff = Regs[r1] - Regs[r2];
          if (((Regs[r1] ^ Regs[r2]) & 0x80000000) &&
              ((diff ^ Regs[r1]) & 0x80000000))
            goto lOverflow;
          Regs[r3] = diff;
        }
        break; // sub d,s,t
      case 35:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r1] - Regs[r2];
        break; // subu d,s,t
      case 36:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r1] & Regs[r2];
        break; // and d,s,t
      case 37:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r1] | Regs[r2];
        break; // or d,s,t
      case 38:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r1] ^ Regs[r2];
        break; // xor d,s,t
      case 39:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = ~(Regs[r1] | Regs[r2]);
        break; // nor d,s,t
      case 42:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = (int32)Regs[r1] < (int32)Regs[r2];
        break; // slt d,s,t
      case 43:
#ifdef CHECK_INVALID_INSTR
        if (shft)
          goto lInvalidInstruction;
#endif
        Regs[r3] = Regs[r1] < Regs[r2];
        break; // sltu d,s,t

      case 48: if ((int32)Regs[r1] >= (int32)Regs[r2]) goto lTrap; break; // tge s,t
      case 49: if (Regs[r1] >= Regs[r2]) goto lTrap; break; // tgeu s,t
      case 50: if ((int32)Regs[r1] < (int32)Regs[r2]) goto lTrap; break; // tlt s,t
      case 51: if (Regs[r1] < Regs[r2]) goto lTrap; break; // tltu s,t
      case 52: if (Regs[r1] == Regs[r2]) goto lTrap; break; // teq s,t
      case 53: if (Regs[r1] != Regs[r2]) goto lTrap; break; // tne s,t

      default: goto lInvalidInstruction;
      }
      break;

    case 1:
      switch (r2)
      {
      case 0: if ((int32)Regs[r1] < 0) nextPc += simm16 << 2, delaySlot = 1; break; // bltz s,p
      case 1: if ((int32)Regs[r1] >= 0) nextPc += simm16 << 2, delaySlot = 1; break; // bgez s,p
      case 2: if ((int32)Regs[r1] < 0) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // bltzl s,p
      case 3: if ((int32)Regs[r1] >= 0) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // bgezl s,p

      case 8: if ((int32)Regs[r1] >= (int32)simm16) goto lTrap; break; // tgei s,j
      case 9: if (Regs[r1] >= (uint32)simm16) goto lTrap; break; // tgeiu s,j
      case 10: if ((int32)Regs[r1] < (int32)simm16) goto lTrap; break; // tlti s,j
      case 11: if (Regs[r1] < (uint32)simm16) goto lTrap; break; // tltiu s,j
      case 12: if (Regs[r1] == (uint32)simm16) goto lTrap; break; // teqi s,j
      case 14: if (Regs[r1] != (uint32)simm16) goto lTrap; break; // tnei s,j

      case 16: Regs[REG_RA] = nextPc + 4; if ((int32)Regs[r1] < 0) nextPc += simm16 << 2, delaySlot = 1; break; // bltzal s,p
      case 17: Regs[REG_RA] = nextPc + 4; if ((int32)Regs[r1] >= 0) nextPc += simm16 << 2, delaySlot = 1; break; // bgezal s,p
      case 18: Regs[REG_RA] = nextPc + 4; if ((int32)Regs[r1] < 0) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // bltzall s,p
      case 19: Regs[REG_RA] = nextPc + 4; if ((int32)Regs[r1] >= 0) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // bgezall s,p

      default: goto lInvalidInstruction;
      }
      break;

    case 2: nextPc = (pc & 0xF0000000) | (jtgt << 2); delaySlot = 1; break; // j target
    case 3: Regs[REG_RA] = nextPc + 4; nextPc = (pc & 0xF0000000) | (jtgt << 2); delaySlot = 1; break; // jal target

    case 4: if (Regs[r1] == Regs[r2]) nextPc += simm16 << 2, delaySlot = 1; break; // beq s,t,p
    case 5: if (Regs[r1] != Regs[r2]) nextPc += simm16 << 2, delaySlot = 1; break; // bne s,t,p
    case 6:
#ifdef CHECK_INVALID_INSTR
      if (r2)
        goto lInvalidInstruction;
#endif
      if ((int32)Regs[r1] <= 0) nextPc += simm16 << 2, delaySlot = 1; break; // blez s,p
    case 7:
#ifdef CHECK_INVALID_INSTR
      if (r2)
        goto lInvalidInstruction;
#endif
      if ((int32)Regs[r1] > 0) nextPc += simm16 << 2, delaySlot = 1; break; // bgtz s,p

    case 8:
      {
        uint32 sum = Regs[r1] + simm16;
        if (((Regs[r1] ^ simm16 ^ 0x80000000) & 0x80000000) &&
            ((sum ^ Regs[r1]) & 0x80000000))
          goto lOverflow;
        Regs[r2] = sum;
      }
      break; // addi d,s,const
    case 9: Regs[r2] = Regs[r1] + simm16; break; // addiu d,s,const
    case 10: Regs[r2] = (int32)Regs[r1] < (int32)simm16; break; // slti d,s,const
    case 11: Regs[r2] = Regs[r1] < (uint32)simm16; break; // sltiu d,s,const
    case 12: Regs[r2] = Regs[r1] & imm16; break; // andi d,s,const
    case 13: Regs[r2] = Regs[r1] | imm16; break; // ori d,s,const
    case 14: Regs[r2] = Regs[r1] ^ imm16; break; // xori d,s,const
    case 15:
#ifdef CHECK_INVALID_INSTR
      if (r1)
        goto lInvalidInstruction;
#endif
      Regs[r2] = imm16 << 16; break; // lui d,const

    case 20: if (Regs[r1] == Regs[r2]) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // beql s,t,p
    case 21: if (Regs[r1] != Regs[r2]) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // bnel s,t,p
    case 22:
#ifdef CHECK_INVALID_INSTR
      if (r2)
        goto lInvalidInstruction;
#endif
      if ((int32)Regs[r1] <= 0) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // blezl s,p
    case 23:
#ifdef CHECK_INVALID_INSTR
      if (r2)
        goto lInvalidInstruction;
#endif
      if ((int32)Regs[r1] > 0) nextPc += simm16 << 2, delaySlot = 1; else nextPc += 4, CachedCnt--, ++CachedInstr; break; // bgtzl s,p

    case 28:
#ifdef CHECK_INVALID_INSTR
      if (shft)
        goto lInvalidInstruction;
#endif
      switch (fxn)
      {
      case 0:
#ifdef CHECK_INVALID_INSTR
        if (r3)
          goto lInvalidInstruction;
#endif
        {
          int64 p = (int64)(int32)Regs[r1] * (int32)Regs[r2];
          if (Regs[REG_LO] > 0xFFFFFFFF - (uint32)p)
            Regs[REG_HI]++;
          Regs[REG_LO] += (uint32)p;
          Regs[REG_HI] += (uint32)(p >> 32);
        }
        break; // madd s,t
      case 1:
#ifdef CHECK_INVALID_INSTR
        if (r3)
          goto lInvalidInstruction;
#endif
        {
          uint64 p = (uint64)Regs[r1] * Regs[r2];
          if (Regs[REG_LO] > 0xFFFFFFFF - (uint32)p)
            Regs[REG_HI]++;
          Regs[REG_LO] += (uint32)p;
          Regs[REG_HI] += (uint32)(p >> 32);
        }
        break; // maddu s,t
      case 2: Regs[r3] = Regs[r1] * Regs[r2]; break; // mul d,s,t
      case 4:
#ifdef CHECK_INVALID_INSTR
        if (r3)
          goto lInvalidInstruction;
#endif
        {
          int64 p = (int64)(int32)Regs[r1] * (int32)Regs[r2];
          if (Regs[REG_LO] < (uint32)p)
            Regs[REG_HI]--;
          Regs[REG_LO] -= (uint32)p;
          Regs[REG_HI] -= (uint32)(p >> 32);
        }
        break; // msub s,t
      case 5:
#ifdef CHECK_INVALID_INSTR
        if (r3)
          goto lInvalidInstruction;
#endif
        {
          uint64 p = (uint64)Regs[r1] * Regs[r2];
          if (Regs[REG_LO] < (uint32)p)
            Regs[REG_HI]--;
          Regs[REG_LO] -= (uint32)p;
          Regs[REG_HI] -= (uint32)(p >> 32);
        }
        break; // msubu s,t

      case 32:
#ifdef CHECK_INVALID_INSTR
        if (r1 != r2)
          goto lInvalidInstruction;
#endif
        Regs[r3] = CountLeadingZeroes(Regs[r2]); break; // clz d,s
      case 33:
#ifdef CHECK_INVALID_INSTR
        if (r1 != r2)
          goto lInvalidInstruction;
#endif
        Regs[r3] = CountLeadingOnes(Regs[r2]); break; // clo d,s

      default: goto lInvalidInstruction;
      }
      break;

    case 31:
      switch (fxn)
      {
      case 0:
        if (shft + r3 <= 31)
        {
          uint size = r3 + 1;
          uint32 mask = (0xFFFFFFFF >> (32 - size)) << shft;
          Regs[r2] = (Regs[r1] & mask) >> shft;
        }
        break; // ext t,s,pos,sz
      case 4:
        if (r3 >= shft)
        {
          uint size = r3 - shft + 1;
          uint32 mask = (0xFFFFFFFF >> (32 - size)) << shft;
          Regs[r2] = (Regs[r2] & ~mask) | ((Regs[r1] << shft) & mask);
        }
        break; // ins t,s,pos,sz
      case 32:
#ifdef CHECK_INVALID_INSTR
        if (r1)
          goto lInvalidInstruction;
#endif
        switch (shft)
        {
        case 2: Regs[r3] = ((Regs[r2] & 0x00FF) << 8) |
                           ((Regs[r2] & 0xFF00) >> 8) |
                           ((Regs[r2] & 0x00FF0000) << 8) |
                           ((Regs[r2] & 0xFF000000) >> 8); break; // wsbh d,t
        case 16: Regs[r3] = (int8)Regs[r2]; break; // seb d,t
        case 24: Regs[r3] = (int16)Regs[r2]; break; // seh d,t
        default: goto lInvalidInstruction;
        }
        break;

      default: goto lInvalidInstruction;
      }
      break;

    case 32: Regs[r2] = (int8)ReadByte(Regs[r1] + simm16); break; // lb t,o(b)
    case 33: Regs[r2] = (int16)ReadHalfWord(Regs[r1] + simm16); break; // lh t,o(b)
    case 34:
      {
        uint32 v = ReadByte(Regs[r1] + simm16);
        v = (v << 8) | ReadByte(Regs[r1] + simm16 - 1);
        Regs[r2] = (Regs[r2] & 0xFFFF) | (v << 16);
      }
      break; // lwl t,o(b)
    case 35: Regs[r2] = ReadWord(Regs[r1] + simm16); break; // lw t,o(b)
    case 36: Regs[r2] = ReadByte(Regs[r1] + simm16); break; // lbu t,o(b)
    case 37: Regs[r2] = ReadHalfWord(Regs[r1] + simm16); break; // lhu t,o(b)
    case 38:
      {
        uint32 v = ReadByte(Regs[r1] + simm16);
        v |= (uint32)ReadByte(Regs[r1] + simm16 + 1) << 8;
        Regs[r2] = (Regs[r2] & 0xFFFF0000) | v;
      }
      break; // lwr t,o(b)

    case 40: WriteByte(Regs[r1] + simm16, (uint8)Regs[r2]); break; // sb t,o(b)
    case 41: WriteHalfWord(Regs[r1] + simm16, (uint16)Regs[r2]); break; // sh t,o(b)
    case 42:
      WriteByte(Regs[r1] + simm16, (uint8)(Regs[r2] >> 24));
      WriteByte(Regs[r1] + simm16 - 1, (uint8)(Regs[r2] >> 16));
      break; // swl t,o(b)
    case 43: WriteWord(Regs[r1] + simm16, Regs[r2]); break; // sw t,o(b)
    case 46:
      WriteByte(Regs[r1] + simm16, (uint8)Regs[r2]);
      WriteByte(Regs[r1] + simm16 + 1, (uint8)(Regs[r2] >> 8));
      break; // swr t,o(b)

    default:
      goto lInvalidInstruction;
    }

    Regs[0] = 0;

    Regs[REG_PC] = nextPc;

    if (delaySlot)
    {
      if (delaySlot == 1)
      {
         postDelaySlotPc = nextPc;
         Regs[REG_PC] = pc + 4;
         delaySlot = 2;
      }
      else
      {
         Regs[REG_PC] = postDelaySlotPc;
         delaySlot = 0;
         CachedCnt = 0;
      }
    }

#ifdef STATS
    EmulateCnt++;
#endif
  } // for (;;)

lBreak:
  DoBreak(instr);
  return;

lTrap:
  DoTrap(instr);
  return;

lOverflow:
  DoOverflow();
  return;

lInvalidInstruction:
  DoInvalidInstruction(instr);
  return;
#if 01
#undef op
#undef r1
#undef r2
#undef r3
#undef shft
#undef fxn
#undef imm16
#undef simm16
#undef jtgt
#endif
}

STATIC
void DoBreak(uint32 instr)
{
  uint32 code = (instr >> 16) & 0x3FF; // are there really another/extra 10 bits of the code?

  switch (code)
  {
  case 6:
    printstr("\nBreak: Signed division overflow");
    break;
  case 7:
    printstr("\nBreak: Division by 0");
    break;
  default:
    printstr("\nBreak: Code: 0x"); printhex(code);
    break;
  }
  printstr(" at PC = 0x"); printhex(Regs[REG_PC]); printchr('\n');
  exit(-1);
}

STATIC
void DoTrap(uint32 instr)
{
  uint32 code = (instr >> 6) & 0x3FF;

  switch (code)
  {
  case 6:
    printstr("\nTrap: Signed division overflow");
    break;
  case 7:
    printstr("\nTrap: Division by 0");
    break;
  default:
    printstr("\nTrap: Code: 0x"); printhex(code);
    break;
  }
  printstr(" at PC = 0x"); printhex(Regs[REG_PC]); printchr('\n');
  exit(-1);
}

STATIC
void DoOverflow(void)
{
  printstr("Signed integer addition/subtraction overflow");
  printstr(" at PC = 0x"); printhex(Regs[REG_PC]); printchr('\n');
  exit(-1);
}

STATIC
void DoInvalidInstruction(uint32 instr)
{
  printstr("Invalid/unsupported instruction: Opcode: 0x"); printhex(instr);
  printstr(" at PC = 0x"); printhex(Regs[REG_PC]); printchr('\n');
  exit(-1);
}
