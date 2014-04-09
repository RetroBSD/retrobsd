BUILDPATH = ../../../tools/configsys/../../sys/pic32
H         = ../../../tools/configsys/../../sys/include
M         = ../../../tools/configsys/../../sys/pic32
S         = ../../../tools/configsys/../../sys/kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

KERNOBJ += _startup.o adc.o clock.o cons.o devsw.o exception.o gpio.o init_main.o init_sysent.o kern_clock.o kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_mman.o kern_proc.o kern_prot.o kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o kern_subr.o kern_synch.o kern_sysctl.o kern_time.o machdep.o mem.o rd_sd.o rdisk.o signal.o spi_bus.o subr_prf.o subr_rmap.o swap.o sys_generic.o sys_inode.o sys_pipe.o sys_process.o syscalls.o sysctl.o tty.o tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o ufs_syscalls.o ufs_syscalls2.o usb_device.o usb_function_cdc.o usb_uart.o vers.o vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o 
EXTRA_TARGETS = bootloader 

DEFS += -DADC_ENABLED=YES
DEFS += -DBL_BUTTON_PIN=7
DEFS += -DBL_BUTTON_PORT=TRISE
DEFS += -DBL_LED_PIN=3
DEFS += -DBL_LED_PORT=TRISE
DEFS += -DBUS_DIV=1
DEFS += -DBUS_KHZ='CPU_KHZ/BUS_DIV'
DEFS += -DCONSOLE_DEVICE=ttyUSB0
DEFS += -DCPU_IDIV=2
DEFS += -DCPU_KHZ='((CRYSTAL*1000)/CPU_IDIV*CPU_MUL/CPU_ODIV)'
DEFS += -DCPU_MUL=20
DEFS += -DCPU_ODIV=1
DEFS += -DCRYSTAL=8
DEFS += -DDC0_DEBUG=DEVCFG0_DEBUG_DISABLED
DEFS += -DDC0_ICE=0
DEFS += -DDC1_CKM=0
DEFS += -DDC1_CKS=0
DEFS += -DDC1_FNOSC=DEVCFG1_FNOSC_PRIPLL
DEFS += -DDC1_IESO=DEVCFG1_IESO
DEFS += -DDC1_OSCIOFNC=0
DEFS += -DDC1_PBDIV=DEVCFG1_FPBDIV_1
DEFS += -DDC1_POSCMOD=DEVCFG1_POSCMOD_HS
DEFS += -DDC1_SOSC=0
DEFS += -DDC1_WDTEN=0
DEFS += -DDC1_WDTPS=DEVCFG1_WDTPS_1
DEFS += -DDC2_PLLIDIV=DEVCFG2_FPLLIDIV_2
DEFS += -DDC2_PLLMUL=DEVCFG2_FPLLMUL_20
DEFS += -DDC2_PLLODIV=DEVCFG2_FPLLODIV_1
DEFS += -DDC2_UPLL=0
DEFS += -DDC2_UPLLIDIV=DEVCFG2_UPLLIDIV_2
DEFS += -DDC3_CAN=DEVCFG3_FCANIO
DEFS += -DDC3_ETH=DEVCFG3_FETHIO
DEFS += -DDC3_MII=DEVCFG3_FMIIEN
DEFS += -DDC3_SRS=DEVCFG3_FSRSSEL_7
DEFS += -DDC3_USBID=DEVCFG3_FUSBIDIO
DEFS += -DDC3_USERID=0xffff
DEFS += -DDC3_VBUSON=DEVCFG3_FVBUSONIO
DEFS += -DFLASH_USER=0x1d005000
DEFS += -DGPIO_ENABLED=YES
DEFS += -DHID_FEATURE_REPORT_BYTES=2
DEFS += -DHID_INPUT_REPORT_BYTES=2
DEFS += -DHID_INT_IN_EP_SIZE=64
DEFS += -DHID_INT_OUT_EP_SIZE=64
DEFS += -DHID_OUTPUT_REPORT_BYTES=2
DEFS += -DHID_RPT01_SIZE=29
DEFS += -DKERNEL
DEFS += -DLED_KERNEL_INVERT=YES
DEFS += -DLED_KERNEL_PIN=1
DEFS += -DLED_KERNEL_PORT=TRISD
DEFS += -DPIC32MX7
DEFS += -DSD0_CS_PIN=13
DEFS += -DSD0_CS_PORT=TRISB
DEFS += -DSD0_PORT=2
DEFS += -DUARTUSB_ENABLED=YES
DEFS += -DUCB_METER
DEFS += -DUSB_EP0_BUFF_SIZE=8
DEFS += -DUSB_MAX_EP_NUMBER=3
DEFS += -DUSB_NUM_STRING_DESCRIPTORS=3


LDSCRIPT = ../../../tools/configsys/../../sys/pic32/cfg/bootloader-ubw32.ld

CONFIG = PINGUINO-MICRO
CONFIGPATH = ../../../tools/configsys

include ../../../tools/configsys/../../sys/pic32/kernel-post.mk
