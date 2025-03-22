PARAM = -DOLIMEX
PARAM += -DPIC32MX7
PARAM += -DUARTUSB_ENABLED
PARAM += -DUART1_ENABLED
PARAM += -DUART2_ENABLED
PARAM += -DSPI2_ENABLED
PARAM += -DSD_ENABLED
PARAM += -DGPIO1_ENABLED
PARAM += -DGPIO2_ENABLED
PARAM += -DGPIO3_ENABLED
PARAM += -DGPIO4_ENABLED
PARAM += -DGPIO5_ENABLED
PARAM += -DGPIO6_ENABLED
PARAM += -DADC_ENABLED
PARAM += -DPWM_ENABLED
PARAM += -DLED_KERNEL_PORT=TRISD -DLED_KERNEL_PIN=1 -DLED_KERNEL_INVERT
PARAM += -DUSB_NUM_STRING_DESCRIPTORS=3
PARAM += -DUSB_MAX_EP_NUMBER=3
PARAM += -DCONS_MAJOR=UARTUSB_MAJOR
PARAM += -DBUS_DIV=1
PARAM += -DBUS_KHZ=80000
PARAM += -DCPU_KHZ=80000
LDSCRIPT = "olimex/bootloader.ld"
#
# Makefile for RetroBSD, pic32 target
#
# DEBUG is set to -g by kconfig if debugging is requested (kconfig -g).
#
include ../../../clang-config.mk

AS              = $(LLVMBIN)clang -target mipsel -mcpu=mips32r2 ${DEBUG}
CC              =  -mabi=o32 -mfloat-abi=soft \
                  -nostdinc -fshort-double -fomit-frame-pointer -finline-hint-functions -I$(TOPSRC)/include
CC              = $(LLVMBIN)clang -target mipsel -mcpu=mips32r2 -mabi=o32 -mfloat-abi=soft \
                  ${DEBUG} -nostdinc -fno-builtin -Werror -Wall
CPP             = $(LLVMBIN)clang-cpp
LD              = $(LLVMBIN)ld.lld -m elf32ltsmip
LDFLAGS         = -nostdlib -T ../${LDSCRIPT}
SIZE            = $(LLVMBIN)llvm-size
OBJCOPY         = $(LLVMBIN)llvm-objcopy
OBJDUMP         = $(LLVMBIN)llvm-objdump
NM              = $(LLVMBIN)llvm-nm

# sources are located via $S relative to the compilation directory
S               = ../..

DEPFLAGS	= -MT $@ -MP -MD -MF .deps/$*.dep
DEFS            = -I. ${PARAM} -DKERNEL $(DEPFLAGS)
CFLAGS          = -O ${DEFS}

# compile rules: rules are named COMPILE_${SUFFIX}
# SUFFIX is the file suffix, capitalized (e.g. C for a .c file).

COMPILE_C       = ${CC} -c ${CFLAGS} $<
COMPILE_S       = ${CC} -c ${DEFS} $<

OBJS = exec_aout.o exec_conf.o exec_elf.o exec_script.o exec_subr.o \
	init_main.o init_sysent.o kern_clock.o kern_descrip.o \
	kern_exec.o kern_exit.o kern_fork.o kern_mman.o kern_proc.o \
	kern_prot.o kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o \
	kern_subr.o kern_synch.o kern_sysctl.o kern_time.o subr_prf.o \
	subr_rmap.o sys_generic.o sys_inode.o sys_pipe.o sys_process.o \
	syscalls.o tty.o tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o \
	ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o ufs_mount.o \
	ufs_namei.o ufs_subr.o ufs_syscalls.o ufs_syscalls2.o \
	vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o clock.o cons.o devsw.o \
	exception.o machdep.o mem.o signal.o swap.o sysctl.o adc.o \
	gpio.o pwm.o sd.o spi.o spi_bus.o uart.o usb_device.o \
	usb_function_cdc.o usb_uart.o 

CFILES = $S/kernel/exec_aout.c $S/kernel/exec_conf.c $S/kernel/exec_elf.c \
	$S/kernel/exec_script.c $S/kernel/exec_subr.c \
	$S/kernel/init_main.c $S/kernel/init_sysent.c \
	$S/kernel/kern_clock.c $S/kernel/kern_descrip.c \
	$S/kernel/kern_exec.c $S/kernel/kern_exit.c \
	$S/kernel/kern_fork.c $S/kernel/kern_mman.c \
	$S/kernel/kern_proc.c $S/kernel/kern_prot.c \
	$S/kernel/kern_prot2.c $S/kernel/kern_resource.c \
	$S/kernel/kern_sig.c $S/kernel/kern_sig2.c $S/kernel/kern_subr.c \
	$S/kernel/kern_synch.c $S/kernel/kern_sysctl.c \
	$S/kernel/kern_time.c $S/kernel/subr_prf.c $S/kernel/subr_rmap.c \
	$S/kernel/sys_generic.c $S/kernel/sys_inode.c \
	$S/kernel/sys_pipe.c $S/kernel/sys_process.c \
	$S/kernel/syscalls.c $S/kernel/tty.c $S/kernel/tty_subr.c \
	$S/kernel/tty_tty.c $S/kernel/ufs_alloc.c $S/kernel/ufs_bio.c \
	$S/kernel/ufs_bmap.c $S/kernel/ufs_dsort.c $S/kernel/ufs_fio.c \
	$S/kernel/ufs_inode.c $S/kernel/ufs_mount.c \
	$S/kernel/ufs_namei.c $S/kernel/ufs_subr.c \
	$S/kernel/ufs_syscalls.c $S/kernel/ufs_syscalls2.c \
	$S/kernel/vfs_vnops.c $S/kernel/vm_sched.c $S/kernel/vm_swap.c \
	$S/kernel/vm_swp.c $S/pic32/clock.c $S/pic32/cons.c \
	$S/pic32/devsw.c $S/pic32/exception.c $S/pic32/machdep.c \
	$S/pic32/mem.c $S/pic32/signal.c $S/pic32/swap.c \
	$S/pic32/sysctl.c $S/pic32/adc.c $S/pic32/gpio.c $S/pic32/pwm.c \
	$S/pic32/sd.c $S/pic32/spi.c $S/pic32/spi_bus.c $S/pic32/uart.c \
	$S/pic32/usb_device.c $S/pic32/usb_function_cdc.c \
	$S/pic32/usb_uart.c swapunix.c 

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ      = startup.o ${OBJS} ioconf.o
ifeq (devcfg.c,$(wildcard devcfg.c))
    SYSTEM_OBJ  += devcfg.o
endif
SYSTEM_DEP      = Makefile ioconf.c | machine sys .deps
SYSTEM_LD_HEAD  = sh ../newvers.sh > vers.c; ${CC} $(CFLAGS) -c vers.c; rm -f $@
SYSTEM_LD       = -@echo ${LD} ${LDFLAGS} '$${SYSTEM_OBJ}' vers.o -o $@; \
                  ${LD} ${LDFLAGS} ${SYSTEM_OBJ} vers.o -o $@
SYSTEM_LD_TAIL  = ${SIZE} $@; \
                  $(OBJCOPY) -O ihex $@ $(basename $@).hex; \
                  $(OBJCOPY) -O binary -R .boot -R .config $@ $(basename $@).bin; \
                  $(OBJCOPY) -O binary --only-section .boot $@ boot.bin; \
                  $(OBJDUMP) -d -S $@ > $(basename $@).dis; \
                  $(NM) -n $@ > $(basename $@).nm

unix: unix.elf

unix.elf: ${SYSTEM_DEP} ${SYSTEM_OBJ} swapunix.o
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} swapunix.o
	${SYSTEM_LD_TAIL}

swapunix.o: swapunix.c ${SYSTEM_DEP}
	${COMPILE_C}

all: unix

clean:
	rm -rf *.elf *.o *.nm *.dis *.bin

clean-all: clean
	rm -f *.h *.hex ioconf.c swap*.c vers.c machine sys .deps

.PHONY: reconfig

reconfig: ioconf.c

swapunix.c: Config ../../../tools/kconfig/kconfig | sys machine
	../../../tools/kconfig/kconfig Config
	$(MAKE) clean
	rm -f *.hex

ioconf.c: Config ../../../tools/kconfig/kconfig | sys machine
	../../../tools/kconfig/kconfig Config
	$(MAKE) clean
	rm -f *.hex

load:   unix.elf
	pic32prog unix.hex

machine:
	ln -s .. $@

sys:
	ln -s ../../include $@

.deps:
	mkdir .deps

startup.o: ../startup.S ${SYSTEM_DEP}
	${COMPILE_S}

ioconf.o: ioconf.c  ${SYSTEM_DEP}
	${COMPILE_C}

devcfg.o: devcfg.c ${SYSTEM_DEP}

exec_aout.o: $S/kernel/exec_aout.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_conf.o: $S/kernel/exec_conf.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_elf.o: $S/kernel/exec_elf.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_script.o: $S/kernel/exec_script.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_subr.o: $S/kernel/exec_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

init_main.o: $S/kernel/init_main.c ${SYSTEM_DEP}
	${COMPILE_C}

init_sysent.o: $S/kernel/init_sysent.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_clock.o: $S/kernel/kern_clock.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_descrip.o: $S/kernel/kern_descrip.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_exec.o: $S/kernel/kern_exec.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_exit.o: $S/kernel/kern_exit.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_fork.o: $S/kernel/kern_fork.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_mman.o: $S/kernel/kern_mman.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_proc.o: $S/kernel/kern_proc.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_prot.o: $S/kernel/kern_prot.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_prot2.o: $S/kernel/kern_prot2.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_resource.o: $S/kernel/kern_resource.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_sig.o: $S/kernel/kern_sig.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_sig2.o: $S/kernel/kern_sig2.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_subr.o: $S/kernel/kern_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_synch.o: $S/kernel/kern_synch.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_sysctl.o: $S/kernel/kern_sysctl.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_time.o: $S/kernel/kern_time.c ${SYSTEM_DEP}
	${COMPILE_C}

subr_prf.o: $S/kernel/subr_prf.c ${SYSTEM_DEP}
	${COMPILE_C}

subr_rmap.o: $S/kernel/subr_rmap.c ${SYSTEM_DEP}
	${COMPILE_C}

sys_generic.o: $S/kernel/sys_generic.c ${SYSTEM_DEP}
	${COMPILE_C}

sys_inode.o: $S/kernel/sys_inode.c ${SYSTEM_DEP}
	${COMPILE_C}

sys_pipe.o: $S/kernel/sys_pipe.c ${SYSTEM_DEP}
	${COMPILE_C}

sys_process.o: $S/kernel/sys_process.c ${SYSTEM_DEP}
	${COMPILE_C}

syscalls.o: $S/kernel/syscalls.c ${SYSTEM_DEP}
	${COMPILE_C}

tty.o: $S/kernel/tty.c ${SYSTEM_DEP}
	${COMPILE_C}

tty_subr.o: $S/kernel/tty_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

tty_tty.o: $S/kernel/tty_tty.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_alloc.o: $S/kernel/ufs_alloc.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_bio.o: $S/kernel/ufs_bio.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_bmap.o: $S/kernel/ufs_bmap.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_dsort.o: $S/kernel/ufs_dsort.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_fio.o: $S/kernel/ufs_fio.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_inode.o: $S/kernel/ufs_inode.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_mount.o: $S/kernel/ufs_mount.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_namei.o: $S/kernel/ufs_namei.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_subr.o: $S/kernel/ufs_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_syscalls.o: $S/kernel/ufs_syscalls.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_syscalls2.o: $S/kernel/ufs_syscalls2.c ${SYSTEM_DEP}
	${COMPILE_C}

vfs_vnops.o: $S/kernel/vfs_vnops.c ${SYSTEM_DEP}
	${COMPILE_C}

vm_sched.o: $S/kernel/vm_sched.c ${SYSTEM_DEP}
	${COMPILE_C}

vm_swap.o: $S/kernel/vm_swap.c ${SYSTEM_DEP}
	${COMPILE_C}

vm_swp.o: $S/kernel/vm_swp.c ${SYSTEM_DEP}
	${COMPILE_C}

clock.o: $S/pic32/clock.c ${SYSTEM_DEP}
	${COMPILE_C}

cons.o: $S/pic32/cons.c ${SYSTEM_DEP}
	${COMPILE_C}

devsw.o: $S/pic32/devsw.c ${SYSTEM_DEP}
	${COMPILE_C}

exception.o: $S/pic32/exception.c ${SYSTEM_DEP}
	${COMPILE_C}

machdep.o: $S/pic32/machdep.c ${SYSTEM_DEP}
	${COMPILE_C}

mem.o: $S/pic32/mem.c ${SYSTEM_DEP}
	${COMPILE_C}

signal.o: $S/pic32/signal.c ${SYSTEM_DEP}
	${COMPILE_C}

swap.o: $S/pic32/swap.c ${SYSTEM_DEP}
	${COMPILE_C}

sysctl.o: $S/pic32/sysctl.c ${SYSTEM_DEP}
	${COMPILE_C}

adc.o: $S/pic32/adc.c ${SYSTEM_DEP}
	${COMPILE_C}

gpio.o: $S/pic32/gpio.c ${SYSTEM_DEP}
	${COMPILE_C}

pwm.o: $S/pic32/pwm.c ${SYSTEM_DEP}
	${COMPILE_C}

sd.o: $S/pic32/sd.c ${SYSTEM_DEP}
	${COMPILE_C}

spi.o: $S/pic32/spi.c ${SYSTEM_DEP}
	${COMPILE_C}

spi_bus.o: $S/pic32/spi_bus.c ${SYSTEM_DEP}
	${COMPILE_C}

uart.o: $S/pic32/uart.c ${SYSTEM_DEP}
	${COMPILE_C}

usb_device.o: $S/pic32/usb_device.c ${SYSTEM_DEP}
	${COMPILE_C}

usb_function_cdc.o: $S/pic32/usb_function_cdc.c ${SYSTEM_DEP}
	${COMPILE_C}

usb_uart.o: $S/pic32/usb_uart.c ${SYSTEM_DEP}
	${COMPILE_C}


-include .deps/*.dep
