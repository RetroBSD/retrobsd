 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

 /*JZ4740 Header file */

#ifndef __JZ4740_H__
#define __JZ4740_H__

#include "types.h"

/*virtual address and physical address*/
typedef m_uint32_t m_va_t;
typedef m_uint32_t m_pa_t;
typedef m_uint32_t m_reg_t;
typedef m_int32_t m_ireg_t;
typedef m_uint32_t m_cp0_reg_t;

#define  DATA_WIDTH 32          /*64 */
#define LL

/*JZ4740 use soft fpu*/
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

#define JZ4740_CONFIG0  0x80000082
#define JZ4740_CONFIG1 0x3E613080       /*CACHE (128SET*32 BYTES*2 WAY)= 8K */
#define JZ4740_CONFIG7 0x0

#define JZ4740_ROM_PC  0x80000004
#define JZ4740_PRID    0x0ad0024f       /*jz4740 prid */
#define JZ4740_DEFAULT_TLB_ENTRYNO   32 /*32 pairs */

/*------------------------REG DEFINE---------------------------------*/
#define NAND_DATAPORT	0x18000000
#define NAND_ADDRPORT	0x18010000
#define NAND_COMMPORT	0x18008000

/*FOR CS8900*/
#define CS8900_IO_BASE  0x8000000
#define CS8900_SIZE         0x10
#define CS8900_DEFAULT_IRQ      107
#define CS8900_GPIO_GROUP      1
/* GPIO is in 4 groups. 32 per group*/
/*

48-79      0
80-111    1
112-143  2
144-175  3

so irq 107 is in gpio group 1
*/

/*---------------------GPIO----------------------------------*/
#define JZ4740_GPIO_BASE       0x10010000
#define JZ4740_GPIO_SIZE       0x388
//n = 0,1,2,3
#define GPIO_PXPIN(n)	( (0x00 + (n)*0x100))   /* PIN Level Register */
#define GPIO_PXDAT(n)	((0x10 + (n)*0x100))    /* Port Data Register */
#define GPIO_PXDATS(n)	( (0x14 + (n)*0x100))   /* Port Data Set Register */
#define GPIO_PXDATC(n)	( (0x18 + (n)*0x100))   /* Port Data Clear Register */
#define GPIO_PXIM(n)	( (0x20 + (n)*0x100))   /* Interrupt Mask Register */
#define GPIO_PXIMS(n)	( (0x24 + (n)*0x100))   /* Interrupt Mask Set Reg */
#define GPIO_PXIMC(n)	( (0x28 + (n)*0x100))   /* Interrupt Mask Clear Reg */
#define GPIO_PXPE(n)	((0x30 + (n)*0x100))    /* Pull Enable Register */
#define GPIO_PXPES(n)	( (0x34 + (n)*0x100))   /* Pull Enable Set Reg. */
#define GPIO_PXPEC(n)	( (0x38 + (n)*0x100))   /* Pull Enable Clear Reg. */
#define GPIO_PXFUN(n)	( (0x40 + (n)*0x100))   /* Function Register */
#define GPIO_PXFUNS(n)	( (0x44 + (n)*0x100))   /* Function Set Register */
#define GPIO_PXFUNC(n)	( (0x48 + (n)*0x100))   /* Function Clear Register */
#define GPIO_PXSEL(n)	( (0x50 + (n)*0x100))   /* Select Register */
#define GPIO_PXSELS(n)	( (0x54 + (n)*0x100))   /* Select Set Register */
#define GPIO_PXSELC(n)	( (0x58 + (n)*0x100))   /* Select Clear Register */
#define GPIO_PXDIR(n)	( (0x60 + (n)*0x100))   /* Direction Register */
#define GPIO_PXDIRS(n)	( (0x64 + (n)*0x100))   /* Direction Set Register */
#define GPIO_PXDIRC(n)	( (0x68 + (n)*0x100))   /* Direction Clear Register */
#define GPIO_PXTRG(n)	( (0x70 + (n)*0x100))   /* Trigger Register */
#define GPIO_PXTRGS(n)	( (0x74 + (n)*0x100))   /* Trigger Set Register */
#define GPIO_PXTRGC(n)	( (0x78 + (n)*0x100))   /* Trigger Set Register */
#define GPIO_PXFLG(n)	( (0x80 + (n)*0x100))   /* Port Flag Register */
/* According to datasheet, it is 0x14. I think it shoud be 0x84*/
#define GPIO_PXFLGC(n)	( (0x84 + (n)*0x100))   /* Port Flag clear Register */

#define JZ4740_GPIO_INDEX_MAX  0xe2     /*0x388/4 */

/*---------------------UART----------------------------------*/

#define JZ4740_UART_SIZE       0x302c

#define JZ4740_UART0_BASE       0x10030000
#define JZ4740_UART0_SIZE       0x2c

#define JZ4740_UART1_BASE       0x10031000
#define JZ4740_UART1_SIZE       0x2c
#define JZ4740_UART2_BASE       0x10032000
#define JZ4740_UART2_SIZE       0x2c
#define JZ4740_UART3_BASE       0x10033000
#define JZ4740_UART3_SIZE       0x2c

#define JZ4740_UART_BASE       JZ4740_UART0_BASE

#define UART_RBR         (0x00)
#define UART_THR         (0x00)
#define UART_DLLR        (0x00)
#define UART_DLHR        (0x04)
#define UART_IER         (0x04)
#define UART_IIR         (0x08)
#define UART_FCR         (0x08)
#define UART_LCR         (0x0C)
#define UART_MCR         (0x10)
#define UART_LSR         (0x14)
#define UART_MSR         (0x18)
#define UART_SPR         (0x1C)
#define UART_ISR         (0x20)
#define UART_UMR         (0x24)
#define UART_UACR        (0x28)

#define JZ4740_UART_INDEX_MAX							0xb     //0x02c/4
#define JZ4740_UART_NUMBER                            2 //we emulates two uarts

#define UART_IER_RDRIE      0x01
#define UART_IER_TDRIE      0x02
#define UART_IER_RLSIE      0x04
#define UART_IER_MSIE      0x08
#define UART_IER_RTOIE      0x10

#define UART_FCR_FME   0x1
#define UART_FCR_RFRT   0x2
#define UART_FCR_TFRT   0x4
#define UART_FCR_DME   0x8
#define UART_FCR_UME   0x10
#define UART_FCR_RDTR   0xC0
#define UART_FCR_RDTR_SHIFT   0x6

#define UART_LSR_DRY   0x1
#define UART_LSR_OVER   0x2
#define UART_LSR_PARER   0x4
#define UART_LSR_FMER   0x8
#define UART_LSR_BI   0x10
#define UART_LSR_TDRQ   0x20
#define UART_LSR_TEMP   0x40
#define UART_LSR_FIFOE   0x80

/*-------------------PLL----------------------*/

#define JZ4740_CPM_BASE 0x10000000
#define JZ4740_CPM_SIZE 0X70

#define CPM_CPCCR       (0x00)
#define CPM_CPPCR       (0x10)
#define CPM_I2SCDR      (0x60)
#define CPM_LPCDR       (0x64)
#define CPM_MSCCDR      (0x68)
#define CPM_UHCCDR      (0x6C)

#define CPM_LCR         (0x04)
#define CPM_CLKGR       (0x20)
#define CPM_SCR         (0x24)

#define CPM_HCR         (0x30)
#define CPM_HWFCR       (0x34)
#define CPM_HRCR        (0x38)
#define CPM_HWCR        (0x3c)
#define CPM_HWSR        (0x40)
#define CPM_HSPR        (0x44)

#define JZ4740_CPM_INDEX_MAX 0X1c       /*0X70/4 */

/*-------------------EMC----------------------*/
#define JZ4740_EMC_BASE 0x13010000
#define JZ4740_EMC_SIZE 0xa400  /*FROM A000-A3FF is mode register */

#define EMC_BCR         ( 0x0)  /* BCR */
#define EMC_SMCR0       ( 0x10) /* Static Memory Control Register 0 */
#define EMC_SMCR1       ( 0x14) /* Static Memory Control Register 1 */
#define EMC_SMCR2       ( 0x18) /* Static Memory Control Register 2 */
#define EMC_SMCR3       ( 0x1c) /* Static Memory Control Register 3 */
#define EMC_SMCR4       ( 0x20) /* Static Memory Control Register 4 */
#define EMC_SACR0       ( 0x30) /* Static Memory Bank 0 Addr Config Reg */
#define EMC_SACR1       ( 0x34) /* Static Memory Bank 1 Addr Config Reg */
#define EMC_SACR2       ( 0x38) /* Static Memory Bank 2 Addr Config Reg */
#define EMC_SACR3       ( 0x3c) /* Static Memory Bank 3 Addr Config Reg */
#define EMC_SACR4       ( 0x40) /* Static Memory Bank 4 Addr Config Reg */

#define EMC_NFCSR       ( 0x050)        /* NAND Flash Control/Status Register */
#define EMC_NFECR       ( 0x100)        /* NAND Flash ECC Control Register */
#define EMC_NFECC       ( 0x104)        /* NAND Flash ECC Data Register */
#define EMC_NFPAR0      ( 0x108)        /* NAND Flash RS Parity 0 Register */
#define EMC_NFPAR1      ( 0x10c)        /* NAND Flash RS Parity 1 Register */
#define EMC_NFPAR2      ( 0x110)        /* NAND Flash RS Parity 2 Register */
#define EMC_NFINTS      ( 0x114)        /* NAND Flash Interrupt Status Register */
#define EMC_NFINTE      ( 0x118)        /* NAND Flash Interrupt Enable Register */
#define EMC_NFERR0      ( 0x11c)        /* NAND Flash RS Error Report 0 Register */
#define EMC_NFERR1      ( 0x120)        /* NAND Flash RS Error Report 1 Register */
#define EMC_NFERR2      ( 0x124)        /* NAND Flash RS Error Report 2 Register */
#define EMC_NFERR3      ( 0x128)        /* NAND Flash RS Error Report 3 Register */

#define EMC_DMCR        ( 0x80) /* DRAM Control Register */
#define EMC_RTCSR       ( 0x84) /* Refresh Time Control/Status Register */
#define EMC_RTCNT       ( 0x88) /* Refresh Timer Counter */
#define EMC_RTCOR       ( 0x8c) /* Refresh Time Constant Register */
#define EMC_DMAR0       ( 0x90) /* SDRAM Bank 0 Addr Config Register */

#define EMC_SDMR0       ( 0xa000)       /* Mode Register of SDRAM bank 0 */
/*has other register*/

#define JZ4740_EMC_INDEX_MAX 0x4b       /*0x12c/4 */

/*-----------------------RTC-------------------------------------*/

#define JZ4740_RTC_BASE 0x10003000
#define JZ4740_RTC_SIZE  0x38
#define RTC_RCR         ( 0x00) /* RTC Control Register */
#define RTC_RSR         ( 0x04) /* RTC Second Register */
#define RTC_RSAR        ( 0x08) /* RTC Second Alarm Register */
#define RTC_RGR         ( 0x0c) /* RTC Regulator Register */

#define RTC_HCR         ( 0x20) /* Hibernate Control Register */
#define RTC_HWFCR       ( 0x24) /* Hibernate Wakeup Filter Counter Reg */
#define RTC_HRCR        ( 0x28) /* Hibernate Reset Counter Register */
#define RTC_HWCR        ( 0x2c) /* Hibernate Wakeup Control Register */
#define RTC_HWRSR       ( 0x30) /* Hibernate Wakeup Status Register */
#define RTC_HSPR        ( 0x34) /* Hibernate Scratch Pattern Register */

#define RTC_RCR_WRDY_BIT 7
#define RTC_RCR_WRDY    (1 << 7)        /* Write Ready Flag */
#define RTC_RCR_1HZ_BIT 6
#define RTC_RCR_1HZ     (1 << RTC_RCR_1HZ_BIT)  /* 1Hz Flag */
#define RTC_RCR_1HZIE   (1 << 5)        /* 1Hz Interrupt Enable */
#define RTC_RCR_AF_BIT  4
#define RTC_RCR_AF      (1 << RTC_RCR_AF_BIT)   /* Alarm Flag */
#define RTC_RCR_AIE     (1 << 3)        /* Alarm Interrupt Enable */
#define RTC_RCR_AE      (1 << 2)        /* Alarm Enable */
#define RTC_RCR_RTCE    (1 << 0)        /* RTC Enable */

 /* RTC Regulator Register */
#define RTC_RGR_LOCK            (1 << 31)       /* Lock Bit */
#define RTC_RGR_ADJC_BIT        16
#define RTC_RGR_ADJC_MASK       (0x3ff << RTC_RGR_ADJC_BIT)
#define RTC_RGR_NC1HZ_BIT       0
#define RTC_RGR_NC1HZ_MASK      (0xffff << RTC_RGR_NC1HZ_BIT)

 /* Hibernate Control Register */
#define RTC_HCR_PD              (1 << 0)        /* Power Down */

 /* Hibernate Wakeup Filter Counter Register */
#define RTC_HWFCR_BIT           5
#define RTC_HWFCR_MASK          (0x7ff << RTC_HWFCR_BIT)

 /* Hibernate Reset Counter Register */
#define RTC_HRCR_BIT            5
#define RTC_HRCR_MASK           (0x7f << RTC_HRCR_BIT)

 /* Hibernate Wakeup Control Register */
#define RTC_HWCR_EALM           (1 << 0)        /* RTC alarm wakeup enable */

 /* Hibernate Wakeup Status Register */
#define RTC_HWRSR_HR            (1 << 5)        /* Hibernate reset */
#define RTC_HWRSR_PPR           (1 << 4)        /* PPR reset */
#define RTC_HWRSR_PIN           (1 << 1)        /* Wakeup pin status bit */
#define RTC_HWRSR_ALM           (1 << 0)        /* RTC alarm status bit */

#define JZ4740_RTC_INDEX_MAX 0xe        /*0x38/4 */

/*----------------------WDT&TCU--------------------------------*/
#define JZ4740_WDT_TCU_BASE 0x10002000
#define JZ4740_WDT_TCU_SIZE  0xa0

#define WDT_TDR         ( 0x00)
#define WDT_TCER        ( 0x04)
#define WDT_TCNT        ( 0x08)
#define WDT_TCSR        ( 0x0C)

#define TCU_CLOCK_EXT  0x4
#define TCU_CLOCK_RTC  0x2
#define TCU_CLOCK_PCK  0x0
#define TCU_CLOCK_SOUCE_MASK  0x7

#define TCU_CLOCK_PRESCALE_MASK  0x38
#define TCU_CLOCK_PRESCALE_OFFSET  0x3

#define WDT_CLOCK_EXT  0x4
#define WDT_CLOCK_RTC  0x2
#define WDT_CLOCK_PCK  0x0
#define WDT_CLOCK_SOUCE_MASK  0x7

#define WDT_CLOCK_PRESCALE_MASK  0x38
#define WDT_CLOCK_PRESCALE_OFFSET  0x3

#define EXT_CLOCK  12000000     /*12M */
#define RTC_CLOCK       32768

/*************************************************************************
 * TCU (Timer Counter Unit)
 *************************************************************************/
#define TCU_TSR		( 0x1C) /* Timer Stop Register */
#define TCU_TSSR	( 0x2C) /* Timer Stop Set Register */
#define TCU_TSCR	( 0x3C) /* Timer Stop Clear Register */
#define TCU_TER		( 0x10) /* Timer Counter Enable Register */
#define TCU_TESR	( 0x14) /* Timer Counter Enable Set Register */
#define TCU_TECR	( 0x18) /* Timer Counter Enable Clear Register */
#define TCU_TFR		( 0x20) /* Timer Flag Register */
#define TCU_TFSR	( 0x24) /* Timer Flag Set Register */
#define TCU_TFCR	( 0x28) /* Timer Flag Clear Register */
#define TCU_TMR		( 0x30) /* Timer Mask Register */
#define TCU_TMSR	( 0x34) /* Timer Mask Set Register */
#define TCU_TMCR	( 0x38) /* Timer Mask Clear Register */
#define TCU_TDFR0	( 0x40) /* Timer Data Full Register */
#define TCU_TDHR0	( 0x44) /* Timer Data Half Register */
#define TCU_TCNT0	( 0x48) /* Timer Counter Register */
#define TCU_TCSR0	( 0x4C) /* Timer Control Register */
#define TCU_TDFR1	( 0x50)
#define TCU_TDHR1	( 0x54)
#define TCU_TCNT1	( 0x58)
#define TCU_TCSR1	( 0x5C)
#define TCU_TDFR2	 (0x60)
#define TCU_TDHR2	( 0x64)
#define TCU_TCNT2	( 0x68)
#define TCU_TCSR2	( 0x6C)
#define TCU_TDFR3	( 0x70)
#define TCU_TDHR3	( 0x74)
#define TCU_TCNT3	( 0x78)
#define TCU_TCSR3	( 0x7C)
#define TCU_TDFR4	( 0x80)
#define TCU_TDHR4	( 0x84)
#define TCU_TCNT4	( 0x88)
#define TCU_TCSR4	( 0x8C)
#define TCU_TDFR5	( 0x90)
#define TCU_TDHR5	( 0x94)
#define TCU_TCNT5	( 0x98)
#define TCU_TCSR5	( 0x9C)

#define TCU_TDFR(n)   (TCU_TDFR0+n*0x10)
#define TCU_TDHR(n)   (TCU_TDHR0+n*0x10)
#define TCU_TCNT(n)   (TCU_TCNT0+n*0x10)
#define TCU_TCSR(n)   (TCU_TCSR0+n*0x10)

#define  WDT_TIMER_STOP  0x10000
#define JZ4740_WDT_INDEX_MAX 0x28       /*0xa0/4 */

/*-------------------LCD---------------------*/
#define JZ4740_LCD_BASE 0x13050000
#define JZ4740_LCD_SIZE  0x60

#define LCD_CFG         ( 0x00) /* LCD Configure Register */
#define LCD_VSYNC       ( 0x04) /* Vertical Synchronize Register */
#define LCD_HSYNC       ( 0x08) /* Horizontal Synchronize Register */
#define LCD_VAT         ( 0x0c) /* Virtual Area Setting Register */
#define LCD_DAH         ( 0x10) /* Display Area Horizontal Start/End Point */
#define LCD_DAV         ( 0x14) /* Display Area Vertical Start/End Point */
#define LCD_PS          ( 0x18) /* PS Signal Setting */
#define LCD_CLS         ( 0x1c) /* CLS Signal Setting */
#define LCD_SPL         ( 0x20) /* SPL Signal Setting */
#define LCD_REV         ( 0x24) /* REV Signal Setting */
#define LCD_CTRL        ( 0x30) /* LCD Control Register */
#define LCD_STATE       ( 0x34) /* LCD Status Register */
#define LCD_IID         ( 0x38) /* Interrupt ID Register */
#define LCD_DA0         ( 0x40) /* Descriptor Address Register 0 */
#define LCD_SA0         ( 0x44) /* Source Address Register 0 */
#define LCD_FID0        ( 0x48) /* Frame ID Register 0 */
#define LCD_CMD0        ( 0x4c) /* DMA Command Register 0 */
#define LCD_DA1         ( 0x50) /* Descriptor Address Register 1 */
#define LCD_SA1         ( 0x54) /* Source Address Register 1 */
#define LCD_FID1        ( 0x58) /* Frame ID Register 1 */
#define LCD_CMD1        ( 0x5c) /* DMA Command Register 1 */

#define LCD_CTRL_BST_BIT        28      /* Burst Length Selection */
#define LCD_CTRL_BST_MASK       (0x03 << LCD_CTRL_BST_BIT)
#define LCD_CTRL_BST_4        (0 << LCD_CTRL_BST_BIT)   /* 4-word */
#define LCD_CTRL_BST_8        (1 << LCD_CTRL_BST_BIT)   /* 8-word */
#define LCD_CTRL_BST_16       (2 << LCD_CTRL_BST_BIT)   /* 16-word */
#define LCD_CTRL_RGB565         (0 << 27)       /* RGB565 mode */
#define LCD_CTRL_RGB555         (1 << 27)       /* RGB555 mode */
#define LCD_CTRL_OFUP           (1 << 26)       /* Output FIFO underrun protection enable */
#define LCD_CTRL_FRC_BIT        24      /* STN FRC Algorithm Selection */
#define LCD_CTRL_FRC_MASK       (0x03 << LCD_CTRL_FRC_BIT)
#define LCD_CTRL_FRC_16       (0 << LCD_CTRL_FRC_BIT)   /* 16 grayscale */
#define LCD_CTRL_FRC_4        (1 << LCD_CTRL_FRC_BIT)   /* 4 grayscale */
#define LCD_CTRL_FRC_2        (2 << LCD_CTRL_FRC_BIT)   /* 2 grayscale */
#define LCD_CTRL_PDD_BIT        16      /* Load Palette Delay Counter */
#define LCD_CTRL_PDD_MASK       (0xff << LCD_CTRL_PDD_BIT)
#define LCD_CTRL_EOFM           (1 << 13)       /* EOF interrupt mask */
#define LCD_CTRL_SOFM           (1 << 12)       /* SOF interrupt mask */
#define LCD_CTRL_OFUM           (1 << 11)       /* Output FIFO underrun interrupt mask */
#define LCD_CTRL_IFUM0          (1 << 10)       /* Input FIFO 0 underrun interrupt mask */
#define LCD_CTRL_IFUM1          (1 << 9)        /* Input FIFO 1 underrun interrupt mask */
#define LCD_CTRL_LDDM           (1 << 8)        /* LCD disable done interrupt mask */
#define LCD_CTRL_QDM            (1 << 7)        /* LCD quick disable done interrupt mask */
#define LCD_CTRL_BEDN           (1 << 6)        /* Endian selection */
#define LCD_CTRL_PEDN           (1 << 5)        /* Endian in byte:0-msb first, 1-lsb first */
#define LCD_CTRL_DIS            (1 << 4)        /* Disable indicate bit */
#define LCD_CTRL_ENA            (1 << 3)        /* LCD enable bit */
#define LCD_CTRL_BPP_BIT        0       /* Bits Per Pixel */
#define LCD_CTRL_BPP_MASK       (0x07 << LCD_CTRL_BPP_BIT)
#define LCD_CTRL_BPP_1        (0 << LCD_CTRL_BPP_BIT)   /* 1 bpp */
#define LCD_CTRL_BPP_2        (1 << LCD_CTRL_BPP_BIT)   /* 2 bpp */
#define LCD_CTRL_BPP_4        (2 << LCD_CTRL_BPP_BIT)   /* 4 bpp */
#define LCD_CTRL_BPP_8        (3 << LCD_CTRL_BPP_BIT)   /* 8 bpp */
#define LCD_CTRL_BPP_16       (4 << LCD_CTRL_BPP_BIT)   /* 15/16 bpp */
#define LCD_CTRL_BPP_18_24    (5 << LCD_CTRL_BPP_BIT)   /* 18/24/32 bpp */

 /* Display Area Horizontal Start/End Point Register */
#define LCD_DAH_HDS_BIT         16      /* Horizontal display area start in dot clock */
#define LCD_DAH_HDS_MASK        (0xffff << LCD_DAH_HDS_BIT)
#define LCD_DAH_HDE_BIT         0       /* Horizontal display area end in dot clock */
#define LCD_DAH_HDE_MASK        (0xffff << LCD_DAH_HDE_BIT)

 /* Display Area Vertical Start/End Point Register */
#define LCD_DAV_VDS_BIT         16      /* Vertical display area start in line clock */
#define LCD_DAV_VDS_MASK        (0xffff << LCD_DAV_VDS_BIT)
#define LCD_DAV_VDE_BIT         0       /* Vertical display area end in line clock */
#define LCD_DAV_VDE_MASK        (0xffff << LCD_DAV_VDE_BIT)

#define LCD_STATE_QD            (1 << 7)        /* Quick Disable Done */
#define LCD_STATE_EOF           (1 << 5)        /* EOF Flag */
#define LCD_STATE_SOF           (1 << 4)        /* SOF Flag */
#define LCD_STATE_OFU           (1 << 3)        /* Output FIFO Underrun */
#define LCD_STATE_IFU0          (1 << 2)        /* Input FIFO 0 Underrun */
#define LCD_STATE_IFU1          (1 << 1)        /* Input FIFO 1 Underrun */
#define LCD_STATE_LDD           (1 << 0)        /* LCD Disabled */

#define LCD_CMD_SOFINT          (1 << 31)
#define LCD_CMD_EOFINT          (1 << 30)
#define LCD_CMD_PAL             (1 << 28)
#define LCD_CMD_LEN_BIT         0
#define LCD_CMD_LEN_MASK        (0xffffff << LCD_CMD_LEN_BIT)

#define JZ4740_LCD_INDEX_MAX 0x18       /*0x60/4 */

/*---------------------touch screen------------------*/
#define JZ4740_TS_BASE 0x10070000
#define JZ4740_TS_SIZE  0x28

#define SADC_ENA        ( 0x00) /* ADC Enable Register */
#define SADC_CFG        ( 0x04) /* ADC Configure Register */
#define SADC_CTRL       ( 0x08) /* ADC Control Register */
#define SADC_STATE      ( 0x0C) /* ADC Status Register */
#define SADC_SAMETIME   ( 0x10) /* ADC Same Point Time Register */
#define SADC_WAITTIME   ( 0x14) /* ADC Wait Time Register */
#define SADC_TSDAT      ( 0x18) /* ADC Touch Screen Data Register */
#define SADC_BATDAT     ( 0x1C) /* ADC PBAT Data Register */
#define SADC_SADDAT     ( 0x20) /* ADC SADCIN Data Register */

 /* ADC Enable Register */
#define SADC_ENA_ADEN           (1 << 7)        /* Touch Screen Enable */
#define SADC_ENA_TSEN           (1 << 2)        /* Touch Screen Enable */
#define SADC_ENA_PBATEN         (1 << 1)        /* PBAT Enable */
#define SADC_ENA_SADCINEN       (1 << 0)        /* SADCIN Enable */

 /* ADC Configure Register */
#define SADC_CFG_EXIN           (1 << 30)
#define SADC_CFG_CLKOUT_NUM_BIT 16
#define SADC_CFG_CLKOUT_NUM_MASK (0x7 << SADC_CFG_CLKOUT_NUM_BIT)
#define SADC_CFG_TS_DMA         (1 << 15)       /* Touch Screen DMA Enable */
#define SADC_CFG_XYZ_BIT        13      /* XYZ selection */
#define SADC_CFG_XYZ_MASK       (0x3 << SADC_CFG_XYZ_BIT)
#define SADC_CFG_XY           (0 << SADC_CFG_XYZ_BIT)
#define SADC_CFG_XYZ          (1 << SADC_CFG_XYZ_BIT)
#define SADC_CFG_XYZ1Z2       (2 << SADC_CFG_XYZ_BIT)
#define SADC_CFG_SNUM_BIT       10      /* Sample Number */
#define SADC_CFG_SNUM_MASK      (0x7 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_1       (0x0 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_2       (0x1 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_3       (0x2 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_4       (0x3 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_5       (0x4 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_6       (0x5 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_8       (0x6 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_SNUM_9       (0x7 << SADC_CFG_SNUM_BIT)
#define SADC_CFG_CLKDIV_BIT     5       /* AD Converter frequency clock divider */
#define SADC_CFG_CLKDIV_MASK    (0x1f << SADC_CFG_CLKDIV_BIT)
#define SADC_CFG_PBAT_HIGH      (0 << 4)        /* PBAT >= 2.5V */
#define SADC_CFG_PBAT_LOW       (1 << 4)        /* PBAT < 2.5V */
#define SADC_CFG_CMD_BIT        0       /* ADC Command */
#define SADC_CFG_CMD_MASK       (0xf << SADC_CFG_CMD_BIT)
#define SADC_CFG_CMD_X_SE     (0x0 << SADC_CFG_CMD_BIT) /* X Single-End */
#define SADC_CFG_CMD_Y_SE     (0x1 << SADC_CFG_CMD_BIT) /* Y Single-End */
#define SADC_CFG_CMD_X_DIFF   (0x2 << SADC_CFG_CMD_BIT) /* X Differential */
#define SADC_CFG_CMD_Y_DIFF   (0x3 << SADC_CFG_CMD_BIT) /* Y Differential */
#define SADC_CFG_CMD_Z1_DIFF  (0x4 << SADC_CFG_CMD_BIT) /* Z1 Differential */
#define SADC_CFG_CMD_Z2_DIFF  (0x5 << SADC_CFG_CMD_BIT) /* Z2 Differential */
#define SADC_CFG_CMD_Z3_DIFF  (0x6 << SADC_CFG_CMD_BIT) /* Z3 Differential */
#define SADC_CFG_CMD_Z4_DIFF  (0x7 << SADC_CFG_CMD_BIT) /* Z4 Differential */
#define SADC_CFG_CMD_TP_SE    (0x8 << SADC_CFG_CMD_BIT) /* Touch Pressure */
#define SADC_CFG_CMD_PBATH_SE (0x9 << SADC_CFG_CMD_BIT) /* PBAT >= 2.5V */
#define SADC_CFG_CMD_PBATL_SE (0xa << SADC_CFG_CMD_BIT) /* PBAT < 2.5V */
#define SADC_CFG_CMD_SADCIN_SE (0xb << SADC_CFG_CMD_BIT)        /* Measure SADCIN */
#define SADC_CFG_CMD_INT_PEN  (0xc << SADC_CFG_CMD_BIT) /* INT_PEN Enable */

 /* ADC Control Register */
#define SADC_CTRL_PENDM         (1 << 4)        /* Pen Down Interrupt Mask */
#define SADC_CTRL_PENUM         (1 << 3)        /* Pen Up Interrupt Mask */
#define SADC_CTRL_TSRDYM        (1 << 2)        /* Touch Screen Data Ready Interrupt Mask */
#define SADC_CTRL_PBATRDYM      (1 << 1)        /* PBAT Data Ready Interrupt Mask */
#define SADC_CTRL_SRDYM         (1 << 0)        /* SADCIN Data Ready Interrupt Mask */

 /* ADC Status Register */
#define SADC_STATE_TSBUSY       (1 << 7)        /* TS A/D is working */
#define SADC_STATE_PBATBUSY     (1 << 6)        /* PBAT A/D is working */
#define SADC_STATE_SBUSY        (1 << 5)        /* SADCIN A/D is working */
#define SADC_STATE_PEND         (1 << 4)        /* Pen Down Interrupt Flag */
#define SADC_STATE_PENU         (1 << 3)        /* Pen Up Interrupt Flag */
#define SADC_STATE_TSRDY        (1 << 2)        /* Touch Screen Data Ready Interrupt Flag */
#define SADC_STATE_PBATRDY      (1 << 1)        /* PBAT Data Ready Interrupt Flag */
#define SADC_STATE_SRDY         (1 << 0)        /* SADCIN Data Ready Interrupt Flag */

 /* ADC Touch Screen Data Register */
#define SADC_TSDAT_DATA0_BIT    0
#define SADC_TSDAT_DATA0_MASK   (0xfff << SADC_TSDAT_DATA0_BIT)
#define SADC_TSDAT_TYPE0        (1 << 15)
#define SADC_TSDAT_DATA1_BIT    16
#define SADC_TSDAT_DATA1_MASK   (0xfff << SADC_TSDAT_DATA1_BIT)
#define SADC_TSDAT_TYPE1        (1 << 31)

#define JZ4740_TS_INDEX_MAX 0xA /*0x28/4 */

/*--------------DMA--------------------*/
#define JZ4740_DMA_BASE 0x13020000
#define JZ4740_DMA_SIZE  0x310
#define MAX_DMA_NUM	6       /* max 6 channels */

#define DMAC_DSAR(n)	( (0x00 + (n) * 0x20))  /* DMA source address */
#define DMAC_DTAR(n)	( (0x04 + (n) * 0x20))  /* DMA target address */
#define DMAC_DTCR(n)	( (0x08 + (n) * 0x20))  /* DMA transfer count */
#define DMAC_DRSR(n)	( (0x0c + (n) * 0x20))  /* DMA request source */
#define DMAC_DCCSR(n)	( (0x10 + (n) * 0x20))  /* DMA control/status */
#define DMAC_DCMD(n)	( (0x14 + (n) * 0x20))  /* DMA command */
#define DMAC_DDA(n)	( (0x18 + (n) * 0x20))  /* DMA descriptor address */
#define DMAC_DMACR	( 0x0300)       /* DMA control register */
#define DMAC_DMAIPR	( 0x0304)       /* DMA interrupt pending */
#define DMAC_DMADBR	( 0x0308)       /* DMA doorbell */
#define DMAC_DMADBSR	( 0x030C)       /* DMA doorbell set */

// channel 0
#define DMAC_DSAR0      DMAC_DSAR(0)
#define DMAC_DTAR0      DMAC_DTAR(0)
#define DMAC_DTCR0      DMAC_DTCR(0)
#define DMAC_DRSR0      DMAC_DRSR(0)
#define DMAC_DCCSR0     DMAC_DCCSR(0)
#define DMAC_DCMD0	DMAC_DCMD(0)
#define DMAC_DDA0	DMAC_DDA(0)

// channel 1
#define DMAC_DSAR1      DMAC_DSAR(1)
#define DMAC_DTAR1      DMAC_DTAR(1)
#define DMAC_DTCR1      DMAC_DTCR(1)
#define DMAC_DRSR1      DMAC_DRSR(1)
#define DMAC_DCCSR1     DMAC_DCCSR(1)
#define DMAC_DCMD1	DMAC_DCMD(1)
#define DMAC_DDA1	DMAC_DDA(1)

// channel 2
#define DMAC_DSAR2      DMAC_DSAR(2)
#define DMAC_DTAR2      DMAC_DTAR(2)
#define DMAC_DTCR2      DMAC_DTCR(2)
#define DMAC_DRSR2      DMAC_DRSR(2)
#define DMAC_DCCSR2     DMAC_DCCSR(2)
#define DMAC_DCMD2	DMAC_DCMD(2)
#define DMAC_DDA2	DMAC_DDA(2)

// channel 3
#define DMAC_DSAR3      DMAC_DSAR(3)
#define DMAC_DTAR3      DMAC_DTAR(3)
#define DMAC_DTCR3      DMAC_DTCR(3)
#define DMAC_DRSR3      DMAC_DRSR(3)
#define DMAC_DCCSR3     DMAC_DCCSR(3)
#define DMAC_DCMD3	DMAC_DCMD(3)
#define DMAC_DDA3	DMAC_DDA(3)

// channel 4
#define DMAC_DSAR4      DMAC_DSAR(4)
#define DMAC_DTAR4      DMAC_DTAR(4)
#define DMAC_DTCR4      DMAC_DTCR(4)
#define DMAC_DRSR4      DMAC_DRSR(4)
#define DMAC_DCCSR4     DMAC_DCCSR(4)
#define DMAC_DCMD4	DMAC_DCMD(4)
#define DMAC_DDA4	DMAC_DDA(4)

// channel 5
#define DMAC_DSAR5      DMAC_DSAR(5)
#define DMAC_DTAR5      DMAC_DTAR(5)
#define DMAC_DTCR5      DMAC_DTCR(5)
#define DMAC_DRSR5      DMAC_DRSR(5)
#define DMAC_DCCSR5     DMAC_DCCSR(5)
#define DMAC_DCMD5	DMAC_DCMD(5)
#define DMAC_DDA5	DMAC_DDA(5)

#define JZ4740_DMA_INDEX_MAX 0xC4       /*0x310/4 */

// DMA request source register
#define DMAC_DRSR_RS_BIT	0
#define DMAC_DRSR_RS_MASK	(0x1f << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_AUTO	(8 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART0OUT	(20 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_UART0IN	(21 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SSIOUT	(22 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SSIIN	(23 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_AICOUT	(24 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_AICIN	(25 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSCOUT	(26 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_MSCIN	(27 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_TCU	(28 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SADC	(29 << DMAC_DRSR_RS_BIT)
#define DMAC_DRSR_RS_SLCD	(30 << DMAC_DRSR_RS_BIT)

// DMA channel control/status register
#define DMAC_DCCSR_NDES		(1 << 31)       /* descriptor (0) or not (1) ? */
#define DMAC_DCCSR_CDOA_BIT	16      /* copy of DMA offset address */
#define DMAC_DCCSR_CDOA_MASK	(0xff << DMAC_DCCSR_CDOA_BIT)
#define DMAC_DCCSR_INV		(1 << 6)        /* descriptor invalid */
#define DMAC_DCCSR_AR		(1 << 4)        /* address error */
#define DMAC_DCCSR_TT		(1 << 3)        /* transfer terminated */
#define DMAC_DCCSR_HLT		(1 << 2)        /* DMA halted */
#define DMAC_DCCSR_CT		(1 << 1)        /* count terminated */
#define DMAC_DCCSR_EN		(1 << 0)        /* channel enable bit */

// DMA channel command register 
#define DMAC_DCMD_SAI		(1 << 23)       /* source address increment */
#define DMAC_DCMD_DAI		(1 << 22)       /* dest address increment */
#define DMAC_DCMD_RDIL_BIT	16      /* request detection interval length */
#define DMAC_DCMD_RDIL_MASK	(0x0f << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_IGN	(0 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_2	(1 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_4	(2 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_8	(3 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_12	(4 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_16	(5 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_20	(6 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_24	(7 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_28	(8 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_32	(9 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_48	(10 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_60	(11 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_64	(12 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_124	(13 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_128	(14 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_RDIL_200	(15 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_SWDH_BIT	14      /* source port width */
#define DMAC_DCMD_SWDH_MASK	(0x03 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_SWDH_32	(0 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_SWDH_8	(1 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_SWDH_16	(2 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_DWDH_BIT	12      /* dest port width */
#define DMAC_DCMD_DWDH_MASK	(0x03 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DWDH_32	(0 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DWDH_8	(1 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DWDH_16	(2 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DS_BIT	8       /* transfer data size of a data unit */
#define DMAC_DCMD_DS_MASK	(0x07 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_32BIT	(0 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_8BIT	(1 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_16BIT	(2 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_16BYTE	(3 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_DS_32BYTE	(4 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_TM		(1 << 7)        /* transfer mode: 0-single 1-block */
#define DMAC_DCMD_DES_V		(1 << 4)        /* descriptor valid flag */
#define DMAC_DCMD_DES_VM	(1 << 3)        /* descriptor valid mask: 1:support V-bit */
#define DMAC_DCMD_DES_VIE	(1 << 2)        /* DMA valid error interrupt enable */
#define DMAC_DCMD_TIE		(1 << 1)        /* DMA transfer interrupt enable */
#define DMAC_DCMD_LINK		(1 << 0)        /* descriptor link enable */

// DMA descriptor address register
#define DMAC_DDA_BASE_BIT	12      /* descriptor base address */
#define DMAC_DDA_BASE_MASK	(0x0fffff << DMAC_DDA_BASE_BIT)
#define DMAC_DDA_OFFSET_BIT	4       /* descriptor offset address */
#define DMAC_DDA_OFFSET_MASK	(0x0ff << DMAC_DDA_OFFSET_BIT)

// DMA control register
#define DMAC_DMACR_PR_BIT	8       /* channel priority mode */
#define DMAC_DMACR_PR_MASK	(0x03 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_012345	(0 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_023145	(1 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_201345	(2 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_PR_RR	(3 << DMAC_DMACR_PR_BIT)        /* round robin */
#define DMAC_DMACR_HLT		(1 << 3)        /* DMA halt flag */
#define DMAC_DMACR_AR		(1 << 2)        /* address error flag */
#define DMAC_DMACR_DMAE		(1 << 0)        /* DMA enable bit */

/*------------INT CONTROLLER------------------------*/
#define JZ4740_INT_BASE 0x10001000
#define JZ4740_INT_SIZE  0x1C

#define INTC_ISR	( 0x00)
#define INTC_IMR	( 0x04)
#define INTC_IMSR	( 0x08)
#define INTC_IMCR	( 0x0c)
#define INTC_IPR	( 0x10)

// 1st-level interrupts
#define IRQ_I2C		1
#define IRQ_UHC		3
#define IRQ_UART0	9
#define IRQ_SADC	12
#define IRQ_MSC		14
#define IRQ_RTC		15
#define IRQ_SSI		16
#define IRQ_CIM		17
#define IRQ_AIC		18
#define IRQ_ETH		19
#define IRQ_DMAC	20
#define IRQ_TCU2	21
#define IRQ_TCU1	22
#define IRQ_TCU0	23
#define IRQ_UDC 	24
#define IRQ_GPIO3	25
#define IRQ_GPIO2	26
#define IRQ_GPIO1	27
#define IRQ_GPIO0	28
#define IRQ_IPU		29
#define IRQ_LCD		30

// 2nd-level interrupts
#define IRQ_DMA_0	32      /* 32 to 37 for DMAC channel 0 to 5 */
#define IRQ_GPIO_0	48      /* 48 to 175 for GPIO pin 0 to 127 */

#define JZ4740_INT_INDEX_MAX 0x7        /*0x1C/4 */
#define JZ4740_INT_TO_MIPS   0x2        /*jz4740 intc will issue int 2 to mips cpu */

int jz4740_boot_from_nandflash (vm_instance_t * vm);
int jz4740_reset (vm_instance_t * vm);

#endif
