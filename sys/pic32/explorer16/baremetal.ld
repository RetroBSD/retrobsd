/*
 * Linker script for PIC32 firmware on bare metal.
 * Flash area for user starts at 9d000000.
 * Start address for user program is bfc00000.
 */
OUTPUT_ARCH(mips)
ENTRY(_reset_vector_)
MEMORY
{
  ram                  (rw!x): ORIGIN = 0x80000000, LENGTH = 26K
  u0area               (rw!x): ORIGIN = 0x80006800, LENGTH = 3K
  uarea                (rw!x): ORIGIN = 0x80007400, LENGTH = 3K
  devcfg               (r)   : ORIGIN = 0x9fc02ff0, LENGTH = 16

  /* Required by Microchip C32 linker */
  kseg0_program_mem    (rx)  : ORIGIN = 0x9D000000, LENGTH = 512K
  exception_mem        (rx)  : ORIGIN = 0x9D000000, LENGTH = 0x1000
  kseg0_boot_mem       (rx)  : ORIGIN = 0x9FC00000, LENGTH = 12K-16
  kseg1_boot_mem       (rx)  : ORIGIN = 0xBFC00000, LENGTH = 0x490
  kseg1_data_mem       (w!x) : ORIGIN = 0xA0000000, LENGTH = 0x20000
}

/* higher address of the user mode stack */
u0	= ORIGIN(u0area);
u	= ORIGIN(uarea);
u_end	= ORIGIN(uarea) + LENGTH(uarea);

SECTIONS
{
  /* Device configuration.  */
  .config         :
  {
    *(.config3)
    *(.config2)
    *(.config1)
    *(.config0)
  } > devcfg

  .boot           :
  {
    /* Execution starts here. */
    *(.startup)
  } > kseg0_boot_mem

  .text           :
  {
    /* Exception handlers. */
    *(.exception)
    *(.text .stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.glue_7t) *(.glue_7)
    __rodata_start = . ;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.dinit)
    /* Align here to ensure that the .text section ends on word boundary. */
    . = ALIGN (16);
    _etext = .;
  } > kseg0_program_mem

  /* Start data (internal SRAM).  */
  .data		  : AT (ADDR (.text) + SIZEOF (.text))
  {
    __data_start = . ;
    _gp = .; 	/* We have only 32k RAM on MC-24, so no need for 0x8000 offset. */
    *(.data .data.* .gnu.linkonce.d.*)
    /* We want the small data sections together, so single-instruction offsets
       can access them all, and initialized data all before uninitialized, so
       we can shorten the on-disk segment size.  */
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    *(.eh_frame)
    . = ALIGN (16);
    _edata = .;
  } > ram

  .bss ADDR (.data) + SIZEOF (.data) (NOLOAD) :
  {
   __bss_start = .;
   *(.dynbss)
   *(.sbss .sbss.*)
   *(.scommon)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   /* Align here to ensure that the .bss section occupies space up to
      _end.  Align after .bss to ensure correct alignment even if the
      .bss section disappears because there are no input sections.  */
   . = ALIGN (16);
  } > ram
  __bss_end = . ;
  _end = .;

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.got.plt) *(.got) }
}
