BUILDPATH = ../../../tools/configsys/../../sys/pic32
H         = ../../../tools/configsys/../../sys/include
M         = ../../../tools/configsys/../../sys/pic32
S         = ../../../tools/configsys/../../sys/kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

KERNOBJ += clock.o cons.o devsw.o exception.o exec_aout.o exec_conf.o exec_elf.o exec_script.o exec_subr.o init_main.o init_sysent.o kern_clock.o kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_mman.o kern_proc.o kern_prot.o kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o kern_subr.o kern_synch.o kern_sysctl.o kern_time.o machdep.o mem.o rd_sd.o rd_sdramp.o rdisk.o sdram.o signal.o spi_bus.o startup.o subr_prf.o subr_rmap.o swap.o sys_generic.o sys_inode.o sys_pipe.o sys_process.o syscalls.o sysctl.o tty.o tty_subr.o tty_tty.o uart.o ufs_alloc.o ufs_bio.o ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o ufs_syscalls.o ufs_syscalls2.o vers.o vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o 
EXTRA_TARGETS = 

DEFS += -DBUS_DIV=1
DEFS += -DBUS_KHZ='CPU_KHZ/BUS_DIV'
DEFS += -DCONSOLE_DEVICE=tty1
DEFS += -DCPU_IDIV=2
DEFS += -DCPU_KHZ='((CRYSTAL*1000)/CPU_IDIV*CPU_MUL/CPU_ODIV)'
DEFS += -DCPU_MUL=20
DEFS += -DCPU_ODIV=1
DEFS += -DCRYSTAL=8
DEFS += -DEXEC_AOUT
DEFS += -DEXEC_ELF
DEFS += -DEXEC_SCRIPT
DEFS += -DKERNEL
DEFS += -DKERNEL_EXECUTABLE_RAM
DEFS += -DLED_DISK_INVERT=YES
DEFS += -DLED_DISK_PIN=9
DEFS += -DLED_DISK_PORT=TRISB
DEFS += -DLED_KERNEL_INVERT=YES
DEFS += -DLED_KERNEL_PIN=10
DEFS += -DLED_KERNEL_PORT=TRISB
DEFS += -DLED_SWAP_INVERT=YES
DEFS += -DLED_SWAP_PIN=8
DEFS += -DLED_SWAP_PORT=TRISB
DEFS += -DLED_TTY_INVERT=YES
DEFS += -DLED_TTY_PIN=11
DEFS += -DLED_TTY_PORT=TRISB
DEFS += -DPARTITION="sdramp0:sa@2048,fs@14000"
DEFS += -DPIC32MX7
DEFS += -DSD0_CS_PIN=9
DEFS += -DSD0_CS_PORT=TRISA
DEFS += -DSD0_PORT=2
DEFS += -DSDRAMP_ENABLED=YES
DEFS += -DUART2_ENABLED=YES
DEFS += -DUCB_METER


LDSCRIPT = ../../../tools/configsys/../../sys/pic32/cfg/bootloader-sdram.ld

CONFIG = UBW32-UART-SDRAM
CONFIGPATH = ../../../tools/configsys

include ../../../tools/configsys/../../sys/pic32/kernel-post.mk
