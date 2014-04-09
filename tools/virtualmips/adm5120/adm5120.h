 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __ADM5120_H__
#define __ADM5120_H__

#include "types.h"

/*virtual address and physical address*/
typedef m_uint32_t m_va_t;
typedef m_uint32_t m_pa_t;
typedef m_uint32_t m_reg_t;
typedef m_int32_t m_ireg_t;
typedef m_uint32_t m_cp0_reg_t;

#define  DATA_WIDTH 32          /*64 */
#define LL

/*ADM5120 use soft fpu*/
#define SOFT_FPU   1

/*Guest endian*/
#define GUEST_BYTE_ORDER  ARCH_LITTLE_ENDIAN
#ifndef GUEST_BYTE_ORDER
#error Please define guest architecture in utils.h!
#endif

/* Host to VM conversion functions */
#if HOST_BYTE_ORDER == GUEST_BYTE_ORDER
#define htovm16(x) (x)
#define htovm32(x) (x)
#define htovm64(x) (x)

#define vmtoh16(x) (x)
#define vmtoh32(x) (x)
#define vmtoh64(x) (x)
#elif HOST_BYTE_ORDER==ARCH_LITTLE_ENDIAN       //host:little guest:big
#define htovm16(x) (htons(x))
#define htovm32(x) (htonl(x))
#define htovm64(x) (swap64(x))

#define vmtoh16(x) (ntohs(x))
#define vmtoh32(x) (ntohl(x))
#define vmtoh64(x) (swap64(x))
#else //host:big guest:little

#define htovm16(x) (ntohs(x))
#define htovm32(x) (ntohl(x))
#define htovm64(x) (swap64(x))

#define vmtoh16(x) (htons(x))
#define vmtoh32(x) (htonl(x))
#define vmtoh64(x) (swap64(x))
#endif

#define ADM5120_DEFAULT_CONFIG_FILE     "adm5120.conf"
#define ADM5120_DEFAULT_RAM_SIZE     16
#define ADM5120_DEFAULT_BOOT_METHOD     BOOT_ELF
#define ADM5120_DEFAULT_KERNEL_FILENAME     "vmlinux"
#define ADM5120_ADDR_BUS_MASK   0xffffffff      /*32bit phy address */

#define ADM5120_CONFIG0  0x80000082
#define ADM5120_CONFIG1 0x3E613080      /*CACHE (128SET*32 BYTES*2 WAY)= 8K */
//#define ADM5120_CONFIG7 0x0  

//#define SOC_CONFIG0 ADM5120_CONFIG0
//#define SOC_CONFIG1 ADM5120_CONFIG1
//#define SOC_CONFIG7 ADM5120_CONFIG7

#define ADM5120_ROM_PC  0xbfc00000UL
#define ADM5120_PRID    0x0001800b
#define ADM5120_DEFAULT_TLB_ENTRYNO   16

struct adm5120_system {
    /* Associated VM instance */
    vm_instance_t *vm;
};

typedef struct adm5120_system adm5120_t;

#define VM_ADM5120(vm) ((adm5120_t *)vm->hw_data)

vm_instance_t *create_instance (char *conf);
int init_instance (vm_instance_t * vm);

//void virtual_timer(cpu_mips_t *cpu);

/*---------ADM5120 SOC releated--------------------*/

/*=========================  Physical Memory Map  ============================*/
#define ADM5120_SDRAM_BASE						0
#define ADM5120_SMEM1_BASE						0x10000000

#define ADM5120_EXTIO0_BASE						0x10C00000
#define ADM5120_EXTIO1_BASE						0x10E00000
#define ADM5120_MPMC_BASE						0x11000000
#define ADM5120_USBHOST_BASE					0x11200000
#define ADM5120_PCIMEM_BASE						0x11400000
#define ADM5120_PCIIO_BASE						0x11500000
#define ADM5120_PCICFG_BASE						0x115FFFF0
#define ADM5120_MIPS_BASE						0x11A00000
#define ADM5120_SWCTRL_BASE						0x12000000

#define ADM5120_INTC_BASE						0x12200000
#define ADM5120_SYSC_BASE						0x12400000

#define ADM5120_UART0_BASE						0x12600000
#define ADM5120_UART1_BASE						0x12800000

#define ADM5120_SMEM0_BASE						0x1FC00000

/*=========================  Switch Control Register  ========================*/
/* Control Register */
#define CODE_REG								0x0000
#define SftRest_REG								0x0004
#define Boot_done_REG							0x0008
#define SWReset_REG								0x000C
#define Global_St_REG							0x0010
#define PHY_st_REG								0x0014
#define Port_st_REG								0x0018
#define Mem_control_REG							0x001C
#define SW_conf_REG								0x0020
#define CPUp_conf_REG							0x0024
#define Port_conf0_REG							0x0028
#define Port_conf1_REG							0x002C
#define Port_conf2_REG							0x0030

#define VLAN_G1_REG								0x0040
#define VLAN_G2_REG								0x0044
#define Send_trig_REG							0x0048
#define Srch_cmd_REG							0x004C
#define ADDR_st0_REG							0x0050
#define ADDR_st1_REG							0x0054
#define MAC_wt0_REG								0x0058
#define MAC_wt1_REG								0x005C
#define BW_cntl0_REG							0x0060
#define BW_cntl1_REG							0x0064
#define PHY_cntl0_REG							0x0068
#define PHY_cntl1_REG							0x006C
#define FC_th_REG								0x0070
#define Adj_port_th_REG							0x0074
#define Port_th_REG								0x0078
#define PHY_cntl2_REG							0x007C
#define PHY_cntl3_REG							0x0080
#define Pri_cntl_REG							0x0084
#define VLAN_pri_REG							0x0088
#define TOS_en_REG								0x008C
#define TOS_map0_REG							0x0090
#define TOS_map1_REG							0x0094
#define Custom_pri1_REG							0x0098
#define Custom_pri2_REG							0x009C
#define PHY_cntl4_REG							0x00A0
#define Empty_cnt_REG							0x00A4
#define Port_cnt_sel_REG						0x00A8
#define Port_cnt_REG							0x00AC
#define SW_Int_st_REG							0x00B0
#define SW_Int_mask_REG							0x00B4

// GPIO config
#define GPIO_conf0_REG							0x00B8
#define GPIO_conf2_REG							0x00BC

// Watch dog
#define Watchdog0_REG							0x00C0
#define Watchdog1_REG							0x00C4

#define Swap_in_REG								0x00C8
#define Swap_out_REG							0x00CC

// Tx/Rx Descriptors
#define Send_HBaddr_REG							0x00D0
#define Send_LBaddr_REG							0x00D4
#define Recv_HBaddr_REG							0x00D8
#define Recv_LBaddr_REG							0x00DC
#define Send_HWaddr_REG							0x00E0
#define Send_LWaddr_REG							0x00E4
#define Recv_HWaddr_REG							0x00E8
#define Recv_LWaddr_REG							0x00EC

// Timer Control 
#define Timer_int_REG							0x00F0
#define Timer_REG								0x00F4

// LED control
#define Port0_LED_REG							0x0100
#define Port1_LED_REG							0x0104
#define Port2_LED_REG							0x0108
#define Port3_LED_REG							0x010c
#define Port4_LED_REG							0x0110

#define SW_INDEX_MAX							0x45    //0x0114/4

/* Timer_int_REG */
#define SW_TIMER_INT_DISABLE					0x10000
#define SW_TIMER_INT							0x1

/* Timer_REG */
#define SW_TIMER_EN								0x10000
#define SW_TIMER_MASK							0xffff
#define SW_TIMER_10MS_TICKS						0x3D09
#define SW_TIMER_1MS_TICKS						0x61A
#define SW_TIMER_100US_TICKS					0x9D

/*====================  MultiPort Memory Controller (MPMC) ==================*/
/* registers offset */
#define MPMC_CONTROL_REG						0x0000
#define MPMC_STATUS_REG							0x0004
#define MPMC_CONFIG_REG							0x0008

#define MPMC_DM_CONTROL_REG						0x0020
#define MPMC_DM_REFRESH_REG						0x0024

#define MPMC_DM_TRP_REG							0x0030
#define MPMC_DM_TRAS_REG						0x0034
#define MPMC_DM_TSREX_REG						0x0038
#define MPMC_DM_TAPR_REG						0x003C
#define MPMC_DM_TDAL_REG						0x0040
#define MPMC_DM_TWR_REG							0x0044
#define MPMC_DM_TRC_REG							0x0048
#define MPMC_DM_TRFC_REG						0x004C
#define MPMC_DM_TXSR_REG						0x0050
#define MPMC_DM_TRRD_REG						0x0054
#define MPMC_DM_TMRD_REG						0x0058

#define MPMC_SM_EXTWAIT_REG						0x0080

#define MPMC_DM_CONFIG0_REG						0x0100
#define MPMC_DM_RASCAS0_REG						0x0104

#define MPMC_DM_CONFIG1_REG						0x0120
#define MPMC_DM_RASCAS1_REG						0x0124

#define MPMC_SM_CONFIG0_REG						0x0200
#define MPMC_SM_WAITWEN0_REG					0x0204
#define MPMC_SM_WAITOEN0_REG					0x0208
#define MPMC_SM_WAITRD0_REG						0x020C
#define MPMC_SM_WAITPAGE0_REG					0x0210
#define MPMC_SM_WAITWR0_REG						0x0214
#define MPMC_SM_WAITTURN0_REG					0x0218

#define MPMC_SM_CONFIG1_REG						0x0220
#define MPMC_SM_WAITWEN1_REG					0x0224
#define MPMC_SM_WAITOEN1_REG					0x0228
#define MPMC_SM_WAITRD1_REG						0x022C
#define MPMC_SM_WAITPAGE1_REG					0x0230
#define MPMC_SM_WAITWR1_REG						0x0234
#define MPMC_SM_WAITTURN1_REG					0x0238

#define MPMC_SM_CONFIG2_REG						0x0240
#define MPMC_SM_WAITWEN2_REG					0x0244
#define MPMC_SM_WAITOEN2_REG					0x0248
#define MPMC_SM_WAITRD2_REG						0x024C
#define MPMC_SM_WAITPAGE2_REG					0x0250
#define MPMC_SM_WAITWR2_REG						0x0254
#define MPMC_SM_WAITTURN2_REG					0x0258

#define MPMC_SM_CONFIG3_REG						0x0260
#define MPMC_SM_WAITWEN3_REG					0x0264
#define MPMC_SM_WAITOEN3_REG					0x0268
#define MPMC_SM_WAITRD3_REG						0x026C
#define MPMC_SM_WAITPAGE3_REG					0x0270
#define MPMC_SM_WAITWR3_REG						0x0274
#define MPMC_SM_WAITTURN3_REG					0x0278

#define MPMC_INDEX_MAX							0x9f    //0x027C/4

/*===========================  UART Control Register  
========================*/
#define UART_DR_REG								0x00
#define UART_RSR_REG							0x04
#define UART_ECR_REG							0x04
#define UART_LCR_H_REG							0x08
#define UART_LCR_M_REG							0x0c
#define UART_LCR_L_REG							0x10
#define UART_CR_REG								0x14
#define UART_FR_REG								0x18
#define UART_IIR_REG							0x1c
#define UART_ICR_REG							0x1C
#define UART_ILPR_REG							0x20

/*  rsr/ecr reg  */
#define UART_OVERRUN_ERR						0x08
#define UART_BREAK_ERR							0x04
#define UART_PARITY_ERR							0x02
#define UART_FRAMING_ERR						0x01
#define UART_RX_STATUS_MASK						0x0f
#define UART_RX_ERROR							( UART_BREAK_ERR	\
												| UART_PARITY_ERR	\
												| UART_FRAMING_ERR)

/*  lcr_h reg  */
#define UART_SEND_BREAK							0x01
#define UART_PARITY_EN							0x02
#define UART_EVEN_PARITY						0x04
#define UART_TWO_STOP_BITS						0x08
#define UART_ENABLE_FIFO						0x10

#define UART_WLEN_5BITS							0x00
#define UART_WLEN_6BITS							0x20
#define UART_WLEN_7BITS							0x40
#define UART_WLEN_8BITS							0x60
#define UART_WLEN_MASK							0x60

/*  cr reg  */
#define UART_PORT_EN							0x01
#define UART_SIREN								0x02
#define UART_SIRLP								0x04
#define UART_MODEM_STATUS_INT_EN				0x08
#define UART_RX_INT_EN							0x10
#define UART_TX_INT_EN							0x20
#define UART_RX_TIMEOUT_INT_EN					0x40
#define UART_LOOPBACK_EN						0x80

/*  fr reg  */
#define UART_CTS								0x01
#define UART_DSR								0x02
#define UART_DCD								0x04
#define UART_BUSY								0x08
#define UART_RX_FIFO_EMPTY						0x10
#define UART_TX_FIFO_FULL						0x20
#define UART_RX_FIFO_FULL						0x40
#define UART_TX_FIFO_EMPTY						0x80

/*  iir/icr reg  */
#define UART_MODEM_STATUS_INT					0x01
#define UART_RX_INT								0x02
#define UART_TX_INT								0x04
#define UART_RX_TIMEOUT_INT						0x08

#define UART_INT_MASK							0x0f

#define ADM5120_UARTCLK_FREQ					62500000

/*  uart_baudrate  */
#define UART_230400bps_DIVISOR					UART_BAUDDIV(230400)
// #define UART_115200bps_DIVISOR                                       UART_BAUDDIV(115200)
#define UART_115200bps_DIVISOR					33
// #define UART_76800bps_DIVISOR                                        UART_BAUDDIV(76800)
#define UART_76800bps_DIVISOR					50
// #define UART_57600bps_DIVISOR                                        UART_BAUDDIV(57600)
#define UART_57600bps_DIVISOR					67
//#define UART_38400bps_DIVISOR                                 UART_BAUDDIV(38400)
#define UART_38400bps_DIVISOR					102
//#define UART_19200bps_DIVISOR                                 UART_BAUDDIV(19200)
#define UART_19200bps_DIVISOR					202
//#define UART_14400bps_DIVISOR                                 UART_BAUDDIV(14400)
#define UART_14400bps_DIVISOR					270
//#define UART_9600bps_DIVISOR                                  UART_BAUDDIV(9600)
#define UART_9600bps_DIVISOR					406
//#define UART_2400bps_DIVISOR                                  UART_BAUDDIV(2400)
#define UART_2400bps_DIVISOR					1627
//#define UART_1200bps_DIVISOR                                  UART_BAUDDIV(1200)

#define UART_INDEX_MAX							0x9     //0x024/4

/* pci space 
pci memory  0x1140 0000- 0x114f ffff
pci io            0x1150 0000 - 0x 115f ffef
pci configuration address port  0x115ffff0
pci configuration data port       0x115ffff8

*/
#define ADM5120_PCI_BASE						0x11400000
#define PCI_INDEX_MAX                         0X80000   //0X200000/4

/*==========================  Interrupt Controller  ==========================*/
/* registers offset */
#define IRQ_STATUS_REG							0x00    /* Read */
#define IRQ_RAW_STATUS_REG						0x04    /* Read */
#define IRQ_ENABLE_REG							0x08    /* Read/Write */
#define IRQ_DISABLE_REG							0x0C    /* Write */
#define IRQ_SOFT_REG							0x10    /* Write */

#define IRQ_MODE_REG							0x14    /* Read/Write */
#define FIQ_STATUS_REG							0x18    /* Read */

/* test registers */
#define IRQ_TESTSRC_REG							0x1c    /* Read/Write */
#define IRQ_SRCSEL_REG							0x20    /* Read/Write */
#define IRQ_LEVEL_REG							0x24    /* Read/Write */

#define INTCTRL_INDEX_MAX							0xa     //0x028/4

/* interrupt levels */
#define INT_LVL_TIMER							0       /* Timer */
#define INT_LVL_UART0							1       /* Uart 0 */
#define INT_LVL_UART1							2       /* Uart 1 */
#define INT_LVL_USBHOST							3       /* USB Host */
#define INT_LVL_EXTIO_0							4       /* External I/O 0 */
#define INT_LVL_EXTIO_1							5       /* External I/O 1 */
#define INT_LVL_PCI_0							6       /* PCI 0 */
#define INT_LVL_PCI_1							7       /* PCI 1 */
#define INT_LVL_PCI_2							8       /* PCI 2 */
#define INT_LVL_SWITCH							9       /* Switch */
#define INT_LVL_MAX								INT_LVL_SWITCH

/* interrupts */
#define IRQ_TIMER								(0x1 << INT_LVL_TIMER)
#define IRQ_UART0								(0x1 << INT_LVL_UART0)
#define IRQ_UART1								(0x1 << INT_LVL_UART1)
#define IRQ_USBHOST								(0x1 << INT_LVL_USBHOST)
#define IRQ_EXTIO_0								(0x1 << INT_LVL_EXTIO_0)
#define IRQ_EXTIO_1								(0x1 << INT_LVL_EXTIO_1)
#define IRQ_PCI_INT0							(0x1 << INT_LVL_PCI_0)
#define IRQ_PCI_INT1							(0x1 << INT_LVL_PCI_1)
#define IRQ_PCI_INT2							(0x1 << INT_LVL_PCI_2)
#define IRQ_SWITCH								(0x1 << INT_LVL_SWITCH)

#define IRQ_MASK								0x3ff

#define ADM5120_MIPSINT_SOFT0					0
#define ADM5120_MIPSINT_SOFT1					1
#define ADM5120_MIPSINT_IRQ						2
#define ADM5120_MIPSINT_FIQ						3
#define ADM5120_MIPSINT_REV0					4
#define ADM5120_MIPSINT_REV1					5
#define ADM5120_MIPSINT_REV2					6
#define ADM5120_MIPSINT_TIMER					7

#endif
