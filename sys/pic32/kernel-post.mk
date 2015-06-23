
DEPFLAGS	= -MT $@ -MP -MD -MF .deps/$*.dep
CFLAGS		= -I. -I$(H) -O $(DEFS) $(DEPFLAGS)
ASFLAGS		= -I. -I$(H) $(DEFS) $(DEPFLAGS)

include $(BUILDPATH)/gcc-config.mk

CC		= $(GCCPREFIX)gcc -EL -g -mips32r2
CC		+= -nostdinc -fno-builtin -Werror -Wall -fno-dwarf2-cfi-asm
LDFLAGS         += -nostdlib
SIZE		= $(GCCPREFIX)size
OBJDUMP		= $(GCCPREFIX)objdump
OBJCOPY		= $(GCCPREFIX)objcopy
PROGTOOL        = $(AVRDUDE) -c stk500v2 -p pic32 -b 115200
BLCFLAGS        = -Os -I. -I$(H) $(DEFS) $(DEPFLAGS)
BLOBJS          = bl_usb_boot.o bl_usb_device.o bl_usb_function_hid.o bl_devcfg.o

DEFS            += -DCONFIG=$(CONFIG)

all:		.deps sys machine unix.elf $(EXTRA_TARGETS)
		$(SIZE) unix.elf

bootloader:     .deps sys machine bootloader.elf

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

bootloader.elf: $(BLOBJS)
		$(CC) $(LDFLAGS) -T$(BUILDPATH)/cfg/boot.ld -Wl,-Map=usbboot.map $(BLOBJS) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > bootloader.dis
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ bootloader.hex

bl_usb_boot.o:  $(BUILDPATH)/usb_boot.c
		$(CC) $(BLCFLAGS) -o $@ -c $<

bl_usb_device.o: $(BUILDPATH)/usb_device.c
		$(CC) $(BLCFLAGS) -o $@ -c $<

bl_usb_function_hid.o: $(BUILDPATH)/usb_function_hid.c
		$(CC) $(BLCFLAGS) -o $@ -c $<

bl_devcfg.o:    devcfg.c
		$(CC) $(BLCFLAGS) -o $@ -c $<

load:           unix.hex
		pic32prog $(BLREBOOT) unix.hex

loadmax:        unix.hex
		$(PROGTOOL) -U flash:w:unix.hex:i

loadboot:       bootloader.hex
		pic32prog $(BLREBOOT) bootloader.hex

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
