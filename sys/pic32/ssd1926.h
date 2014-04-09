/*
 * SSD1926 hardware definitions
 *
 * Copyright (C) 2008 Microchip Technology Inc.  All rights reserved.
 * Microchip licenses to you the right to use, modify, copy and distribute
 * Software only when embedded on a Microchip microcontroller or digital
 * signal controller, which is integrated into your product or third party
 * product (pursuant to the sublicense terms in the accompanying license
 * agreement).
 *
 * You should refer to the license agreement accompanying this Software
 * for additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED 'AS IS' WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR
 * OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,
 * BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL,
 * INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA,
 * COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY
 * CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF),
 * OR OTHER SIMILAR COSTS.
 *
 * Author                 Date           Comments
 * -----------------------------------------------------
 * Sean Justice        15_Sept-2008      First release
 * Anton Alkhimenok    06_Jun-2009       Ported to PIC24
 */
#ifndef _SSD1926_H

#define _SSD1926_H

/*
 * User Defines
 */
#define SSD_SYS_CLOCK   (DWORD) (4000000)   /* SSD1926 crystal frequncy */
#define SSD_SD_CLK_INIT (DWORD) (400000)
#define SD_TIMEOUT      (DWORD) (3000000)

/*
 * Registers
 */
#define SSD_REG_PLL_CONFIG0                 0x0126
#define SSD_REG_PLL_CONFIG1                 0x0127
#define SSD_REG_MCLK_CONIG                  0x0004
#define SSD_SDCARD_SD_CLK                   0x1001
#define SSD_SDCARD_REG_DMA_ADDR             0x1100
#define SSD_SDCARD_REG_BLK_SIZE             0x1104
#define SSD_SDCARD_REG_BLK_CNT              0x1106
#define SSD_SDCARD_REG_ARG_32BIT            0x1108
#define SSD_SDCARD_REG_XFR_MODE             0x110c
#define SSD_SDCARD_REG_CMD                  0x110e
#define SSD_SDCARD_REG_RSP                  0x1110
#define SSD_SDCARD_REG_DATA_PORT            0x1120
#define SSD_SDCARD_REG_RSVD1                0x1121
#define SSD_SDCARD_REG_PRSNT_STATE_0        0x1124
#define SSD_SDCARD_REG_PRSNT_STATE_1        0x1125
#define SSD_SDCARD_REG_PRSNT_STATE_2        0x1126
#define SSD_SDCARD_REG_PRSNT_STATE_3        0x1127
#define SSD_SDCARD_REG_HST_CNTL             0x1128
#define SSD_SDCARD_REG_PWR_CNTL             0x1129
#define SSD_SDCARD_REG_BLK_GAP_CNTL         0x112a
#define SSD_SDCARD_REG_WKUP_CNTL            0x112b
#define SSD_SDCARD_REG_CLK_CNTL             0x112c
#define SSD_SDCARD_REG_CLK_DIV              0x112d
#define SSD_SDCARD_REG_TOUT_CNTL            0x112e
#define SSD_SDCARD_REG_SW_RESET             0x112f
#define SSD_SDCARD_REG_NRM_INTR_STATUS      0x1130
#define SSD_SDCARD_REG_ERR_INTR_STATUS      0x1132
#define SSD_SDCARD_REG_NRM_INTR_STATUS_EN   0x1134
#define SSD_SDCARD_REG_ERR_INTR_STATUS_EN   0x1136
#define SSD_SDCARD_REG_NRM_INTR_SIG_EN      0x1138
#define SSD_SDCARD_REG_ERR_INTR_SIG_EN      0x113a
#define SSD_SDCARD_REG_ACMD12_ERR_STATUS    0x113c
#define SSD_SDCARD_REG_RSVD2                0x113e
#define SSD_SDCARD_REG_CAPABILITIES         0x1140
#define SSD_SDCARD_REG_CAP_RSVD             0x1144
#define SSD_SDCARD_REG_MAX_CURR_CAP         0x1148
#define SSD_SDCARD_REG_MAX_CURR_CAP_RSVD    0x114c
#define SSD_SDCARD_REG_RSVD3                0x1150
#define SSD_SDCARD_REG_SLOT_INTR_STATUS     0x11fc
#define SSD_SDCARD_REG_HCVER                0x11fe

/*
 * SD Commands
 */
#define CMD_RESET               0
#define CMD_SEND_OCR            1           // used exclusively in MMC
#define CMD_SEND_ALL_CID        2           // R2: R136
#define CMD_SEND_RCA            3           // R1 (MMC) or R6(SDMEM)
#define CMD_SET_DSR             4
#define CMD_IO_SEND_OCR         5           // R4, unique to IO cards
#define CMD_SELECT_CARD         7           // R1, arg=rca[31..16] or 0
#define CMD_SEND_IF_COND	8
#define CMD_SEND_CSD            9           // R2: R136
#define CMD_SEND_CID            10          // R2: R136
#define CMD_STOP_TRANSMISSION   12          // R1b: arg=stuff bits
#define CMD_SEND_STATUS         13          // R1
#define CMD_GO_INACTIVE         15          // None, arg=rca[31..16], stuff[15..0]
#define CMD_SET_BLKLEN          16          // R1, arg=block len[31..0]
#define CMD_RD_SINGLE           17          // R1, arg=block address[31..0]
#define CMD_RD_MULTIPLE         18          // R1, arg=block address[31..0]
#define CMD_WR_SINGLE           24          // R1, arg=block address[31..0]
#define CMD_WR_MULTIPLE         25          // R1, arg=block address[31..0]
#define CMD_SET_WP              28          // R1b, arg=wp address[31..0]
#define CMD_CLR_WP              29          // R1b, arg=wp address[31..0]
#define CMD_SEND_WP             30          // R1, DATA, arg=wp address[31..0]
#define CMD_ERASE_SADDR         32          // R1, arg=block address[31..0]
#define CMD_ERASE_EADDR         33          // R1, arg=block address[31..0]
#define CMD_ERASE_GRP_SADDR     35          // R1, arg=block address[31..0]
#define CMD_ERASE_GRP_EADDR     36          // R1, arg=block address[31..0]
#define CMD_ERASE               38          // R1b, arg=stuff bits[31..0]
#define CMD_IO_RW_DIRECT        52          // R5
#define CMD_IO_RW_EXTENDED      53          // R1, data transfer
#define CMD_APP_CMD             55          // R1, arg=rca[31..16], stuff[15..0]
#define CMD_GEN_CMD             56          // R1, data, arg=stuff[31..1], RD/WR[0]
#define ACMD_SET_BUS_WIDTH      6           // R1, arg=[1..0] = bus width, [31:2] stuff bits
#define ACMD_SEND_STATUS        13          // R1, DATA, arg=stuff bits [31..0]
#define ACMD_SEND_NUM_WR_BLK    22          // R1, DATA, arg=stuff bits [31..0]
#define ACMD_SEND_OCR           41
#define ACMD_SEND_SCR           51          // R1, arg=stuff bits[31..0]

/*
 * Flags
 */
#define SSD_SD_CLK_CTRL_ON      (DWORD) 0x80000000
#define SSD_SD_CLK_ENABLE       (DWORD) 0x00000004
#define SSD_SD_INT_CLK_STABLE   (DWORD) 0x00000002
#define SSD_SD_INT_CLK_ENABLE   (DWORD) 0x00000001
#define SSD_SD_CLK_FLAGS        (SSD_SD_CLK_CTRL_ON | SSD_SD_CLK_ENABLE | SSD_SD_INT_CLK_ENABLE)
#define SSD_CMD_TYPE_ABORT      0xC0
#define SDD_CMD_TYPE_RESUME     0x80
#define SDD_CMD_TYPE_SUSPEND    0x40
#define SDD_CMD_TYPE_NORMAL     0x00
#define SSD_DATA_PRESENT        0x20
#define SSD_CMD_IDX_CHK         0x10
#define SSD_CMD_CRC_CHK         0x08
#define SSD_NO_RESPONSE         0x00
#define SSD_RESPONSE_136        0x01
#define SSD_RESPONSE_48         0x02
#define SSD_RESPONSE_48_BUSY    0x03

#define SSD_CARD_DETECT         0x04
#define SSD_CARD_STABLE         0x02
#define SSD_CARD_INSERTED       0x01

#define SSD_WRITE_PROTECT       0x08

#define SSD_RESET_ALL           0x01
#define SSD_RESET_CMD           0x02
#define SSD_RESET_DATA          0x04

#define WAIT_CNT                (DWORD) 10000000l

#endif /* _SSD1926_H */
