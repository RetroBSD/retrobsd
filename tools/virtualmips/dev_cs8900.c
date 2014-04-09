 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

 /*
  * cs8900 net card emulation.
  * (jz4740 driver).
  * Only works in linux 2.6.24/2.6.22/2.4.20
  * uboot can not use it.
  *
  * Please use TCP instead of UDP when using NFS.
  * Throughput is about 50k-100k bytes per second when downloading a file from host using http.
  * Maybe improved when JIT is implemented in the future.
  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include "crc.h"
#include "utils.h"
#include "cpu.h"
#include "vm.h"
#include "mips_memory.h"
#include "device.h"
#include "net.h"
#include "net_io.h"
#include "dev_cs8900.h"

/*#define QUEUE_SIZE  128
#define PACKET_LEN   1600
m_uint8_t recv_buffer[QUEUE_SIZE][PACKET_LEN];
m_uint8_t packet_len[QUEUE_SIZE];

m_uint8_t read_index=0;
m_uint8_t write_index=0;
*/

/*00:62:9c:61:cf:16*/
static uint8_t cs8900a_default_mac[6] =
    { 0x00, 0x62, 0x9c, 0x61, 0xcf, 0x16 };

#define CS8900_DEFAULT_RX_TIMEOUT  40
#define CS8900_MIN_RX_TIMEOUT  20
#define CS8900_MAX_RX_TIMEOUT  100
#define CS8900_RX_TIMEOUT_STEP 5

static m_uint32_t cs8900a_rx_timeout = CS8900_DEFAULT_RX_TIMEOUT;

/* Maximum packet size */
#define CS8900_MAX_PKT_SIZE     1518
#define CS8900_MIN_PKT_SIZE     8
#define CS8900_RUN_PKT_SIZE     64

#define CS8900A_PRODUCT_ID  0x630e      /*little endian */

#define PP_RT_DATA0              0x00
#define PP_RT_DATA1              0x02
#define PP_TX_CMD              0X04
#define PP_TX_LEN              0X06
#define PP_IO_ISQ              0X08
#define PP_ADDRESS              0x0a    /* PacketPage Pointer Port (Section 4.10.10) */
#define PP_DATA0                 0x0c   /* PacketPage Data Port (Section 4.10.10) */
#define PP_DATA1               0X0e

#define PP_ProductID            0x0000  /* Section 4.3.1   Product Identification Code */
#define PP_ISAIOB 					0x0020  /*  IO base address */
#define PP_IntNum                       0x0022  /* Section 3.2.3   Interrupt Number */
#define PP_ISASOF						 0x0026 /*  ISA DMA offset */
#define PP_DmaFrameCnt 				0x0028  /*  ISA DMA Frame count */
#define PP_DmaByteCnt 					0x002A  /*  ISA DMA Byte count */
#define PP_MemBase                      0x002c  /* Section 4.9.2   Memory Base Address Register */
#define PP_EEPROMCommand        0x0040  /* Section 4.3.11  EEPROM Command */
#define PP_EEPROMData           0x0042  /* Section 4.3.12  EEPROM Data */

#define PP_RxCFG                        0x0102  /* Section 4.4.6   Receiver Configuration */
#define PP_RxCTL                        0x0104  /* Section 4.4.8   Receiver Control */
#define PP_TxCFG                        0x0106  /* Section 4.4.9   Transmit Configuration */
#define PP_BufCFG                       0x010a  /* Section 4.4.12  Buffer Configuration */
#define PP_LineCTL                      0x0112  /* Section 4.4.16  Line Control */
#define PP_SelfCTL                      0x0114  /* Section 4.4.18  Self Control */
#define PP_BusCTL                       0x0116  /* Section 4.4.20  Bus Control */
#define PP_TestCTL                      0x0118  /* Section 4.4.22  Test Control */
#define PP_AutoNegCTL 				0x011C  /*  Auto Negotiation Ctrl */
#define PP_ISQ                            0x0120        /* Section 4.4.5   Interrupt Status Queue */
#define PP_RxEvent 							0x0124  /*  Rx Event Register */
#define PP_TxEvent                      0x0128  /* Section 4.4.10  Transmitter Event */
#define PP_BufEvent                     0x012c  /* Section 4.4.13  Buffer Event */
#define PP_RxMISS                       0x0130  /* Section 4.4.14  Receiver Miss Counter */
#define PP_TxCOL                        0x0132  /* Section 4.4.15  Transmit Collision Counter */
#define PP_LineST							 0x0134 /*  Line State Register */
#define PP_SelfST                       0x0136  /* Section 4.4.19  Self Status */
#define PP_BusST                        0x0138  /* Section 4.4.21  Bus Status */
#define PP_TDR 								0x013C  /*  Time Domain Reflectometry */
#define PP_AutoNegST 				0x013E  /*  Auto Neg Status */
#define PP_TxCMD                        0x0144  /* Section 4.4.11  Transmit Command */
#define PP_TxLength                     0x0146  /* Section 4.5.2   Transmit Length */
#define PP_LAF								 0x0150 /*  Hash Table */
#define PP_IA                           	 0x0158 /* Section 4.6.2   Individual Address (IEEE Address) */

#define PP_RxStatus                     0x0400  /* Section 4.7.1   Receive Status */
#define PP_RxLength                     0x0402  /* Section 4.7.1   Receive Length (in bytes) */
#define PP_RxFrame                      0x0404  /* Section 4.7.2   Receive Frame Location */
#define PP_TxFrame                      0x0a00  /* Section 4.7.2   Transmit Frame Location */

/* PP_RxCFG */
#define Skip_1                  0x0040
#define StreamE                 0x0080
#define RxOKiE                  0x0100
#define RxDMAonly               0x0200
#define AutoRxDMAE              0x0400
#define BufferCRC               0x0800
#define CRCerroriE              0x1000
#define RuntiE                  0x2000
#define ExtradataiE             0x4000

 /* PP_TxCFG */
#define Loss_of_CRSiE   0x0040
#define SQErroriE               0x0080
#define TxOKiE                  0x0100
#define Out_of_windowiE 0x0200
#define JabberiE                0x0400
#define AnycolliE               0x0800
#define T16colliE               0x8000

/* PP_BufCFG */
#define SWint_X                 0x0040
#define RxDMAiE                 0x0080
#define Rdy4TxiE                0x0100
#define TxUnderruniE    0x0200
#define RxMissiE                0x0400
#define Rx128iE                 0x0800
#define TxColOvfiE              0x1000
#define MissOvfloiE             0x2000
#define RxDestiE                0x8000

 /* PP_RxCTL */
#define IAHashA                 0x0040
#define PromiscuousA    0x0080
#define RxOKA                   0x0100
#define MulticastA              0x0200
#define IndividualA             0x0400
#define BroadcastA              0x0800
#define CRCerrorA               0x1000
#define RuntA                   0x2000
#define ExtradataA              0x4000

 /* PP_SelfCTL */
#define RESET                   0x0040
#define SWSuspend               0x0100
#define HWSleepE                0x0200
#define HWStandbyE              0x0400
#define HC0E                    0x1000
#define HC1E                    0x2000
#define HCB0                    0x4000
#define HCB1                    0x8000

/* PP_LineCTL */
#define SerRxON                 0x0040
#define SerTxON                 0x0080
#define AUIonly                 0x0100
#define AutoAUI_10BT    0x0200
#define ModBackoffE             0x0800
#define PolarityDis             0x1000
#define L2_partDefDis   0x2000
#define LoRxSquelch             0x4000

/* PP_TxEvent */
#define Loss_of_CRS             0x0040
#define SQEerror                0x0080
#define TxOK                    0x0100
#define Out_of_window   0x0200
#define Jabber                  0x0400
#define T16coll                 0x8000

#define RxEvent                 0x0004
#define TxEvent                 0x0008
#define BufEvent                0x000c
#define RxMISS                  0x0010
#define TxCOL                   0x0012

/* PP_BufEvent */
#define SWint                   0x0040
#define RxDMAFrame              0x0080
#define Rdy4Tx                  0x0100
#define TxUnderrun              0x0200
#define RxMiss                  0x0400
#define Rx128                   0x0800
#define RxDest                  0x8000

 /* PP_TxCMD */
#define After5                  0
#define After381                1
#define After1021               2
#define AfterAll                3
#define TxStart(x) ((x) << 6)

#define Force                   0x0100
#define Onecoll                 0x0200
#define InhibitCRC              0x1000
#define TxPadDis                0x2000

 /* PP_BusST */
#define TxBidErr                0x0080
#define Rdy4TxNOW               0x0100

extern cpu_mips_t *current_cpu;
static void dev_cs8900_gen_interrupt (struct cs8900_data *d)
{
    vm_instance_t *vm;
    vm = d->vm;

    /*must check RQ bit in 0x116 */
    m_uint8_t *ram_base;
    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));
    if ((*(m_uint16_t *) (ram_base + PP_BusCTL)) & (0x8000)) {
        /*generate IRQ */
        vm->set_irq (vm, d->irq_no);
    }
}

static void dev_cs8900_init_defaultvalue (struct cs8900_data *d)
{
    m_uint8_t *ram_base;

    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));

    *(m_uint32_t *) (ram_base + PP_ProductID) = CS8900A_PRODUCT_ID;
    *(m_uint16_t *) (ram_base + PP_ISAIOB) = 0x300;
    *(m_uint16_t *) (ram_base + PP_IntNum) = 0x4;
    *(m_uint16_t *) (ram_base + PP_IntNum) = 0x4;

    *(m_uint16_t *) (ram_base + PP_RxCFG) = 0x3;
    *(m_uint16_t *) (ram_base + PP_RxEvent) = 0x4;

    *(m_uint16_t *) (ram_base + PP_RxCTL) = 0x5;
    *(m_uint16_t *) (ram_base + PP_TxCFG) = 0x7;
    *(m_uint16_t *) (ram_base + PP_TxEvent) = 0x8;
    *(m_uint16_t *) (ram_base + 0x108) = 0x9;

    *(m_uint16_t *) (ram_base + PP_BufCFG) = 0xb;
    *(m_uint16_t *) (ram_base + PP_BufEvent) = 0xc;

    *(m_uint16_t *) (ram_base + PP_RxMISS) = 0x10;

    *(m_uint16_t *) (ram_base + PP_TxCOL) = 0x12;
    *(m_uint16_t *) (ram_base + PP_LineCTL) = 0x13;
    *(m_uint16_t *) (ram_base + PP_LineST) = 0x14;
    *(m_uint16_t *) (ram_base + PP_SelfCTL) = 0x15;

    *(m_uint16_t *) (ram_base + PP_SelfST) = 0x16;
    *(m_uint16_t *) (ram_base + PP_BusCTL) = 0x17;

    *(m_uint16_t *) (ram_base + PP_BusST) = 0x18;
    *(m_uint16_t *) (ram_base + PP_TestCTL) = 0x19;

    *(m_uint16_t *) (ram_base + PP_TDR) = 0x1c;

    *(m_uint16_t *) (ram_base + PP_TxCMD) = 0x9;

    *(ram_base + PP_IA) = cs8900a_default_mac[0];
    *(ram_base + PP_IA + 1) = cs8900a_default_mac[1];
    *(ram_base + PP_IA + 2) = cs8900a_default_mac[2];
    *(ram_base + PP_IA + 3) = cs8900a_default_mac[3];
    *(ram_base + PP_IA + 4) = cs8900a_default_mac[4];
    *(ram_base + PP_IA + 5) = cs8900a_default_mac[5];

}

static void dev_cs8900_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    struct cs8900_data *d = dev->priv_data;
    memset (d->internal_ram, 0, sizeof (d->internal_ram));
    dev_cs8900_init_defaultvalue (d);
}

/* Check if a packet must be delivered to the emulated chip based on length*/
static inline int cs8900_handle_len (struct cs8900_data *d, m_uint8_t * pkt,
    ssize_t pkt_len)
{
    /*we do not check CRC !!!! */
    if (pkt_len < CS8900_MIN_PKT_SIZE)
        return FALSE;

    ASSERT ((pkt_len >= CS8900_RUN_PKT_SIZE)
        && (pkt_len <= CS8900_MAX_PKT_SIZE), "not valid pktlen 0x%x\n",
        (unsigned) pkt_len);
    /*64<LEN<1518 */

    return TRUE;
}

/* Check if a packet must be delivered to the emulated chip */
static inline int cs8900_handle_mac_addr (struct cs8900_data *d,
    m_uint8_t * pkt)
{
    n_eth_hdr_t *hdr = (n_eth_hdr_t *) pkt;

    m_uint8_t *ram_base;
    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));

    if ((*(m_uint16_t *) (ram_base + PP_RxCTL)) & PromiscuousA) {
        goto rx_dest_int;
    }
    if (eth_addr_is_bcast (&hdr->daddr)) {
        if ((*(m_uint16_t *) (ram_base + PP_RxCTL)) & BroadcastA) {
            *(m_uint16_t *) (ram_base + PP_RxEvent) |= BroadcastA;
            *(m_uint16_t *) (ram_base + PP_RxStatus) |= BroadcastA;
            goto rx_dest_int;
        } else
            return FALSE;
    }
    if (eth_addr_is_mcast (&hdr->daddr)) {
        if ((*(m_uint16_t *) (ram_base + PP_RxCTL)) & MulticastA) {
            *(m_uint16_t *) (ram_base + PP_RxEvent) |= MulticastA;
            *(m_uint16_t *) (ram_base + PP_RxStatus) |= MulticastA;
            goto rx_dest_int;
        } else
            return FALSE;
    }

    if ((*(m_uint16_t *) (ram_base + PP_RxCTL)) & IndividualA) {
        /* Accept frames directly for us, discard others */
        if (!memcmp ((ram_base + PP_IA), &hdr->daddr, N_ETH_ALEN)) {
            *(m_uint16_t *) (ram_base + PP_RxEvent) |= IndividualA;
            *(m_uint16_t *) (ram_base + PP_RxStatus) |= IndividualA;
            goto rx_dest_int;
        } else
            return FALSE;

    }

  rx_dest_int:
    return (TRUE);
}

static int dev_cs8900_receive_pkt (struct cs8900_data *d, u_char * pkt,
    ssize_t pkt_len)
{
    m_uint8_t *ram_base;
    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));

    /* Truncate the packet if it is too big */
    pkt_len = m_min (pkt_len, CS8900_MAX_PKT_SIZE);
    /*set RX len */
    *(m_uint16_t *) (ram_base + PP_RxLength) = pkt_len;
    /*Rx status has been set */
    /*just copy frame to internal ram */
    memcpy (ram_base + PP_RxFrame, pkt, pkt_len);
    /*generate interrupt */

    *(m_uint16_t *) (ram_base + PP_RxEvent) |= RxOKA;
    *(m_uint16_t *) (ram_base + PP_RxStatus) |= RxOKA;
    if ((*(m_uint16_t *) (ram_base + PP_RxCFG)) & RxOKiE) {
        //*(m_uint16_t*)(ram_base+PP_ISQ) &= ~0x3f;
        *(m_uint16_t *) (ram_base + PP_ISQ) |= RxEvent;
        dev_cs8900_gen_interrupt (d);
    }

    return TRUE;

}

static int dev_cs8900_rx (netio_desc_t * nio, u_char * pkt, ssize_t pkt_len,
    struct cs8900_data *d)
{
    m_uint8_t *ram_base;
    m_uint16_t real_len;
    int i;
    m_uint32_t ifcs;

    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));

    if (!((*(m_uint16_t *) (ram_base + PP_LineCTL)) & SerRxON))
        return FALSE;
    real_len = pkt_len;

    /*FIXME: yajin
     * jzdriver discard <64 bytes packet. But arp packet has 40 bytes. Pad it to 64 bytes to meet jz driver's requirement
     */
    if (unlikely (pkt_len < 64)) {
        /*pad to 60 bytes */
        for (i = pkt_len; i < 60; i++) {
            *(pkt + i) = 0x0;
        }
        /*add crc */
        ifcs = crc32_compute (0xFFFFFFFF, pkt, 60);
        *(pkt + 60) = ifcs & 0xff;
        *(pkt + 61) = (ifcs >> 8) & 0xff;
        *(pkt + 62) = (ifcs >> 16) & 0xff;
        *(pkt + 63) = ifcs >> 24;
        real_len = 64;
    }

    /*check MAC address */
    if (!(cs8900_handle_mac_addr (d, pkt)))
        return FALSE;

    /*check frame len */
    if (!(cs8900_handle_len (d, pkt, real_len)))
        return FALSE;

    return (dev_cs8900_receive_pkt (d, pkt, real_len));

}

static int dev_cs8900_tx (struct cs8900_data *d)
{

    m_uint8_t *ram_base;
    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));
    m_uint16_t send_len;
    int i;
    m_uint32_t ifcs;

    send_len = *(m_uint16_t *) (ram_base + PP_TxLength);
    /*check if tx is enabled */
    if ((*(m_uint16_t *) (ram_base + PP_LineCTL)) & SerTxON) {
        /*pad if len<60 */
        if (send_len <= (CS8900_RUN_PKT_SIZE - 4)) {
            if (!((*(m_uint16_t *) (ram_base + PP_TxCMD)) & TxPadDis)) {
                /*pad to 60 bytes */
                for (i = send_len; i < 60; i++) {
                    *(ram_base + PP_TxFrame + i) = 0x0;
                }
                send_len = 60;
                if (!((*(m_uint16_t *) (ram_base + PP_TxCMD)) & InhibitCRC)) {
                    /*append crc */
                    ifcs =
                        crc32_compute (0xFFFFFFFF, ram_base + PP_TxFrame,
                        send_len);
                    *(ram_base + PP_TxFrame + send_len) = ifcs & 0xff;
                    *(ram_base + PP_TxFrame + send_len + 1) =
                        (ifcs >> 8) & 0xff;
                    *(ram_base + PP_TxFrame + send_len + 2) =
                        (ifcs >> 16) & 0xff;
                    *(ram_base + PP_TxFrame + send_len + 3) = ifcs >> 24;
                    send_len += 4;

                }
            }
        }
        *(m_uint16_t *) (ram_base + PP_TxLength) = send_len;
        netio_send (d->nio, ram_base + PP_TxFrame, send_len);
        *(m_uint16_t *) (ram_base + PP_TxEvent) = TxOK | 0x8;   /*is = not |.  all other bits must be cleared */
        if ((*(m_uint16_t *) (ram_base + PP_TxCFG)) & TxOKiE) {
            /*if TXOKIE, generate an interrupt */
            /*set ISQ (regno=TX Event) */
            //*(m_uint16_t*)(ram_base+PP_ISQ) &= ~0x3f;
            *(m_uint16_t *) (ram_base + PP_ISQ) |= TxEvent;
            dev_cs8900_gen_interrupt (d);
        }

    }

    return TRUE;
}

/*
how to determinte the timeout value???
*/
void dev_cs8900_active_timer (struct cs8900_data *d)
{
    vp_mod_timer (d->cs8900_timer,
        vp_get_clock (rt_clock) + cs8900a_rx_timeout);
}

void dev_cs8900_unactive_timer (struct cs8900_data *d)
{
    vp_del_timer (d->cs8900_timer);
}

void dev_cs8900_cb (void *opaque)
{
    struct cs8900_data *d = opaque;

    int fd;
    ssize_t pkt_len;

    static m_uint8_t status = 0;

    ASSERT (d->nio != NULL, "set nio first\n");

    if ((fd = netio_get_fd (d->nio)) == -1) {
        ASSERT (0, "can not get nio fd. init cs8900 nio first.\n");
    }

    pkt_len = netio_recv (d->nio, d->nio->rx_pkt, sizeof (d->nio->rx_pkt));

    if (pkt_len > 0) {
        /*rx packet */
        dev_cs8900_rx (d->nio, d->nio->rx_pkt, pkt_len, d);

        /*Why we need to adjust CS8900_MAX_RX_TIMEOUT? yajin
         * If CS8900_MAX_RX_TIMEOUT is small, that means rx packets quickly. Tx can not get enough time to tell cpu
         * that tx ok.
         * If CS8900_MAX_RX_TIMEOUT is big, that means rx packets slow. This will decrease network throughtput and
         * some applications will complain about rx timeout.
         * So I adjut the CS8900_MAX_RX_TIMEOUT dynamicly when receiving a packet .
         *
         * Please use TCP protocol instead of UDP when mounting directory using nfs.
         *
         */
        if (cs8900a_rx_timeout >= CS8900_MAX_RX_TIMEOUT)
            status = 1;
        else if (cs8900a_rx_timeout <= CS8900_MIN_RX_TIMEOUT)
            status = 2;

        if (status == 0)
            cs8900a_rx_timeout -= CS8900_RX_TIMEOUT_STEP;
        if (status == 1)
            cs8900a_rx_timeout -= CS8900_RX_TIMEOUT_STEP;
        else if (status == 2)
            cs8900a_rx_timeout += CS8900_RX_TIMEOUT_STEP;

    }

    cs8900a_rx_timeout = CS8900_DEFAULT_RX_TIMEOUT;
    dev_cs8900_active_timer (d);

}

static void *dev_cs8900_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_uint32_t * data, m_uint8_t * has_set_value)
{
    struct cs8900_data *d = dev->priv_data;
    void *ret;
    m_uint8_t *ram_base;
    m_uint16_t io_address;
    m_uint16_t isq;

    ram_base = (m_uint8_t *) (&(d->internal_ram[0]));

    if (offset >= d->cs8900_size) {
        *data = 0;
        return NULL;
    }
#if  VALIDE_CS8900_OPERATION
    if (op_type == MTS_WRITE) {
        ASSERT (offset != PP_IO_ISQ,
            "Write to read only register in CS8900. offset %x\n", offset);
    } else if (op_type == MTS_READ) {
        ASSERT (offset != PP_TX_CMD,
            "Read write only register in CS8900. offset %x\n", offset);
        ASSERT (offset != PP_TX_LEN,
            "Read write only register in CS8900. offset %x\n", offset);
    }
#endif

    switch (offset) {
    case PP_RT_DATA0:
    case PP_RT_DATA0 + 1:
    case PP_RT_DATA1:
    case PP_RT_DATA1 + 1:
        if (op_type == MTS_READ) {
            ASSERT (d->rx_read_index <
                (*(m_uint16_t *) (ram_base + PP_RxLength)),
                "read out of data rx_read_index %x data len %x \n",
                d->rx_read_index, (*(m_uint16_t *) (ram_base + PP_RxLength)));
            ret = (void *) (ram_base + PP_RxFrame + d->rx_read_index);
            d->rx_read_index += op_size;
                        /****if read all data,set d->rx_read_index=0*/
            if (d->rx_read_index >= *(m_uint16_t *) (ram_base + PP_RxLength))
                d->rx_read_index = 0;
            return ret;
        } else if (op_type == MTS_WRITE) {
            ret = (void *) (ram_base + PP_TxFrame + d->tx_send_index);
            if (op_size == MTS_BYTE)
                *(m_uint8_t *) ret = *data;
            if (op_size == MTS_HALF_WORD)
                *(m_uint16_t *) ret = *data;
            else
                *(m_uint32_t *) ret = *data;
            *has_set_value = TRUE;
            d->tx_send_index += op_size;
            /*if write all data into tx buffer, set d->tx_send_index=0 */
            if (d->tx_send_index >= *(m_uint16_t *) (ram_base + PP_TxLength)) {
                d->tx_send_index = 0;
                /*start tx a frame */
                dev_cs8900_tx (d);
            }
            return NULL;
        }
        break;
    case PP_TX_CMD:

        ret = (void *) (ram_base + PP_TxCMD);
        return ret;
    case PP_TX_LEN:
        ret = (void *) (ram_base + PP_TxLength);
        return ret;
    case PP_IO_ISQ:
        ASSERT (0, "not support PP_IO_ISQ \n");
    case PP_ADDRESS:
        return (void *) (ram_base + PP_ADDRESS);
    case PP_DATA0:
    case PP_DATA1:
        if (offset == PP_DATA0)
            ASSERT (op_size == MTS_HALF_WORD,
                "op_size must be 2. op_size %x\n", op_size);
        else if (offset == PP_DATA1)
            ASSERT (0, "cs8900 only support 16 bit IO operation");
        io_address = *(m_uint16_t *) (ram_base + PP_ADDRESS);
        switch (io_address) {
        case PP_ProductID:
            ASSERT (op_type == MTS_READ, "write to read only register %x\n",
                *(m_uint16_t *) (ram_base + PP_ADDRESS));
            *data = CS8900A_PRODUCT_ID;
            *has_set_value = TRUE;
            return NULL;
        case PP_ProductID + 2: /*16 bit */
            *data = 0;
            *has_set_value = TRUE;
            return NULL;
        case PP_ISAIOB:
        case PP_IntNum:
            return (void *) (ram_base + io_address);
        case PP_ISASOF:
        case PP_DmaFrameCnt:
        case PP_DmaByteCnt:
        case PP_MemBase:
        case PP_EEPROMCommand:
        case PP_EEPROMData:
            ASSERT (0, "Not support yet offset %x \n", io_address);
            break;
        case PP_RxCFG:
            if (op_type == MTS_WRITE) {
                if (*data & Skip_1) {
                    memset (ram_base + PP_RxFrame, 0x0,
                        PP_TxFrame - 1 - PP_RxFrame);
                }
                *(m_uint16_t *) (ram_base + PP_RxCFG) = *data | 0x3;
                *has_set_value = TRUE;
                return NULL;
            } else              /*read */
                return (void *) (ram_base + io_address);
        case PP_RxCTL:
            if (op_type == MTS_WRITE) {
                *(m_uint16_t *) (ram_base + PP_RxCTL) = *data | 0x5;
                *has_set_value = TRUE;
                if (*data & IAHashA)
                    ASSERT (0, "Hash dest address is not support yet \n");
                return NULL;
            } else
                return (void *) (ram_base + io_address);

        case PP_TxCFG:
            if (op_type == MTS_WRITE) {
                *(m_uint16_t *) (ram_base + PP_TxCFG) = *data | 0x7;
                *has_set_value = TRUE;
                return NULL;
            } else
                return (void *) (ram_base + io_address);
        case 0x108:
            /*read 0x108 actually read 0x144 TXcmd */
            ASSERT (op_type == MTS_READ,
                "CS8900 write to read only register. IO address 0x108 \n");
            return (void *) (ram_base + 0x144);
        case PP_BufCFG:
            if (op_type == MTS_WRITE) {
                if (*data & SWint_X) {
                    *(m_uint16_t *) (ram_base + PP_BufEvent) |= SWint;
                    //*(m_uint16_t*)(ram_base+PP_ISQ) &= ~0x3f;
                    *(m_uint16_t *) (ram_base + PP_ISQ) |= BufEvent;
                    dev_cs8900_gen_interrupt (d);
                }
                if (*data & Rdy4TxiE) {
                    /*if host set rdy4tx, we are always ready for tx */
                    *(m_uint16_t *) (ram_base + PP_BufEvent) |= Rdy4Tx;
                    //*(m_uint16_t*)(ram_base+PP_ISQ) &= ~0x3f;
                    *(m_uint16_t *) (ram_base + PP_ISQ) |= BufEvent;
                    dev_cs8900_gen_interrupt (d);
                }
                *(m_uint16_t *) (ram_base + PP_BufCFG) = *data | 0xb;
                *has_set_value = TRUE;
                return NULL;
            } else
                return (void *) (ram_base + io_address);

        case PP_LineCTL:
            if (op_type == MTS_WRITE) {
                *(m_uint16_t *) (ram_base + PP_LineCTL) = *data | 0x13;
                if ((*data & SerRxON) || (*data & SerTxON))
                    dev_cs8900_active_timer (d);
                *has_set_value = TRUE;
                return NULL;
            } else
                return (void *) (ram_base + io_address);
        case PP_SelfCTL:
            if (op_type == MTS_WRITE) {
                if (*data & RESET) {
                    dev_cs8900_reset (cpu, dev);
                }
                *(m_uint16_t *) (ram_base + PP_SelfCTL) = *data | 0x15;
                *has_set_value = TRUE;
                return NULL;
            } else
                return (void *) (ram_base + io_address);
        case PP_BusCTL:
            if (op_type == MTS_WRITE) {
                *(m_uint16_t *) (ram_base + PP_BusCTL) = *data | 0x17;
                *has_set_value = TRUE;
                return NULL;
            } else
                return (void *) (ram_base + io_address);

        case PP_TestCTL:
            if (op_type == MTS_WRITE) {
                *(m_uint16_t *) (ram_base + PP_TestCTL) = *data | 0x19;
                *has_set_value = TRUE;
                return NULL;
            } else
                return (void *) (ram_base + io_address);

        case PP_ISQ:
            isq = *(m_uint16_t *) (ram_base + PP_ISQ);
            if (op_type == MTS_WRITE) {
                *(m_uint16_t *) (ram_base + PP_ISQ) = 0;
                *has_set_value = TRUE;
                return NULL;
            }
            /*Readonly? But sometimes, kernel will write to this register. */
            //ASSERT(op_type == MTS_READ, "wirte to read only register io_address %x.", io_address);
            /*SHOULD be read */
            if (isq & TxEvent) {
                *(m_uint16_t *) (ram_base + PP_ISQ) &= ~TxEvent;
                *(m_uint16_t *) data =
                    *(m_uint16_t *) (ram_base + PP_TxEvent);
                *(m_uint16_t *) (ram_base + PP_TxEvent) = 0X8;
                //return (void*)(ram_base+PP_TxEvent);
            } else if (isq & RxEvent) {
                *(m_uint16_t *) (ram_base + PP_ISQ) &= ~RxEvent;
                *(m_uint16_t *) data =
                    *(m_uint16_t *) (ram_base + PP_RxEvent);
                *(m_uint16_t *) (ram_base + PP_RxEvent) = 0X4;

                //return (void*)(ram_base+PP_RxEvent);
            } else if (isq & BufEvent) {
                *(m_uint16_t *) (ram_base + PP_ISQ) &= ~BufEvent;
                *(m_uint16_t *) data =
                    *(m_uint16_t *) (ram_base + PP_BufEvent);
                *(m_uint16_t *) (ram_base + PP_BufEvent) = 0Xc;

                //return (void*)(ram_base+PP_BufEvent);
            }

            else if (isq & RxMISS) {
                *(m_uint16_t *) (ram_base + PP_ISQ) &= ~RxMISS;
                *(m_uint16_t *) data = *(m_uint16_t *) (ram_base + PP_RxMISS);
                *(m_uint16_t *) (ram_base + PP_RxMISS) = 0x10;
                //return (void*)(ram_base+PP_RxMISS);
            } else if (isq & TxCOL) {
                *(m_uint16_t *) (ram_base + PP_ISQ) &= ~TxCOL;
                *(m_uint16_t *) data = *(m_uint16_t *) (ram_base + PP_TxCOL);
                *(m_uint16_t *) (ram_base + PP_TxCOL) = 0x12;
                //return (void*)(ram_base+PP_TxCOL);
            } else {
                return (void *) (ram_base + PP_ISQ);
            }
            *has_set_value = TRUE;
            return NULL;
            break;
        case PP_RxEvent:
            /*read rx event will clear it */
            ASSERT (op_type == MTS_READ,
                "CS8900 write to read only register. IO address %x \n",
                io_address);
            *(m_uint16_t *) data = *(m_uint16_t *) (ram_base + PP_RxEvent);
            *has_set_value = TRUE;
            *(m_uint16_t *) (ram_base + PP_RxEvent) = 0X4;
            return NULL;
        case PP_TxEvent:
            /*read tx event will clear it */
            ASSERT (op_type == MTS_READ,
                "CS8900 write to read only register. IO address %x \n",
                io_address);
            *(m_uint16_t *) data = *(m_uint16_t *) (ram_base + PP_TxEvent);
            *has_set_value = TRUE;
            *(m_uint16_t *) (ram_base + PP_TxEvent) = 0X8;
            return NULL;
        case PP_BufEvent:
            /*read BufEvent event will clear it */
            ASSERT (op_type == MTS_READ,
                "CS8900 write to read only register. IO address %x \n",
                io_address);
            *(m_uint16_t *) data = *(m_uint16_t *) (ram_base + PP_BufEvent);
            *has_set_value = TRUE;
            *(m_uint16_t *) (ram_base + PP_BufEvent) = 0Xc;
            return NULL;

        case PP_RxMISS:
        case PP_TxCOL:
        case PP_LineST:
        case PP_SelfST:
        case PP_TDR:
            ASSERT (op_type == MTS_READ,
                "CS8900 write to read only register. IO address %x \n",
                io_address);
            return (void *) (ram_base + io_address);
        case PP_BusST:
            *(m_uint16_t *) (ram_base + PP_BusST) |= Rdy4TxNOW;
            //else
            //{
            //      *(m_uint16_t*)(ram_base+PP_BusST) &= ~Rdy4TxNOW;
            //}
            return (void *) (ram_base + io_address);
        case PP_TxCMD:
            ASSERT (op_type == MTS_WRITE,
                "CS8900 read write only register. IO address %x \n",
                PP_TxCMD);
            *(m_uint16_t *) (ram_base + PP_TxCMD) = *data | 0x9;
            *has_set_value = TRUE;
            return NULL;

        case PP_TxLength:
            ASSERT (op_type == MTS_WRITE,
                "CS8900 read write only register. IO address %x \n",
                PP_TxLength);
            *(m_uint16_t *) (ram_base + PP_TxLength) = *data;
            *has_set_value = TRUE;
            if (*(m_uint16_t *) (ram_base + PP_TxLength) > 1518) {
                *(m_uint16_t *) (ram_base + PP_BusST) |= TxBidErr;
            } else if (*(m_uint16_t *) (ram_base + PP_TxLength) > 1514) {
                if (!((*(m_uint16_t *) (ram_base + PP_TxCMD)) & InhibitCRC))
                    *(m_uint16_t *) (ram_base + PP_BusST) |= TxBidErr;
                else
                    *(m_uint16_t *) (ram_base + PP_BusST) &= ~TxBidErr;
            } else
                *(m_uint16_t *) (ram_base + PP_BusST) &= ~TxBidErr;
            return NULL;
        case PP_LAF:
        case PP_LAF + 1:
        case PP_LAF + 2:
        case PP_LAF + 3:
        case PP_LAF + 4:
        case PP_LAF + 5:
        case PP_LAF + 6:
        case PP_LAF + 7:
        case PP_IA:
        case PP_IA + 1:
        case PP_IA + 2:
        case PP_IA + 3:
        case PP_IA + 4:
        case PP_IA + 5:
        case PP_RxStatus:
        case PP_RxLength:
            return (void *) (ram_base + io_address);

        default:
            ASSERT (0, "error io address %x\n", io_address);

        }

    }

    return NULL;
}

struct cs8900_data *dev_cs8900_init (vm_instance_t * vm, char *name,
    m_pa_t phys_addr, m_uint32_t phys_len, int irq)
{
    struct cs8900_data *d;

    /* Allocate the private data structure for DEC21140 */
    if (!(d = malloc (sizeof (*d)))) {
        fprintf (stderr, "%s (cs8900_data): out of memory\n", name);
        return NULL;
    }
    memset (d, 0, sizeof (*d));

    /* Create the device itself */
    if (!(d->dev = dev_create (name))) {
        fprintf (stderr, "%s (DEC21140): unable to create device.\n", name);
        goto err_dev;
    }
    d->irq_no = irq;
    d->vm = vm;
    d->dev->priv_data = d;
    d->dev->phys_addr = phys_addr;
    d->dev->phys_len = phys_len;
    d->cs8900_size = phys_len;

    d->dev->handler = dev_cs8900_access;
    d->dev->reset_handler = dev_cs8900_reset;

    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->cs8900_timer = vp_new_timer (rt_clock, dev_cs8900_cb, d);
    vm_bind_device (vm, d->dev);
    return (d);

  err_dev:
    free (d);
    return NULL;
}

int dev_cs8900_set_nio (struct cs8900_data *d, netio_desc_t * nio)
{

    /* check that a NIO is not already bound */
    if (d->nio != NULL)
        return (-1);

    d->nio = nio;

    return (0);
}
