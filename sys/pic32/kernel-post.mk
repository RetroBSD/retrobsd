
DEPFLAGS	= -MT $@ -MP -MD -MF .deps/$*.dep
CFLAGS		= -I. -I$(H) -O $(DEFS) $(DEPFLAGS)
ASFLAGS		= -I. -I$(H) $(DEFS) $(DEPFLAGS)

include $(BUILDPATH)/gcc-config.mk

CC		= $(MIPS_GCC_PREFIX)gcc -EL -g -mips32r2
CC		+= -nostdinc -fno-builtin -Werror -Wall -fno-dwarf2-cfi-asm
LDFLAGS         = -nostdlib
SIZE		= $(MIPS_GCC_PREFIX)size
OBJDUMP		= $(MIPS_GCC_PREFIX)objdump
OBJCOPY		= $(MIPS_GCC_PREFIX)objcopy

DEFS            += -DCONFIG=$(CONFIG)

ifneq (${MIPS_GCC_FORMAT},)
    LDFLAGS     += -Wl,--oformat=${MIPS_GCC_FORMAT}
endif

all:		.deps sys machine unix.elf
		$(SIZE) unix.elf

clean:
		rm -rf .deps *.o *.elf *.bin *.dis *.map *.srec core \
		mklog assym.h vers.c genassym sys machine

.deps:
		mkdir .deps

sys:
		ln -s $(BUILDPATH)/../include $@

machine:
		ln -s $(BUILDPATH) $@

unix.elf:       $(KERNOBJ) $(LDSCRIPT)
		$(CC) $(LDFLAGS) -T $(LDSCRIPT) -Wl,-Map=unix.map $(KERNOBJ) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > unix.dis
		$(OBJCOPY) -O binary -R .boot -R .config $@ unix.bin
		$(OBJCOPY) -O binary -j .boot -j .config $@ boot.bin
		test -s boot.bin || rm boot.bin
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ unix.hex
		chmod -x $@ unix.bin

load:           unix.hex
		pic32prog $(BLREBOOT) unix.hex

vers.o:		$(BUILDPATH)/newvers.sh $(H)/*.h $(M)/*.[ch] $(S)/*.c
		sh $(BUILDPATH)/newvers.sh > vers.c
		$(CC) -c vers.c

reconfig:
		../../../tools/configsys/config $(CONFIG)

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .bin .elf

.o.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
