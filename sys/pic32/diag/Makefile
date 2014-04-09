H		= ../../include
M		= ..
S		= ../../kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

include ../gcc-config.mk

# Kernel options.
DEFS		+= -I. -I$(H) -DKERNEL -DPIC32MX7

# CPU frequency 80 MHz.
DEFS		+= -DCPU_KHZ=80000
DEFS		+= -DBUS_KHZ=80000

#
# Ethernet/USB Starter Kit with I/O Expansion board
# =================================================
#
#LDSCRIPT        = ../strater-kit/using-bootloader.ld

# Console on USB
# For Windows, use the driver: http://www.schmalzhaus.com/UBW32/FW/UBW32inf.zip
DEFS            += -DCONSOLE_USB
DEFS  		+= -DUSB_NUM_STRING_DESCRIPTORS=3 -DUSB_MAX_EP_NUMBER=3

# SD/MMC card driver on SPI1
# /CS0 at pin B1
#DEFS            += -DSD0_PORT=1 -DSD0_MHZ=10
#DEFS            += -DSD0_CS_PORT=TRISB -DSD0_CS_PIN=1

# LEDs at pins D0 (red), D1 (yellow), D2 (green)
#DEFS            += -DLED_TTY_PORT=TRISD    -DLED_TTY_PIN=0
#DEFS            += -DLED_DISK_PORT=TRISD   -DLED_DISK_PIN=1
#DEFS            += -DLED_KERNEL_PORT=TRISD -DLED_KERNEL_PIN=2

#
# UBW32 board
# ===========
#
LDSCRIPT        = ../ubw32/using-bootloader.ld

# SD/MMC card driver on SPI1
# /CS0 at pin A9 (and optional /CS1 at pin A10)
DEFS            += -DSD0_PORT=1
DEFS            += -DSD0_CS_PORT=TRISA -DSD0_CS_PIN=9
DEFS            += -DSD1_PORT=1
DEFS            += -DSD1_CS_PORT=TRISA -DSD1_CS_PIN=10

# LEDs at pins E0, E1, E2, E3
DEFS            += -DLED_AUX_PORT=TRISE    -DLED_AUX_PIN=0    -DLED_AUX_INVERT
DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=1   -DLED_DISK_INVERT
DEFS            += -DLED_KERNEL_PORT=TRISE -DLED_KERNEL_PIN=2 -DLED_KERNEL_INVERT
DEFS            += -DLED_TTY_PORT=TRISE    -DLED_TTY_PIN=3    -DLED_TTY_INVERT

#
# Include or exclude drivers
#

# General Purpose I/O
DRIVER_GPIO         = no

# Basic ADC interface
DRIVER_ADC          = no

# Power control (power LED, and soft power-off by button)
# requires supported PSU (ATX)
DRIVER_POWER        = no

POWER_LED_PORT      = TRISG
POWER_LED_PIN       = 12
POWER_SWITCH_PORT   = TRISG
POWER_SWITCH_PIN    = 0
POWER_CONTROL_PORT  = TRISE
POWER_CONTROL_PIN   = 9

DEPFLAGS	= -MT $@ -MD -MP -MF .deps/$*.dep
CFLAGS		= -O $(DEFS) $(DEPFLAGS)
ASFLAGS		= $(DEFS) $(DEPFLAGS)

CC		= $(GCCPREFIX)gcc -EL -g -mips32r2
CC		+= -nostdinc -fno-builtin -Werror -Wall -fno-dwarf2-cfi-asm
LDFLAGS         += -nostdlib -T$(LDSCRIPT) -Wl,-Map=diag.map
SIZE		= $(GCCPREFIX)size
OBJDUMP		= $(GCCPREFIX)objdump
OBJCOPY		= $(GCCPREFIX)objcopy

# Machine-dependent files:
# startup.o MUST be loaded first.
KERNOBJ		= startup.o machdep.o usb_device.o usb_function_cdc.o
KERNOBJ		+= usb_console.o
#KERNOBJ	+= cons.o

# Kernel.
KERNOBJ		+= main.o subr_prf.o

# Drivers.
KERNOBJ		+= rdisk.o rd_sd.o spi_bus.o

# Include any local specific configuration overrides
-include Makefile.local

# This makefile does the work including the right files and options for the drivers
include ../drivers.mk

all:		.deps sys machine diag.elf
		$(SIZE) diag.elf

clean:
		rm -rf .deps *.o *.elf *.bin *.dis *.map *.srec core \
		mklog assym.h vers.c genassym sys machine

.deps:
		mkdir .deps

sys:
		ln -s ../../include $@

machine:
		ln -s .. $@

diag.elf:	$(KERNOBJ) $(LDSCRIPT)
		$(CC) $(LDFLAGS) $(KERNOBJ) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > diag.dis
		$(OBJCOPY) -O binary $@ diag.bin
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ diag.hex
		chmod -x $@ diag.bin

load:           diag.elf
		pic32prog diag.hex

vers.o:		../newvers.sh $(H)/*.h $(M)/*.[ch] $(S)/*.c
		sh ../newvers.sh > vers.c
		$(CC) -c vers.c

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .bin .elf

.o.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
