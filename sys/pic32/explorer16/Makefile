PARAM = -DEXPLORER16
PARAM += -DPIC32MX7
PARAM += -DUART2_ENABLED
PARAM += -DSPI1_ENABLED
PARAM += -DSD_ENABLED
PARAM += -DGPIO_ENABLED
PARAM += -DGPIO1_ENABLED
PARAM += -DGPIO2_ENABLED
PARAM += -DGPIO3_ENABLED
PARAM += -DGPIO4_ENABLED
PARAM += -DGPIO5_ENABLED
PARAM += -DGPIO6_ENABLED
PARAM += -DADC_ENABLED
PARAM += -DPWM_ENABLED
PARAM += -DLED_SWAP_PORT=TRISA -DLED_SWAP_PIN=3
PARAM += -DLED_TTY_PORT=TRISA -DLED_TTY_PIN=2
PARAM += -DLED_KERNEL_PORT=TRISA -DLED_KERNEL_PIN=1
PARAM += -DLED_DISK_PORT=TRISA -DLED_DISK_PIN=0
PARAM += -DSD_MHZ=10
PARAM += -DCONS_MINOR=1
PARAM += -DCONS_MAJOR=UART_MAJOR
PARAM += -DBUS_DIV=1
PARAM += -DBUS_KHZ=80000
PARAM += -DCPU_KHZ=80000
LDSCRIPT = "baremetal/script.ld"
#
# Makefile for RetroBSD, pic32 target
#
# DEBUG is set to -g by kconfig if debugging is requested (kconfig -g).
#
include ../gcc-config.mk

AS              = ${MIPS_GCC_PREFIX}as ${DEBUG} -mips32r2 -EL
CC              = ${MIPS_GCC_PREFIX}gcc ${DEBUG} -mips32r2 -EL -nostdinc -fno-builtin -Werror -Wall
CPP             = ${MIPS_GCC_PREFIX}cpp
LD              = ${MIPS_GCC_PREFIX}gcc -mips32r2 -EL
LDFLAGS         = -nostdlib -T ../${LDSCRIPT} -Wl,-Map=$(basename $@).map
SIZE            = ${MIPS_GCC_PREFIX}size
OBJCOPY         = ${MIPS_GCC_PREFIX}objcopy
OBJDUMP         = ${MIPS_GCC_PREFIX}objdump

ifneq (${MIPS_GCC_FORMAT},)
    LDFLAGS     += -Wl,--oformat=${MIPS_GCC_FORMAT}
endif

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
	gpio.o pwm.o sd.o spi.o spi_bus.o uart.o 

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
	swapunix.c 

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ      = startup.o ${OBJS} ioconf.o
ifeq (devcfg.c,$(wildcard devcfg.c))
    SYSTEM_OBJ  += devcfg.o
endif
SYSTEM_DEP      = Makefile ioconf.c machine sys .deps ${SYSTEM_OBJ}
SYSTEM_LD_HEAD  = sh ../newvers.sh > vers.c; ${CC} $(CFLAGS) -c vers.c; rm -f $@
SYSTEM_LD       = -@echo ${LD} ${LDFLAGS} '$${SYSTEM_OBJ}' vers.o -o $@; \
                  ${LD} ${LDFLAGS} ${SYSTEM_OBJ} vers.o -o $@
SYSTEM_LD_TAIL  = ${SIZE} $@; \
                  $(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ $(basename $@).hex; \
                  $(OBJCOPY) -O binary -R .boot -R .config $@ $(basename $@).bin; \
                  $(OBJDUMP) -d -S $@ > $(basename $@).dis

unix: unix.elf

unix.elf: ${SYSTEM_DEP} swapunix.o
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} swapunix.o
	${SYSTEM_LD_TAIL}

swapunix.o: swapunix.c
	${COMPILE_C}

all: unix

clean:
	rm -rf .deps *.elf *.o *.map *.dis *.bin machine sys

clean-all: clean
	rm -f *.h *.hex ioconf.c swap*.c vers.c

reconfig ioconf.c: Config ../../../tools/kconfig/kconfig
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

startup.o: ../startup.S
	${COMPILE_S}

ioconf.o: ioconf.c
	${COMPILE_C}

exec_aout.o: $S/kernel/exec_aout.c
	${COMPILE_C}

exec_conf.o: $S/kernel/exec_conf.c
	${COMPILE_C}

exec_elf.o: $S/kernel/exec_elf.c
	${COMPILE_C}

exec_script.o: $S/kernel/exec_script.c
	${COMPILE_C}

exec_subr.o: $S/kernel/exec_subr.c
	${COMPILE_C}

init_main.o: $S/kernel/init_main.c
	${COMPILE_C}

init_sysent.o: $S/kernel/init_sysent.c
	${COMPILE_C}

kern_clock.o: $S/kernel/kern_clock.c
	${COMPILE_C}

kern_descrip.o: $S/kernel/kern_descrip.c
	${COMPILE_C}

kern_exec.o: $S/kernel/kern_exec.c
	${COMPILE_C}

kern_exit.o: $S/kernel/kern_exit.c
	${COMPILE_C}

kern_fork.o: $S/kernel/kern_fork.c
	${COMPILE_C}

kern_mman.o: $S/kernel/kern_mman.c
	${COMPILE_C}

kern_proc.o: $S/kernel/kern_proc.c
	${COMPILE_C}

kern_prot.o: $S/kernel/kern_prot.c
	${COMPILE_C}

kern_prot2.o: $S/kernel/kern_prot2.c
	${COMPILE_C}

kern_resource.o: $S/kernel/kern_resource.c
	${COMPILE_C}

kern_sig.o: $S/kernel/kern_sig.c
	${COMPILE_C}

kern_sig2.o: $S/kernel/kern_sig2.c
	${COMPILE_C}

kern_subr.o: $S/kernel/kern_subr.c
	${COMPILE_C}

kern_synch.o: $S/kernel/kern_synch.c
	${COMPILE_C}

kern_sysctl.o: $S/kernel/kern_sysctl.c
	${COMPILE_C}

kern_time.o: $S/kernel/kern_time.c
	${COMPILE_C}

subr_prf.o: $S/kernel/subr_prf.c
	${COMPILE_C}

subr_rmap.o: $S/kernel/subr_rmap.c
	${COMPILE_C}

sys_generic.o: $S/kernel/sys_generic.c
	${COMPILE_C}

sys_inode.o: $S/kernel/sys_inode.c
	${COMPILE_C}

sys_pipe.o: $S/kernel/sys_pipe.c
	${COMPILE_C}

sys_process.o: $S/kernel/sys_process.c
	${COMPILE_C}

syscalls.o: $S/kernel/syscalls.c
	${COMPILE_C}

tty.o: $S/kernel/tty.c
	${COMPILE_C}

tty_subr.o: $S/kernel/tty_subr.c
	${COMPILE_C}

tty_tty.o: $S/kernel/tty_tty.c
	${COMPILE_C}

ufs_alloc.o: $S/kernel/ufs_alloc.c
	${COMPILE_C}

ufs_bio.o: $S/kernel/ufs_bio.c
	${COMPILE_C}

ufs_bmap.o: $S/kernel/ufs_bmap.c
	${COMPILE_C}

ufs_dsort.o: $S/kernel/ufs_dsort.c
	${COMPILE_C}

ufs_fio.o: $S/kernel/ufs_fio.c
	${COMPILE_C}

ufs_inode.o: $S/kernel/ufs_inode.c
	${COMPILE_C}

ufs_mount.o: $S/kernel/ufs_mount.c
	${COMPILE_C}

ufs_namei.o: $S/kernel/ufs_namei.c
	${COMPILE_C}

ufs_subr.o: $S/kernel/ufs_subr.c
	${COMPILE_C}

ufs_syscalls.o: $S/kernel/ufs_syscalls.c
	${COMPILE_C}

ufs_syscalls2.o: $S/kernel/ufs_syscalls2.c
	${COMPILE_C}

vfs_vnops.o: $S/kernel/vfs_vnops.c
	${COMPILE_C}

vm_sched.o: $S/kernel/vm_sched.c
	${COMPILE_C}

vm_swap.o: $S/kernel/vm_swap.c
	${COMPILE_C}

vm_swp.o: $S/kernel/vm_swp.c
	${COMPILE_C}

clock.o: $S/pic32/clock.c
	${COMPILE_C}

cons.o: $S/pic32/cons.c
	${COMPILE_C}

devsw.o: $S/pic32/devsw.c
	${COMPILE_C}

exception.o: $S/pic32/exception.c
	${COMPILE_C}

machdep.o: $S/pic32/machdep.c
	${COMPILE_C}

mem.o: $S/pic32/mem.c
	${COMPILE_C}

signal.o: $S/pic32/signal.c
	${COMPILE_C}

swap.o: $S/pic32/swap.c
	${COMPILE_C}

sysctl.o: $S/pic32/sysctl.c
	${COMPILE_C}

adc.o: $S/pic32/adc.c
	${COMPILE_C}

gpio.o: $S/pic32/gpio.c
	${COMPILE_C}

pwm.o: $S/pic32/pwm.c
	${COMPILE_C}

sd.o: $S/pic32/sd.c
	${COMPILE_C}

spi.o: $S/pic32/spi.c
	${COMPILE_C}

spi_bus.o: $S/pic32/spi_bus.c
	${COMPILE_C}

uart.o: $S/pic32/uart.c
	${COMPILE_C}


ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
