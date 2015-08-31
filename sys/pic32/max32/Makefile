IDENT=-DCHIPKIT_MAX -DPIC32MX7 -DPWM_ENABLED -DADC_ENABLED -DGPIO_ENABLED -DSD0_CS_PIN=14 -DSD0_CS_PORT=TRISC -DSD0_PORT=2 -DSD0_MHZ=10 -DSPI_ENABLED -DCONSOLE_DEVICE=tty0 -DUART4_ENABLED -DUART3_ENABLED -DUART2_ENABLED -DUART1_ENABLED -DLED_KERNEL_PIN=3 -DLED_KERNEL_PORT=TRISA -DUCB_METER -DEXEC_SCRIPT -DEXEC_ELF -DEXEC_AOUT -DBUS_DIV=1 -DHZ=100 -DBUS_KHZ=80000 -DCPU_KHZ=80000
LDSCRIPT="cfg/bootloader-max32.ld"
PARAM=-DTIMEZONE=480 -DDST=1 -DMAXUSERS=2
#
# Makefile for RetroBSD, pic32 target
#
# DEBUG is set to -g by kconfig if debugging is requested (kconfig -g).
# PROF is set to -pg by kconfig if profiling is requested (kconfig -p).

ifndef MIPS_GCC_ROOT
    ifeq (/usr/local/mips-2014.05,$(wildcard /usr/local/mips-2014.05))
        MIPS_GCC_ROOT = /usr/local/mips-2014.05
    endif
    ifeq (/usr/local/mips-gcc-4.8.1,$(wildcard /usr/local/mips-gcc-4.8.1))
        MIPS_GCC_ROOT = /usr/local/mips-gcc-4.8.1
    endif
    ifeq (/usr/local/pic32-tools,$(wildcard /usr/local/pic32-tools))
        MIPS_GCC_ROOT = /usr/local/pic32-tools
    endif
    ifdef UECIDE
        MIPS_GCC_ROOT = ${UECIDE}/compilers/pic32-tools
    endif
endif

ifeq (${MIPS_GCC_ROOT}/bin/mips-sde-elf-gcc,$(wildcard ${MIPS_GCC_ROOT}/bin/mips-sde-elf-gcc))
    MIPS_GCC_PREFIX ?= ${MIPS_GCC_ROOT}/bin/mips-sde-elf-
endif
ifeq (${MIPS_GCC_ROOT}/bin/mips-elf-gcc,$(wildcard ${MIPS_GCC_ROOT}/bin/mips-elf-gcc))
    MIPS_GCC_PREFIX ?= ${MIPS_GCC_ROOT}/bin/mips-elf-
endif
ifeq (${MIPS_GCC_ROOT}/bin/pic32-gcc,$(wildcard ${MIPS_GCC_ROOT}/bin/pic32-gcc))
    MIPS_GCC_PREFIX ?= ${MIPS_GCC_ROOT}/bin/pic32-
endif

AS              = ${MIPS_GCC_PREFIX}as ${DEBUG} -mips32r2 -EL
CC              = ${MIPS_GCC_PREFIX}gcc ${DEBUG} -mips32r2 -EL -nostdinc -fno-builtin -Werror -Wall
CPP             = ${MIPS_GCC_PREFIX}cpp
LD              = ${MIPS_GCC_PREFIX}gcc -mips32r2 -EL
LDFLAGS         = -nostdlib -T $S/pic32/${LDSCRIPT} -Wl,-Map=unix.map
SIZE            = ${MIPS_GCC_PREFIX}size
OBJCOPY         = ${MIPS_GCC_PREFIX}objcopy
OBJDUMP         = ${MIPS_GCC_PREFIX}objdump

ifeq (${MIPS_GCC_ROOT}/bin/mips-elf-gcc,$(wildcard ${MIPS_GCC_ROOT}/bin/mips-elf-gcc))
    LDFLAGS     += -Wl,--oformat=elf32-littlemips
endif
ifeq (${MIPS_GCC_ROOT}/bin/pic32-gcc,$(wildcard ${MIPS_GCC_ROOT}/bin/pic32-gcc))
    LDFLAGS     += -Wl,--oformat=elf32-tradlittlemips
endif

# source tree is located via $S relative to the compilation directory
S               = ../..

COPTS           = -I. ${IDENT} -DKERNEL
CFLAGS          = -O ${COPTS}

# compile rules: rules are named ${TYPE}_${SUFFIX}${CONFIG_DEP}
# where TYPE is NORMAL, DRIVER, or PROFILE}; SUFFIX is the file suffix,
# capitalized (e.g. C for a .c file), and CONFIG_DEP is _C if the file
# is marked as config-dependent.

NORMAL_C        = ${CC} -c ${CFLAGS} ${PROF} $<
NORMAL_C_C      = ${CC} -c ${CFLAGS} ${PROF} ${PARAM} $<

DRIVER_C        = ${CC} -c ${CFLAGS} ${PROF} $<
DRIVER_C_C      = ${CC} -c ${CFLAGS} ${PROF} ${PARAM} $<

PROFILE_C       = ${CC} -p -c ${COPTS} $<

NORMAL_S        = ${CC} -c ${COPTS} $<
NORMAL_S_C      = ${CC} -c ${COPTS} ${PARAM} $<

OBJS=exec_aout.o exec_conf.o exec_elf.o exec_script.o exec_subr.o \
	init_main.o init_sysent.o kern_clock.o kern_descrip.o \
	kern_exec.o kern_exit.o kern_fork.o kern_mman.o kern_proc.o \
	kern_prot.o kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o \
	kern_subr.o kern_synch.o kern_sysctl.o kern_time.o rdisk.o \
	subr_prf.o subr_rmap.o sys_generic.o sys_inode.o sys_pipe.o \
	sys_process.o syscalls.o tty.o tty_subr.o tty_tty.o ufs_alloc.o \
	ufs_bio.o ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o \
	ufs_mount.o ufs_namei.o ufs_subr.o ufs_syscalls.o \
	ufs_syscalls2.o vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o adc.o \
	clock.o cons.o devsw.o exception.o gpio.o machdep.o mem.o pwm.o \
	rd_sd.o signal.o spi.o spi_bus.o swap.o sysctl.o uart.o 

CFILES=$S/kernel/exec_aout.c $S/kernel/exec_conf.c $S/kernel/exec_elf.c \
	$S/kernel/exec_script.c $S/kernel/exec_subr.c \
	$S/kernel/init_main.c $S/kernel/init_sysent.c \
	$S/kernel/kern_clock.c $S/kernel/kern_descrip.c \
	$S/kernel/kern_exec.c $S/kernel/kern_exit.c \
	$S/kernel/kern_fork.c $S/kernel/kern_mman.c \
	$S/kernel/kern_proc.c $S/kernel/kern_prot.c \
	$S/kernel/kern_prot2.c $S/kernel/kern_resource.c \
	$S/kernel/kern_sig.c $S/kernel/kern_sig2.c $S/kernel/kern_subr.c \
	$S/kernel/kern_synch.c $S/kernel/kern_sysctl.c \
	$S/kernel/kern_time.c $S/kernel/rdisk.c $S/kernel/subr_prf.c \
	$S/kernel/subr_rmap.c $S/kernel/sys_generic.c \
	$S/kernel/sys_inode.c $S/kernel/sys_pipe.c \
	$S/kernel/sys_process.c $S/kernel/syscalls.c $S/kernel/tty.c \
	$S/kernel/tty_subr.c $S/kernel/tty_tty.c $S/kernel/ufs_alloc.c \
	$S/kernel/ufs_bio.c $S/kernel/ufs_bmap.c $S/kernel/ufs_dsort.c \
	$S/kernel/ufs_fio.c $S/kernel/ufs_inode.c $S/kernel/ufs_mount.c \
	$S/kernel/ufs_namei.c $S/kernel/ufs_subr.c \
	$S/kernel/ufs_syscalls.c $S/kernel/ufs_syscalls2.c \
	$S/kernel/vfs_vnops.c $S/kernel/vm_sched.c $S/kernel/vm_swap.c \
	$S/kernel/vm_swp.c $S/pic32/adc.c $S/pic32/clock.c \
	$S/pic32/cons.c $S/pic32/devsw.c $S/pic32/exception.c \
	$S/pic32/gpio.c $S/pic32/machdep.c $S/pic32/mem.c $S/pic32/pwm.c \
	$S/pic32/rd_sd.c $S/pic32/signal.c $S/pic32/spi.c \
	$S/pic32/spi_bus.c $S/pic32/swap.c $S/pic32/sysctl.c \
	$S/pic32/uart.c swapunix.c 

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ      = startup.o ${OBJS} #ioconf.o
ifeq (devcfg.c,$(wildcard devcfg.c))
    SYSTEM_OBJ  += devcfg.o
endif
SYSTEM_DEP      = Makefile ioconf.c swapunix.c machine sys ${SYSTEM_OBJ}
SYSTEM_LD_HEAD  = sh $S/pic32/newvers.sh > vers.c; ${CC} $(CFLAGS) -c vers.c; rm -f $@
SYSTEM_LD       = -@echo ${LD} ${LDFLAGS} '$${SYSTEM_OBJ}' vers.o -o unix.elf; \
                  ${LD} ${LDFLAGS} ${SYSTEM_OBJ} vers.o -o unix.elf
SYSTEM_LD_TAIL  = ${SIZE} unix.elf; \
                  $(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ unix.hex; \
                  $(OBJCOPY) -O binary -R .boot -R .config $@ unix.bin; \
                  $(OBJDUMP) -d -S unix.elf > unix.dis

unix.elf: ${SYSTEM_DEP} swapunix.o
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} swapunix.o
	${SYSTEM_LD_TAIL}

swapunix.o: swapunix.c
	${NORMAL_C}

all: unix.elf

clean:
	rm -f *.elf *.o *.map *.dis *.bin

clean-all: clean
	rm -f *.h *.c machine sys

reconfig ioconf.c swapunix.c: Config ../../../tools/kconfig/kconfig
	../../../tools/kconfig/kconfig Config
	$(MAKE) clean

load:   unix.elf
	pic32prog unix.hex

startup.o: $S/pic32/startup.S
	${NORMAL_S}

# the following are necessary because the files depend on the types of
# cpu's included in the system configuration
clock.o machdep.o conf.o: Makefile

machine:
	ln -s $S/pic32 $@

sys:
	ln -s $S/include $@

depend: machine
	mkdep ${COPTS} ${CFILES} ioconf.c

ioconf.o: ioconf.c
	${CC} -c ${CFLAGS} ioconf.c

exec_aout.o: $S/kernel/exec_aout.c
	${NORMAL_C}

exec_conf.o: $S/kernel/exec_conf.c
	${NORMAL_C}

exec_elf.o: $S/kernel/exec_elf.c
	${NORMAL_C}

exec_script.o: $S/kernel/exec_script.c
	${NORMAL_C}

exec_subr.o: $S/kernel/exec_subr.c
	${NORMAL_C}

init_main.o: $S/kernel/init_main.c
	${NORMAL_C}

init_sysent.o: $S/kernel/init_sysent.c
	${NORMAL_C}

kern_clock.o: $S/kernel/kern_clock.c
	${NORMAL_C}

kern_descrip.o: $S/kernel/kern_descrip.c
	${NORMAL_C}

kern_exec.o: $S/kernel/kern_exec.c
	${NORMAL_C}

kern_exit.o: $S/kernel/kern_exit.c
	${NORMAL_C}

kern_fork.o: $S/kernel/kern_fork.c
	${NORMAL_C}

kern_mman.o: $S/kernel/kern_mman.c
	${NORMAL_C}

kern_proc.o: $S/kernel/kern_proc.c
	${NORMAL_C}

kern_prot.o: $S/kernel/kern_prot.c
	${NORMAL_C}

kern_prot2.o: $S/kernel/kern_prot2.c
	${NORMAL_C}

kern_resource.o: $S/kernel/kern_resource.c
	${NORMAL_C}

kern_sig.o: $S/kernel/kern_sig.c
	${NORMAL_C}

kern_sig2.o: $S/kernel/kern_sig2.c
	${NORMAL_C}

kern_subr.o: $S/kernel/kern_subr.c
	${NORMAL_C}

kern_synch.o: $S/kernel/kern_synch.c
	${NORMAL_C}

kern_sysctl.o: $S/kernel/kern_sysctl.c
	${NORMAL_C}

kern_time.o: $S/kernel/kern_time.c
	${NORMAL_C}

rdisk.o: $S/kernel/rdisk.c
	${NORMAL_C}

subr_prf.o: $S/kernel/subr_prf.c
	${NORMAL_C}

subr_rmap.o: $S/kernel/subr_rmap.c
	${NORMAL_C}

sys_generic.o: $S/kernel/sys_generic.c
	${NORMAL_C}

sys_inode.o: $S/kernel/sys_inode.c
	${NORMAL_C}

sys_pipe.o: $S/kernel/sys_pipe.c
	${NORMAL_C}

sys_process.o: $S/kernel/sys_process.c
	${NORMAL_C}

syscalls.o: $S/kernel/syscalls.c
	${NORMAL_C}

tty.o: $S/kernel/tty.c
	${NORMAL_C}

tty_subr.o: $S/kernel/tty_subr.c
	${NORMAL_C}

tty_tty.o: $S/kernel/tty_tty.c
	${NORMAL_C}

ufs_alloc.o: $S/kernel/ufs_alloc.c
	${NORMAL_C}

ufs_bio.o: $S/kernel/ufs_bio.c
	${NORMAL_C}

ufs_bmap.o: $S/kernel/ufs_bmap.c
	${NORMAL_C}

ufs_dsort.o: $S/kernel/ufs_dsort.c
	${NORMAL_C}

ufs_fio.o: $S/kernel/ufs_fio.c
	${NORMAL_C}

ufs_inode.o: $S/kernel/ufs_inode.c
	${NORMAL_C}

ufs_mount.o: $S/kernel/ufs_mount.c
	${NORMAL_C}

ufs_namei.o: $S/kernel/ufs_namei.c
	${NORMAL_C}

ufs_subr.o: $S/kernel/ufs_subr.c
	${NORMAL_C}

ufs_syscalls.o: $S/kernel/ufs_syscalls.c
	${NORMAL_C}

ufs_syscalls2.o: $S/kernel/ufs_syscalls2.c
	${NORMAL_C}

vfs_vnops.o: $S/kernel/vfs_vnops.c
	${NORMAL_C}

vm_sched.o: $S/kernel/vm_sched.c
	${NORMAL_C}

vm_swap.o: $S/kernel/vm_swap.c
	${NORMAL_C}

vm_swp.o: $S/kernel/vm_swp.c
	${NORMAL_C}

adc.o: $S/pic32/adc.c
	${NORMAL_C}

clock.o: $S/pic32/clock.c
	${NORMAL_C}

cons.o: $S/pic32/cons.c
	${NORMAL_C}

devsw.o: $S/pic32/devsw.c
	${NORMAL_C}

exception.o: $S/pic32/exception.c
	${NORMAL_C}

gpio.o: $S/pic32/gpio.c
	${NORMAL_C}

machdep.o: $S/pic32/machdep.c
	${NORMAL_C}

mem.o: $S/pic32/mem.c
	${NORMAL_C}

pwm.o: $S/pic32/pwm.c
	${NORMAL_C}

rd_sd.o: $S/pic32/rd_sd.c
	${NORMAL_C}

signal.o: $S/pic32/signal.c
	${NORMAL_C}

spi.o: $S/pic32/spi.c
	${NORMAL_C}

spi_bus.o: $S/pic32/spi_bus.c
	${NORMAL_C}

swap.o: $S/pic32/swap.c
	${NORMAL_C}

sysctl.o: $S/pic32/sysctl.c
	${NORMAL_C}

uart.o: $S/pic32/uart.c
	${NORMAL_C}

