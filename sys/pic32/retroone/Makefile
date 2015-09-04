#
# Build RetroBSD kernel for PIC32 platform.
#
H		= ../../include
M		= ..
S		= ../../kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

# Kernel options.
DEFS		+= -I. -I$(H) -DKERNEL -DPIC32MX7
DEFS		+= -DGLOBAL_DEBUG

# CPU frequency 80 MHz.
DEFS		+= -DCPU_KHZ=80000
DEFS		+= -DBUS_KHZ=80000 -DBUS_DIV=1

#
# Basic pic32mx795f512l chip with internal oscillator
# ===================================================
#
# Console on UART1 with hardware handshaking
DEFS            += -DCONSOLE_UART1 #-DCONSOLE_RTSCTS
# SD/MMC card driver on SPI1
DEFS            += -DSD0_PORT=1 -DSD0_MHZ=20
# /CS0 at pin B1
DEFS            += -DSD0_CS_PORT=TRISD -DSD0_CS_PIN=9
# LEDs at pins A0, A1, A2, A3
DEFS            += -DLED_DISK_PORT=TRISA   -DLED_DISK_PIN=14
DEFS            += -DLED_KERNEL_PORT=TRISB -DLED_KERNEL_PIN=8
DEFS            += -DLED_TTY_PORT=TRISB    -DLED_TTY_PIN=9
DEFS            += -DLED_AUX_PORT=TRISA    -DLED_AUX_PIN=9

DEPFLAGS	= -MT $@ -MD -MP -MF .deps/$*.dep
CFLAGS		= -O $(DEFS) $(DEPFLAGS)
ASFLAGS		= $(DEFS) $(DEPFLAGS)

include ../gcc-config.mk

CC		= $(MIPS_GCC_PREFIX)gcc -EL -g -mips32r2
CC		+= -nostdinc -fno-builtin -Werror -Wall -fno-dwarf2-cfi-asm
LDFLAGS         = -nostdlib -T ../cfg/bare.ld -Wl,-Map=unix.map
SIZE		= $(MIPS_GCC_PREFIX)size
OBJDUMP		= $(MIPS_GCC_PREFIX)objdump
OBJCOPY		= $(MIPS_GCC_PREFIX)objcopy

ifneq (${MIPS_GCC_FORMAT},)
    LDFLAGS     += -Wl,--oformat=${MIPS_GCC_FORMAT}
endif

# Machine-dependent files:
# startup.o MUST be loaded first.
KERNOBJ		= startup.o clock.o devsw.o cons.o sysctl.o \
		  signal.o machdep.o mem.o exception.o

# Kernel.
KERNOBJ		+= init_main.o init_sysent.o kern_clock.o \
		   kern_descrip.o kern_exec.o kern_exit.o kern_fork.o \
		   kern_mman.o kern_proc.o kern_prot.o \
		   kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o \
		   kern_subr.o kern_synch.o kern_sysctl.o kern_time.o \
		   subr_log.o subr_prf.o subr_rmap.o \
		   sys_generic.o sys_inode.o syscalls.o \
		   sys_pipe.o sys_process.o tty.o exec_conf.o exec_subr.o \
		   exec_aout.o exec_elf.o exec_script.o \
		   tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o \
		   ufs_bmap.o ufs_dsort.o ufs_fio.o \
		   ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o \
		   ufs_syscalls.o ufs_syscalls2.o vfs_vnops.o \
		   vm_sched.o vm_swap.o vm_swp.o kern_glob.o swap.o spi_bus.o

# Drivers.
KERNOBJ		+= rd_sd.o rdisk.o

# Configuration-dependent files.
KERNOBJ		+= vers.o devcfg.o

# Include any local specific configuration overrides
-include Makefile.local

# GPIO driver
KERNOBJ         += gpio.o
DEFS            += -DGPIO_ENABLED

# ADC driver
KERNOBJ         += adc.o
DEFS            += -DADC_ENABLED

# GLCD driver
KERNOBJ         += glcd.o
DEFS            += -DGLCD_ENABLED

# PWM driver
KERNOBJ         += pwm.o
DEFS            += -DPWM_ENABLED

all:		.deps sys machine unix.elf
		$(SIZE) unix.elf

clean:
		rm -rf .deps *.o *.elf *.bin *.dis *.map *.srec core \
		mklog assym.h vers.c genassym sys machine

cleanest: clean
		rm -f unix.hex

.deps:
		mkdir .deps

sys:
		ln -s ../../include $@

machine:
		ln -s .. $@

unix.elf:	$(KERNOBJ) ../cfg/bare.ld
		$(CC) $(LDFLAGS) $(KERNOBJ) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > unix.dis
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ unix.hex

load:           unix.elf
		pic32prog unix.hex

vers.o:		../newvers.sh $(H)/*.h $(M)/*.[ch] $(S)/*.c
		sh ../newvers.sh > vers.c
		$(CC) -c vers.c

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .bin .elf

.o.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
