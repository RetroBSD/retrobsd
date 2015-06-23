BUILDPATH = ../../../tools/configsys/../../sys/pic32
H         = ../../../tools/configsys/../../sys/include
M         = ../../../tools/configsys/../../sys/pic32
S         = ../../../tools/configsys/../../sys/kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

KERNOBJ += clock.o cons.o devsw.o exception.o exec_aout.o exec_conf.o exec_elf.o exec_script.o exec_subr.o gpio.o init_main.o init_sysent.o kern_clock.o kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_mman.o kern_proc.o kern_prot.o kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o kern_subr.o kern_synch.o kern_sysctl.o kern_time.o machdep.o mem.o rd_sd.o rdisk.o signal.o spi_bus.o startup.o subr_prf.o subr_rmap.o swap.o sys_generic.o sys_inode.o sys_pipe.o sys_process.o syscalls.o sysctl.o tty.o tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o ufs_syscalls.o ufs_syscalls2.o usb_device.o usb_function_cdc.o usb_uart.o vers.o vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o 
EXTRA_TARGETS = 

DEFS += -DBUS_DIV=1
DEFS += -DBUS_KHZ=80000
DEFS += -DCONSOLE_DEVICE=ttyUSB0
DEFS += -DCPU_IDIV=2
DEFS += -DCPU_KHZ=80000
DEFS += -DCPU_MUL=20
DEFS += -DCPU_ODIV=1
DEFS += -DCRYSTAL=8
DEFS += -DEXEC_AOUT
DEFS += -DEXEC_ELF
DEFS += -DEXEC_SCRIPT
DEFS += -DGPIO_ENABLED=YES
DEFS += -DKERNEL
DEFS += -DLED_DISK_PIN=1
DEFS += -DLED_DISK_PORT=TRISD
DEFS += -DLED_KERNEL_PIN=2
DEFS += -DLED_KERNEL_PORT=TRISD
DEFS += -DLED_TTY_PIN=0
DEFS += -DLED_TTY_PORT=TRISD
DEFS += -DPIC32MX7
DEFS += -DSD0_CS_PIN=9
DEFS += -DSD0_CS_PORT=TRISA
DEFS += -DSD0_PORT=1
DEFS += -DUARTUSB_ENABLED=YES
DEFS += -DUCB_METER
DEFS += -DUSB_MAX_EP_NUMBER=3
DEFS += -DUSB_NUM_STRING_DESCRIPTORS=3


LDSCRIPT = ../../../tools/configsys/../../sys/pic32/cfg/bootloader.ld

CONFIG = MEB
CONFIGPATH = ../../../tools/configsys

include ../../../tools/configsys/../../sys/pic32/kernel-post.mk
