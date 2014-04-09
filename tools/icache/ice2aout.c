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
/*     MIPS ELF to RetroBSD a.out convertor with support for MIPS icache     */
/*                                                                           */
/*****************************************************************************/

#include <limits.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uchar, uint8;
typedef signed char schar, int8;
typedef unsigned short ushort, uint16;
typedef short int16;
#if UINT_MAX >= 0xFFFFFFFF
typedef unsigned uint32;
typedef int int32;
#else
typedef unsigned long uint32;
typedef long int32;
#endif
typedef unsigned uint;
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned long long ulonglong;
#if ULONG_MAX >= 0xFFFFFFFFFFFFFFFFULL
typedef unsigned long uint64;
typedef long int64;
#else
typedef unsigned long long uint64;
typedef long long int64;
#endif

#define C_ASSERT(expr) extern char CAssertExtern[(expr)?1:-1]

C_ASSERT(CHAR_BIT == 8);
C_ASSERT(sizeof(uint16) == 2);
C_ASSERT(sizeof(uint32) == 4);
C_ASSERT(sizeof(uint64) == 8);
C_ASSERT(sizeof(size_t) >= 4);

#pragma pack(push,1)

typedef struct
{
  uint8  e_ident[16];
  uint16 e_type;
  uint16 e_machine;
  uint32 e_version;
  uint32 e_entry;
  uint32 e_phoff;
  uint32 e_shoff;
  uint32 e_flags;
  uint16 e_ehsize;
  uint16 e_phentsize;
  uint16 e_phnum;
  uint16 e_shentsize;
  uint16 e_shnum;
  uint16 e_shstrndx;
} Elf32Hdr;

typedef struct
{
  uint32 sh_name;
  uint32 sh_type;
  uint32 sh_flags;
  uint32 sh_addr;
  uint32 sh_offset;
  uint32 sh_size;
  uint32 sh_link;
  uint32 sh_info;
  uint32 sh_addralign;
  uint32 sh_entsize;
} Elf32SectHdr;

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

C_ASSERT(sizeof(Elf32Hdr) == 52);
C_ASSERT(sizeof(Elf32SectHdr) == 40);
C_ASSERT(sizeof(AoutHdr) == 32);

typedef struct
{
  const char* Name;
  uint32 FileOffs;
  uint32 Addr;
  uint32 Size;
  uint32 Flags;
  uint32 Flags2;
} tSection;

tSection* Sections = NULL;
uint SectionCnt = 0;
char* SectNames = NULL;
uint32 EntryPointAddr = 0;

FILE* ElfFile = NULL;
FILE* AoutFile = NULL;
const char* AoutName = NULL;

void error(char* format, ...)
{
  va_list vl;
  va_start(vl, format);

  if (ElfFile)
    fclose(ElfFile);
  if (AoutFile)
    fclose(AoutFile);
  if (AoutName != NULL)
    remove(AoutName);

  puts("");
  vprintf(format, vl);

  va_end(vl);
  exit(-1);
}

int SectAddrCompare(const void* pa, const void* pb)
{
  const tSection *p1 = pa, *p2 = pb;
  if (p1->Addr < p2->Addr)
    return -1;
  else if (p1->Addr > p2->Addr)
    return +1;
  return 0;
}

void WriteZeroes(FILE* f, uint32 size)
{
  static const char zeroes[1024];
  while (size)
  {
    uint32 sz;

    if (size > sizeof zeroes)
      sz = sizeof zeroes;
    else
      sz = size;

    if (fwrite(zeroes, 1, sz, f) != sz)
      error("Can't write file\n");

    size -= sz;
  }
}

void CopyFileData(FILE* fto, FILE* ffrom, uint32 size)
{
  char buf[1024];
  while (size)
  {
    uint32 sz;

    if (size > sizeof buf)
      sz = sizeof buf;
    else
      sz = size;

    if (fread(buf, 1, sz, ffrom) != sz)
      error("Can't read file\n");

    if (fwrite(buf, 1, sz, fto) != sz)
      error("Can't write file\n");

    size -= sz;
  }
}

int main(int argc, char** argv)
{
  Elf32Hdr elfHdr;
  Elf32SectHdr sectHdr;
  uint idx;
  int unsupported = 0;
  int verbose = 0;
  AoutHdr aoutHdr;
  uint32 addr;
  uint32 endAddr = 0, codeEndAddr, bssStartAddr;

  if (argc > 1 && !strcmp(argv[1], "-v"))
  {
    verbose = 1;
    argc--;
    argv++;
  }

  if (argc != 3 ||
      !(ElfFile = fopen(argv[1], "rb")) ||
      !(AoutFile = fopen(AoutName = argv[2], "wb")))
    error("Usage:\n  ice2aout [-v] <mips32 elf executable> <RetroBSD mips32 a.out executable>\n");

  if (fread(&elfHdr, 1, sizeof elfHdr, ElfFile) != sizeof elfHdr)
    error("Can't read file\n");

  if (memcmp(elfHdr.e_ident, "\x7F""ELF", 4))
    error("Not an ELF file\n");
  if (elfHdr.e_ident[6] != 1)
    error("Not a v1 ELF file\n");
  if (elfHdr.e_ehsize != sizeof elfHdr)
    error("Unexpected ELF header size\n");
  if (elfHdr.e_shentsize != sizeof sectHdr)
    error("Unexpected ELF section size\n");

  if (elfHdr.e_ident[4] != 1)
    error("Not a 32-bit file\n");
  if (elfHdr.e_ident[5] != 1)
    error("Not a little-endian file\n");
  if (elfHdr.e_type != 2)
    error("Not an executable file\n");
  if (elfHdr.e_machine != 8)
    error("Not a MIPS executable\n");

  if (fseek(ElfFile, elfHdr.e_shoff + elfHdr.e_shstrndx * sizeof sectHdr, SEEK_SET))
    error("Can't read file\n");
  if (fread(&sectHdr, 1, sizeof sectHdr, ElfFile) != sizeof sectHdr)
    error("Can't read file\n");

  if ((SectNames = malloc(sectHdr.sh_size)) == NULL)
    error("Out of memory\n");

  if (fseek(ElfFile, sectHdr.sh_offset, SEEK_SET))
    error("Can't read file\n");
  if (fread(SectNames, 1, sectHdr.sh_size, ElfFile) != sectHdr.sh_size)
    error("Can't read file\n");

  if ((Sections = calloc(1, (elfHdr.e_shnum + 1) * sizeof(tSection))) == NULL)
    error("Out of memory\n");

  for (idx = 0; idx < elfHdr.e_shnum; idx++)
  {
    const char* name = "";

    if (fseek(ElfFile, elfHdr.e_shoff + idx * sizeof sectHdr, SEEK_SET))
      error("Can't read file\n");
    if (fread(&sectHdr, 1, sizeof sectHdr, ElfFile) != sizeof sectHdr)
      error("Can't read file\n");
    if (sectHdr.sh_type == 0)
      memset(&sectHdr, 0, sizeof sectHdr);

    if (sectHdr.sh_name)
      name = SectNames + sectHdr.sh_name;

    unsupported |=
      (!strcmp(name, ".dynsym") ||
       !strcmp(name, ".dynstr") ||
       !strcmp(name, ".dynamic") ||
       !strcmp(name, ".hash") ||
       !strcmp(name, ".got") ||
       !strcmp(name, ".plt") ||
       sectHdr.sh_type == 5 || // SHT_HASH
       sectHdr.sh_type == 6 || // SHT_DYNAMIC
       sectHdr.sh_type == 11); // SHT_DYNSYM

    // Keep only allocatable sections of non-zero size
    if ((sectHdr.sh_flags & 2) && sectHdr.sh_size) // SHF_ALLOC and size > 0
    {
      Sections[SectionCnt].FileOffs = 0;
      Sections[SectionCnt].Name = name;
      Sections[SectionCnt].Addr = sectHdr.sh_addr;
      Sections[SectionCnt].Size = sectHdr.sh_size;
      Sections[SectionCnt].Flags = (sectHdr.sh_flags & 1) | ((sectHdr.sh_flags & 4) >> 1); // bit0=Writable,bit1=eXecutable

      if (sectHdr.sh_type == 1) // SHT_PROGBITS
      {
        Sections[SectionCnt].FileOffs = sectHdr.sh_offset;
      }

      SectionCnt++;
    }
  }

  EntryPointAddr = elfHdr.e_entry;

  // Sort sections by address as we'll need them in order
  // and without gaps inbetween
  qsort(Sections, SectionCnt, sizeof Sections[0], &SectAddrCompare);

  if (verbose)
  {
    printf(" # XAW   VirtAddr   FileOffs       Size Name\n");
    for (idx = 0; idx < SectionCnt; idx++)
    {
      tSection* p = &Sections[idx];
      printf("%2u %c%c%c 0x%08lX 0x%08lX %10lu %s\n",
             idx,
             "-X"[(p->Flags / 2) & 1],
             "-A"[1],
             "-W"[(p->Flags / 1) & 1],
             (ulong)p->Addr,
             (ulong)p->FileOffs,
             (ulong)p->Size,
             p->Name);
    }
    printf("Entry: 0x%08lX\n", (ulong)EntryPointAddr);
    puts("");
  }

  if (unsupported)
    error("Dynamically linked or unsupported type of executable\n");

  // Write an empty a.out header at first, it will be updated later
  memset(&aoutHdr, 0, sizeof aoutHdr);
  if (fwrite(&aoutHdr, 1, sizeof aoutHdr, AoutFile) != sizeof aoutHdr)
    error("Can't write file\n");

#define USER_DATA_START 0x7F008000
#define MAXMEM (96*1024)
#define USER_DATA_END (USER_DATA_START + MAXMEM)

  if (verbose)
    printf("Phase 1: Processing sections in the range 0x%08lX ... 0x%08lX ...\n",
           (ulong)USER_DATA_START, (ulong)USER_DATA_END - 1);

  addr = USER_DATA_START;

  // Copy non-cached sections
  for (idx = 0; idx < SectionCnt; idx++)
  {
    tSection* p = &Sections[idx];

    if (p->Addr + p->Size >= p->Addr &&
        p->Addr >= USER_DATA_START &&
        p->Addr + p->Size <= USER_DATA_END)
    {
      if (verbose)
        printf("Copying %s ...\n", p->Name);

      if (idx && (Sections[idx - 1].Flags2 & 1))
      {
        if (p->Addr < Sections[idx - 1].Addr + Sections[idx - 1].Size)
          error("Sections must not intersect in memory\n");
        if ((p->Flags & 2) && !(Sections[idx - 1].Flags & 2)) // executable after non-executable
          error("Code sections must precede data sections in memory\n");
      }
      if ((p->Flags & 2) && !p->FileOffs) // executable and initialized to all zeroes
        error("Code sections must not be initialized to all zeroes\n");

      // If this section has code/data in it, if it's not initialized to all zeroes...
      if (p->FileOffs)
      {
        // Fill inter-section gaps (and .bss-like sections that aren't at the end)
        // with zeroes. This lets me order sections more flexibly and yet make
        // sure they all are properly initialized.
        if (addr < p->Addr)
        {
          WriteZeroes(AoutFile, p->Addr - addr);
          addr = p->Addr;
        }

        // Copy section
        if (fseek(ElfFile, p->FileOffs, SEEK_SET))
          error("Can't read file\n");
        CopyFileData(AoutFile, ElfFile, p->Size);
        addr += p->Size;
      }

      p->Flags2 |= 1; // section has been processed

      endAddr = p->Addr + p->Size;
    }
    else
    {
      if (verbose)
        printf("Skipping %s ...\n", p->Name);
    }
  }

  if (endAddr == 0)
    error("There are no copiable sections in this range\n");

  if (verbose)
    printf("Phase 2: Processing sections outside the range 0x%08lX ... 0x%08lX ...\n",
           (ulong)USER_DATA_START, (ulong)USER_DATA_END - 1);

  // Append cached section(s)
  for (idx = 0; idx < SectionCnt; idx++)
  {
    tSection* p = &Sections[idx];

    if (p->Addr + p->Size >= p->Addr &&
        (p->Addr >= USER_DATA_END ||
         p->Addr + p->Size <= USER_DATA_START))
    {
      if (verbose)
        printf("Copying %s ...\n", p->Name);

      // Copy section
      if (p->FileOffs)
      {
        if (fseek(ElfFile, p->FileOffs, SEEK_SET))
          error("Can't read file\n");
        CopyFileData(AoutFile, ElfFile, p->Size);
      }

      p->Flags2 |= 1; // section has been processed
    }
  }

  // Make sure no section has been left unprocessed
  for (idx = 0; idx < SectionCnt; idx++)
  {
    tSection* p = &Sections[idx];
    if (!(p->Flags2 & 1))
      error("Not all sections have been processed, e.g. %s hasn't\n", p->Name);
  }

  // Update a.out header
  aoutHdr.a_magic = OMAGIC;
  aoutHdr.a_entry = EntryPointAddr;

  codeEndAddr = endAddr;

  for (idx = SectionCnt - 1; idx != (uint)-1; idx--)
  {
    tSection* p = &Sections[idx];

    if (p->Addr + p->Size >= p->Addr &&
        p->Addr >= USER_DATA_START &&
        p->Addr + p->Size <= USER_DATA_END)
    {
      // While not executable, keep going, executable sections are first
      if (!(p->Flags & 2))
        codeEndAddr = p->Addr;
      else
        break;
    }
  }

  bssStartAddr = endAddr;

  for (idx = SectionCnt - 1; idx != (uint)-1; idx--)
  {
    tSection* p = &Sections[idx];

    if (p->Addr + p->Size >= p->Addr &&
        p->Addr >= USER_DATA_START &&
        p->Addr + p->Size <= USER_DATA_END)
    {
      // While initialized to all zeroes, keep going
      if (!p->FileOffs)
        bssStartAddr = p->Addr;
      else
        break;
    }
  }

  aoutHdr.a_text = codeEndAddr - USER_DATA_START;
  aoutHdr.a_data = bssStartAddr - codeEndAddr;
  aoutHdr.a_bss = endAddr - bssStartAddr;

  if (fseek(AoutFile, 0, SEEK_SET))
    error("Can't write file\n");
  if (fwrite(&aoutHdr, 1, sizeof aoutHdr, AoutFile) != sizeof aoutHdr)
    error("Can't write file\n");

  if (fclose(AoutFile))
    error("Can't write file\n");
  fclose(ElfFile);

  if (verbose)
  {
    printf("a.out header:\n"
           "  text size: %lu\n"
           "  data size: %lu\n"
           "  bss  size: %lu\n",
           (ulong)aoutHdr.a_text,
           (ulong)aoutHdr.a_data,
           (ulong)aoutHdr.a_bss);
    printf("Done\n");
  }

  return 0;
}
