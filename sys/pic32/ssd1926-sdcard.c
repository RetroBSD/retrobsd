/*
 * SSD1926 hardware SD card driver
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
#include "ssd1926.h"

/*
 * Helper Macros
 */
#define CheckDataInhibit()      while(GetReg(0x1124) & 0x02)
#define CheckCommandInhibit()   while(GetReg(0x1124) & 0x01)
#define GetDataPortReg0()       GetReg(0x1120)
#define SetDataPortReg0(data)   SetReg(0x1120, data)
#define SetTransferMode(mode)   SetReg(0x110C, mode)

/*
 * Set graphics controller register (byte access)
 */
void SetReg(WORD index, BYTE value)
{
    DisplaySetCommand(); // set RS line to low for command
    DisplayEnable();     // enable SSD1926

    DeviceWrite(((WORD_VAL) index).v[1]);
    DeviceWrite(index << 8);

    DisplaySetData();    // set RS line to high for data

    if(index & 0x0001)
        DeviceWrite(value);
    else
        DeviceWrite(value << 8);

    DisplayDisable();   // disable SSD1926
}

/*
 * Read graphics controller register value (byte access)
 */
BYTE GetReg(WORD index)
{
    WORD    value;

    DisplaySetCommand(); // set RS line to low for command
    DisplayEnable();     // enable SSD1926

    DeviceWrite(((WORD_VAL) index).v[1]);
    DeviceWrite(index << 8);

    DisplaySetData();    // set RS line to high for data

    value = DeviceRead();
    value = DeviceRead();

    DisplayDisable();   // disable SSD1926

    if(index & 0x0001)
        value &= 0x00ff;
    else
        value = (value >> 8) & 0x00ff;

    return (value);
}

/*
 * Helper Functions
 */
inline void __attribute__ ((always_inline))
SetCommandArgument(DWORD arg)
{
    SetReg(0x110B, ((DWORD_VAL) arg).v[3]);
    SetReg(0x110A, ((DWORD_VAL) arg).v[2]);
    SetReg(0x1109, ((DWORD_VAL) arg).v[1]);
    SetReg(0x1108, ((DWORD_VAL) arg).v[0]);
}

inline void __attribute__ ((always_inline))
SetCommand(BYTE idx, BYTE flags)
{
    SetReg(0x110E, flags);  //set response type
    SetReg(0x110F, idx);    //command index
}

inline volatile DWORD __attribute__ ((always_inline))
GetCommandResponse(BYTE idx)
{
    DWORD_VAL   rsp;

    idx <<= 2;

    rsp.v[0] = GetReg(0x1110 + idx);
    rsp.v[1] = GetReg(0x1111 + idx);
    rsp.v[2] = GetReg(0x1112 + idx);
    rsp.v[3] = GetReg(0x1113 + idx);

    return (rsp.Val);
}

void ReadCmdResponse(DWORD *rsp, UINT size)
{
    DWORD   idx;
    for(idx = 0; idx < size; idx++)
    {
        *rsp = GetCommandResponse(idx);
        rsp++;
    }
}

/*
 * Global Variables
 */
DWORD               finalLBA;
WORD                sectorSize;
DWORD               maxBusClock;
BYTE                hcMode;
MEDIA_INFORMATION   mediaInformation;

/*
 * Local Prototypes
 */
BYTE                SDInit(void);
BYTE                SDSetClock(DWORD clockMax);
BYTE                SDSendCommand(BYTE cmd_idx, BYTE flags, DWORD arg);
BYTE                SDSendAppCommand(BYTE cmd_idx, BYTE flags, DWORD arg1, DWORD arg2);
void                SDReset(BYTE type);

/*
 * Function:
 *  BYTE SDSetClock(DWORD clockMax)
 * Input:
 *  clockMax - SD card maximum frequency,
 * Output:
 *  returns non-zero if the operation is successfull.
 * Overview:
 *  Sets the SD card clock frequency close to clockMax
 */
BYTE SDSetClock(DWORD clockMax)
{
    DWORD   mclk;
    BYTE    reg;
    BYTE    div;

    mclk = SSD_SYS_CLOCK * GetReg(SSD_REG_PLL_CONFIG1);
    mclk /= (GetReg(SSD_REG_PLL_CONFIG0) & 0x1F);
    mclk /= ((GetReg(SSD_REG_MCLK_CONIG) & 0x1F) + 1);

    if(GetReg(SSD_REG_MCLK_CONIG) & 0x01)
        mclk /= 2;

    reg = GetReg(SSD_SDCARD_REG_CLK_CNTL);

    reg = reg &~((BYTE) SSD_SD_CLK_ENABLE);
    SetReg(SSD_SDCARD_REG_CLK_CNTL, reg);

    reg |= SSD_SD_INT_CLK_ENABLE;
    SetReg(SSD_SDCARD_REG_CLK_CNTL, reg);

    DelayMs(1);
    if(!(GetReg(SSD_SDCARD_REG_CLK_CNTL) & SSD_SD_INT_CLK_STABLE))
        return (FALSE);

    div = 0;
    while(mclk > clockMax)
    {
        mclk >>= 1;
        div++;
    }

    div = 1 << (div - 1);

    SetReg(SSD_SDCARD_REG_CLK_DIV, div);

    reg |= SSD_SD_CLK_ENABLE;
    SetReg(SSD_SDCARD_REG_CLK_CNTL, reg);

    return (TRUE);
}

/*
 * Function:
 *  BYTE SDSendCommand(BYTE cmd_idx, BYTE flags, DWORD arg)
 * Input:
 *  cmd_idx - command start index,
 *  flags - command flags,
 *  arg - command arguments.
 * Output:
 *  Returns non-zero if the operation is successful
 * Overview:
 *  Send SD card command.
 */
BYTE SDSendCommand(BYTE cmd_idx, BYTE flags, DWORD arg)
{
    DWORD   timeout;

    SetReg(0x112E, 0x0E);   // Data Timeout Counter Value = MCLK x 2^27
    CheckCommandInhibit();

    // Clear interrupt flags
    SetReg(0x1130, 0xff);

    // Enable command complete interrupt
    SetReg(0x1134, GetReg(0x1134) | 0x01);

    // Clear error interrupt flags
    SetReg(0x1132, 0xff);
    SetReg(0x1133, 0xff);

    // Enable all interrupts
    SetReg(0x1136, 0xff);
    SetReg(0x1137, 0xff);

    SetCommandArgument(arg);
    SetCommand(cmd_idx, flags);

    timeout = SD_TIMEOUT;
    while(1)
    {
        if(GetReg(0x1130) & 0x01)
        {
            break;
        }

        if(!timeout--)
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

/*
 * Function:
 *  BYTE SDSendAppCommand(BYTE cmd_idx, BYTE flags, DWORD arg1, DWORD arg2)
 * Input:
 *  cmd_idx - command start index,
 *  flags - command flags,
 *  arg1 - application command arguments 1
 *  arg2 - application command arguments 2
 * Output:
 *  Returns non-zero if the operation is successful
 * Overview:
 *  Send SD card application command.
 */
BYTE SDSendAppCommand(BYTE cmd_idx, BYTE flags, DWORD arg1, DWORD arg2)
{
    if(!SDSendCommand(CMD_APP_CMD, SSD_RESPONSE_48, arg1))
        return (FALSE);

    if(!SDSendCommand(cmd_idx, flags, arg2))
        return (FALSE);

    return (TRUE);
}

/*
 * Function:
 *  void SDReset(BYTE type)
 * Input:
 *  type - type of reset.
 * Overview:
 *  Resets the SD card module
 */
void SDReset(BYTE type)
{
    DWORD   timeout;

    SetReg(SSD_SDCARD_REG_SW_RESET, type);

    timeout = SD_TIMEOUT;
    while(1)
    {
        if(!timeout--)
            return;
        if(!(GetReg(SSD_SDCARD_REG_SW_RESET) & type))
            return;
    }
}

/*
 * Function:
 *  void SDPower(BYTE on)
 * Input:
 *  on - '0' means off, '1' means on.
 * Overview:
 *  Powers on the SD card.
 */
void SDPower(BYTE on)
{
    if(on)
        SetReg(SSD_SDCARD_REG_PWR_CNTL, 0x0F);
    else
        SetReg(SSD_SDCARD_REG_PWR_CNTL, 0x0E);
}

/*
 * Function:
 *  BYTE SDDetect(void)
 * Output:
 *  TRUE - card detected,
 *  FALSE - no card detected.
 * Overview:
 *  Detects if the SD card is inserted.
 */
BYTE SDDetect(void)
{
    BYTE    reg;

    reg = GetReg(SSD_SDCARD_REG_PRSNT_STATE_2);

    if(!(reg & SSD_CARD_DETECT))
        return (FALSE);

    if(reg & SSD_CARD_INSERTED)
    {
        DWORD   timeout;

        timeout = SD_TIMEOUT;
        while(1)
        {
            if(!timeout--)
                return (FALSE);
            if(GetReg(SSD_SDCARD_REG_PRSNT_STATE_2) & SSD_CARD_STABLE)
                break;
        }
    }
    else
        return (FALSE);

    return (TRUE);
}

/*
 * Function:
 *  BYTE SDWriteProtectState(void)
 * Output:
 *  Returns the status of the "write enabled" pin.
 * Overview:
 *  Determines if the card is write-protected.
 */
BYTE SDWriteProtectState(void)
{
    return (GetReg(SSD_SDCARD_REG_PRSNT_STATE_2) & SSD_WRITE_PROTECT);
}

/*
 * Function:
 *  BYTE SDInit(void)
 * Output:
 *  Returns TRUE if media is initialized, FALSE otherwise.
 * Overview:
 *  MediaInitialize initializes the media card and supporting variables.
 */
BYTE SDInit(void)
{
    DWORD   timeout;
    DWORD   response;
    DWORD   voltage;
    DWORD   CSD_C_SIZE;
    DWORD   CSD_C_SIZE_MULT;
    DWORD   CSD_RD_BL_LEN;
    DWORD   RCA;
    DWORD   CSD[4];

    if(!SDSendCommand(CMD_RESET, SSD_NO_RESPONSE, 0xFFFFFFFF))
        return (FALSE);


    if(!SDSendCommand(CMD_SEND_IF_COND, SSD_RESPONSE_48 | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, 0x000001AA))
        return (FALSE);

    response = GetCommandResponse(0);

    if(response == 0x000001AA)
    {
        voltage = 0x40200000;
        hcMode = 1;
    }else{
        voltage = 0x00200000;
        hcMode = 0;
    }

    timeout = 10;
    do
    {
        if(!timeout--)
        {
            return (FALSE);
        }

        SDReset(SSD_RESET_DATA|SSD_RESET_CMD);
        if(!SDSendAppCommand(ACMD_SEND_OCR, SSD_RESPONSE_48, 0, voltage))
            return (FALSE); // ask card send OCR back
        DelayMs(150);
        response = GetCommandResponse(0);
    } while((response&0x80000000) == 0);

    if((response&0x40000000L) == 0){
        hcMode = 0;
    }

    if(!SDSendCommand(CMD_SEND_ALL_CID, SSD_RESPONSE_136 | SSD_CMD_CRC_CHK, 0xFFFFFFFF))
        return (FALSE);

    DelayMs(150);

    if(!SDSendCommand(CMD_SEND_RCA, SSD_RESPONSE_48 | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, 0xFFFFFFFF))
        return (FALSE);

    RCA = GetCommandResponse(0);
    RCA |= 0x0000ffff;

    //CMD9 - Request CSD from SD Card
    if(!SDSendCommand(CMD_SEND_CSD, SSD_CMD_IDX_CHK | SSD_CMD_CRC_CHK | SSD_RESPONSE_136, RCA))
        return (FALSE);

    ReadCmdResponse(CSD, 4);

    CSD_C_SIZE = ((CSD[2] & 0x00000003) << 10) | ((CSD[1] & 0xFFC00000) >> 22); //CSD Bit[73:62] => Bit[65:54] (Deduct CRC-7 as CRC-7 is not stored in response register)
    CSD_C_SIZE_MULT = ((CSD[1] & 0x00000380) >> 7);                             //CSD Bit[49:47] =? Bit[41:39]
    CSD_RD_BL_LEN = (CSD[2] & 0x00000F00) >> 8; //CSD Bit[83:80] -> Bit[75:72]
    finalLBA = (CSD_C_SIZE + 1) * (1 << (CSD_C_SIZE_MULT + 2));
    sectorSize = (1 << CSD_RD_BL_LEN);
    if((CSD[3] & 0x000000FF) == 0x32)
        maxBusClock = 25000000;                 // 25MHz
    else
        maxBusClock = 25000000;                 // 50MHz
    SDReset(SSD_RESET_CMD);

    //CMD7 Select Card
    if(!SDSendCommand(CMD_SELECT_CARD, SSD_RESPONSE_48_BUSY | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, RCA))
        return (FALSE);

    sectorSize = 512L;

    //CMD16 Set the block size.
    if(!SDSendCommand(CMD_SET_BLKLEN, SSD_RESPONSE_48 | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, sectorSize))
        return (FALSE);

    //CMD55 + ACMD6 Set bus width = 4 Bit
    if(!SDSendAppCommand(ACMD_SET_BUS_WIDTH, SSD_RESPONSE_48 | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, RCA, 0xFFFFFFFE))
        return (FALSE);

    //Data width 4bits
    SetReg(0x1128, 0x02);

    return (TRUE);
}

/*
 * Function:
 *  INT32 SDGetStatus(void)
 * Output:
 *  SD card status.
 * Overview:
 *  Returns SD card status. If the returned value is -1 the there's an error.
 */
INT32 SDGetStatus(void)
{
    DWORD   status;
    DWORD   RCA;

    SDReset(SSD_RESET_CMD);

    //Normal interrupt handling
    SetReg(0x1134, GetReg(0x1134) | 0x01);  //enable read interrupt
    SetReg(0x1130, 0x01);                   //clear previous interrupt

    //Error interrupt handling
    SetReg(0x1136, GetReg(0x1136) | 0x8F);  //enable error interrupt
    SetReg(0x1137, GetReg(0x1137) | 0x71);  //enable error interrupt
    SetReg(0x1132, 0x8F);                   //enable error interrupt
    SetReg(0x1133, 0x71);                   //enable error interrupt
    if(!SDSendCommand(CMD_SEND_RCA, SSD_RESPONSE_48 | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, 0xFFFFFFFF))
        return (DWORD) (-1);

    RCA = GetCommandResponse(0);
    RCA |= 0x0000ffff;

    if(!SDSendCommand(CMD_SEND_STATUS, SSD_RESPONSE_48 | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK, RCA))
        return (DWORD) (-1);

    while(!(GetReg(0x1130) & 0x01));

    //check cmd complt int
    SetReg(0x1130, 0x01);                   //clear previous int
    while((GetReg(0x1132) & 0x8F) || (GetReg(0x1133) & 0x71));
    SetReg(0x1132, 0x8F);                   //enable error interrupt
    SetReg(0x1133, 0x71);                   //enable error interrupt
    ReadCmdResponse(&status, 1);

    return (status);
}

/*
 * Function:
 *  MEDIA_INFORMATION* SDInitialize(void)
 * Output:
 *  Returns a pointer to the media inforamtion structure.
 * Overview:
 *  SDInitialize initializes the media card and supporting variables.
 *  Sets maximum clock frequency for the SD card.
 */
MEDIA_INFORMATION *SDInitialize(void)
{
    DWORD   timeout;

    SDReset(SSD_RESET_DATA | SSD_RESET_CMD | SSD_RESET_ALL);

    SDPower(TRUE);

    mediaInformation.errorCode = MEDIA_NO_ERROR;
    mediaInformation.validityFlags.value = 0;

    if(!SDSetClock(SSD_SD_CLK_INIT))
    {
        mediaInformation.errorCode = MEDIA_DEVICE_NOT_PRESENT;
        return (&mediaInformation);
    }

    timeout = 10;
    while(!SDInit())
    {
        if(!timeout--)
        {
            mediaInformation.errorCode = MEDIA_DEVICE_NOT_PRESENT;
            return (&mediaInformation);
        }
    }

    mediaInformation.validityFlags.bits.sectorSize = 1;
    mediaInformation.sectorSize = sectorSize;

    if(!SDSetClock(maxBusClock>>1))
        mediaInformation.errorCode = MEDIA_DEVICE_NOT_PRESENT;

    return (&mediaInformation);
}

/*
 * Function:        BYTE SDSectorRead(DWORD sector_addr, BYTE *buffer)
 *
 * Input:           sector_addr - Sector address
 *                  buffer      - Buffer where data will be stored, see
 *                                'ram_acs.h' for 'block' definition.
 *                                'Block' is dependent on whether internal or
 *                                external memory is used
 *
 * Output:          Returns TRUE if read successful, false otherwise
 *
 * Overview:        SectorRead reads data from the card starting
 *                  at the sector address specified by sector_addr and stores
 *                  them in the location pointed to by 'buffer'.
 *
 * Note:            The device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address.
 */
BYTE SDSectorRead(DWORD sector_addr, BYTE *buffer)
{
    if(!hcMode)
    {
        sector_addr *= sectorSize;
    }

    CheckDataInhibit();

    SDReset(SSD_RESET_DATA);

    SetTransferMode(0x10);

    // Block size is one sector
    SetReg(0x1104, ((WORD_VAL) sectorSize).v[0]);   // write block size
    SetReg(0x1105, ((WORD_VAL) sectorSize).v[1]);   // write block size

    //Clear error interrupt flags
    SetReg(0x1136, 0xff);
    SetReg(0x1137, 0xff);
    SetReg(0x1132, 0xff);
    SetReg(0x1133, 0xff);

    // Buffer Read Ready, Transfer Complete, Command Complete interrupts enable
    SetReg(0x1134, 0x23);

    // Clear previous interrupt flags
    SetReg(0x1130, 0x23);

    // Send command
    if
    (
        !SDSendCommand
            (
                CMD_RD_SINGLE,
                SSD_RESPONSE_48 | SSD_DATA_PRESENT | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK,
                sector_addr
            )
    ) return (FALSE);

    // Wait for transfer complete
    while(!(GetReg(0x1130) & 0x20));

    // Clear transfer complete flag
    SetReg(0x1130, 0x20);

    // Read data
    WORD    counter = 0;
    while(GetReg(0x1125) & 0x08)
    {
        if(counter < sectorSize)
            *buffer++ = GetDataPortReg0();
        counter++;
    }

    if(counter > sectorSize)
        return (FALSE);

    return (TRUE);
}

/*
 * Function:
 *  BYTE SDSectorDMARead(DWORD sector_addr,  DWORD dma_addr, UINT16 num_blk)
 * Input:
 *  sector_addr - Sector address
 *  buffer      - Buffer where data will be stored, see
 *                'ram_acs.h' for 'block' definition.
 *                'Block' is dependent on whether internal or
 *                external memory is used
 *
 * Output:
 *  Returns TRUE if read successful, false otherwise.
 * Overview:        SectorRead reads data from the card starting
 *                  at the sector address specified by sector_addr and stores
 *                  them in the location pointed to by 'buffer'.
 *
 * Note:            The device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address.
 */
BYTE SDSectorDMARead(DWORD sector_addr, DWORD dma_addr, UINT16 num_blk)
{
    DWORD   dma_size;
    BYTE    boundary;

    if(!hcMode)
    {
        sector_addr *= sectorSize;
    }

    CheckDataInhibit();
    SDReset(SSD_RESET_DATA);

    // set up the DMA address
    SetReg(0x1100, ((DWORD_VAL) dma_addr).v[0]);
    SetReg(0x1101, ((DWORD_VAL) dma_addr).v[1]);
    SetReg(0x1102, ((DWORD_VAL) dma_addr).v[2]);
    SetReg(0x1103, ((DWORD_VAL) dma_addr).v[3]);

    // set up the transefer mode (Multi-Blk, Read, Blk Cnt, DMA)
    SetTransferMode(0x33);

    // set up the DMA buffer size (4k) and sector block size
    dma_size = (DWORD) num_blk * sectorSize;

    dma_size >>= 12;
    boundary = 0;
    while(dma_size)
    {
        dma_size >>= 1;
        boundary++;
    }

    boundary <<= 4;
    boundary |= ((WORD_VAL) sectorSize).v[1] & 0x0F;

    SetReg(0x1104, ((WORD_VAL) sectorSize).v[0]);   // write block size
    SetReg(0x1105, boundary);                       // write block size

    // Set the number of blocks to read
    SetReg(0x1106, (BYTE) (num_blk & 0xFF));        // write block size
    SetReg(0x1107, (BYTE) (num_blk >> 8));          // write block size

    // set up the normal status register 0
    SetReg(0x1134, 0x0B);                           // enable read interrupt
    SetReg(0x1130, GetReg(0x1130) | 0x0B);          // clear previous interrupt

    // set up the error status register
    SetReg(0x1136, 0xFF);                           // clear previous interrupt
    SetReg(0x1132, 0xFF);                           // clear previous interrupt
    SetReg(0x1133, 0x01);                           // clear previous interrupt
    SetReg(0x1130, 0x08);                           // clear previous interrupt

    // set the command to read multiple blocks
    if
    (
        !SDSendCommand
            (
                CMD_RD_MULTIPLE,
                SSD_RESPONSE_48 | SSD_DATA_PRESENT | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK,
                sector_addr
            )
    ) return (FALSE);

    // wait until the dma transfer is done
    while(!(GetReg(0x1130) & 0x08) && (GetReg(0x1125) & 0x02));
    SetReg(0x1130, 0x08);

    // stop the transmission
    if
    (
        !SDSendCommand
            (
                CMD_STOP_TRANSMISSION,
                SSD_CMD_TYPE_ABORT | SSD_RESPONSE_48_BUSY | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK,
                0xFFFFFFFF
            )
    ) return (FALSE);
    SetReg(0x1130, 0x01);                           //clear previous interrupt
    SetReg(0x1106, 0);                              // write block size
    SetReg(0x1107, 0);                              // write block size

    // disable the transfer mode
    SetTransferMode(0);

    return (TRUE);
}

/*
 * Function:        BYTE SDSectorWrite(DWORD sector_addr, BYTE *buffer, BYTE allowWriteToZero)
 *
 * Input:           sector_addr - Sector address
 *                  buffer      - Buffer where data will be read
 *                  allowWriteToZero - If true, writes to the MBR will be valid
 *
 * Output:          Returns TRUE if write successful, FALSE otherwise
 *
 * Overview:        SectorWrite sends data from the location
 *                  pointed to by 'buffer' to the card starting
 *                  at the sector address specified by sector_addr.
 *
 * Note:            The sample device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address.
 */
BYTE SDSectorWrite(DWORD sector_addr, BYTE *buffer, BYTE allowWriteToZero)
{
    UINT16  i;

    if(!hcMode)
    {
        sector_addr *= sectorSize;
    }

    CheckDataInhibit();
    SDReset(SSD_RESET_DATA);
    SetTransferMode(0);

    SetReg(0x1104, ((WORD_VAL) sectorSize).v[0]);   // write block size
    SetReg(0x1105, ((WORD_VAL) sectorSize).v[1]);   // write block size

    //Clear error interrupt flags
    SetReg(0x1136, 0xff);
    SetReg(0x1137, 0xff);
    SetReg(0x1132, 0xff);
    SetReg(0x1133, 0xff);

    //Normal interrupt handling
    SetReg(0x1134, 0x13);          //enable write interrupt
    SetReg(0x1130, 0x13);          //clear all type of normal interrupt

    if
    (
        !SDSendCommand
            (
                CMD_WR_SINGLE,
                SSD_RESPONSE_48 | SSD_DATA_PRESENT | SSD_CMD_CRC_CHK | SSD_CMD_IDX_CHK,
                sector_addr
            )
    ) return (FALSE);

    SetReg(0x1130, 0x01);           //clear previous interrupt
    while(!(GetReg(0x1130)&0x10) && !(GetReg(0x1125) & 0x04));  //check Buffer write int
    SetReg(0x1130, 0x10);           //clear previous interrupt

    for(i = 0; i < sectorSize; i++)
    {
        //check if Write Enable bit set, which mean buffer is free for new data
        while(!(GetReg(0x1125) & 0x04));
        //put data that need be written into SSD1928 buffers
        SetDataPortReg0(*buffer++);
    }

    SetReg(0x1130, 0x10);           //clear previous interrupt
    while(!(GetReg(0x1130)&0x02));  //check Transfer complt int
    SetReg(0x1130, 0x02);           // clear previous interrupt

    return (TRUE);
}

/*
 * Function:
 *  WORD SDReadSectorSize(void)
 * PreCondition:
 *  MediaInitialize() is complete
 * Output:
 *  Size of the sectors for this physical media.
 * Overview:
 *  Returns size of the sectors for this physical media.
 */
WORD SDReadSectorSize(void)
{
    return (sectorSize);
}

/*
 * Function:
 *  DWORD SDReadCapacity(void)
 * PreCondition:
 *  MediaInitialize() is complete
 * Output:
 *  Number of sectors.
 * Overview:
 *  Returns number of sectors.
 */
DWORD SDReadCapacity(void)
{
    return (finalLBA);
}
