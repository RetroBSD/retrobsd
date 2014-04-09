/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * Edit History
 *  Aug  3,  2011 <Lowell Scott Hanson> ported toh chipKIT boards
 *  Sept 13, 2011 <Gene Apperson> change SPI clock divider from DIV8 to DIV32
 *  Apr  16, 2012 <Serge Vakulenko> ported to RetroBSD
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/spi.h>
#include <wiznet/w5100.h>

#define SPI_DEVNAME         "/dev/spi2" // W5100 chip is connected to SPI2
#define SPI_KHZ             5000        // Clock speed 5 MHz
#define SPI_SELPIN          0x0404      // chipKIT board: select pin D4

#define TXBUF_BASE          0x4000
#define RXBUF_BASE          0x6000

#define RST                 7           // Reset BIT

#define TXBUF_MASK          (TXBUF_SIZE-1)      // Tx buffer MASK
#define RXBUF_MASK          (RXBUF_SIZE-1)      // Rx buffer MASK

static uint16_t SBASE [MAX_SOCK_NUM];   // Tx buffer base address
static uint16_t RBASE [MAX_SOCK_NUM];   // Rx buffer base address

static int spi;                         // SPI driver descriptor

void w5100_init()
{
    int i, failed;

    spi = open (SPI_DEVNAME, O_RDWR);
    if (spi < 0) {
        perror (SPI_DEVNAME);
        exit (-1);
    }

    ioctl (spi, SPICTL_SETRATE, SPI_KHZ);
    ioctl (spi, SPICTL_SETSELPIN, SPI_SELPIN);

    /* Reset the chip. */
    w5100_writeMR (MR_RST);

    /* Assign 2 kbytes of memory to RX and TX of each socket. */
    w5100_writeTMSR (0x55);
    w5100_writeRMSR (0x55);

    for (i=0; i<MAX_SOCK_NUM; i++) {
        SBASE[i] = TXBUF_BASE + TXBUF_SIZE * i;
        RBASE[i] = RXBUF_BASE + RXBUF_SIZE * i;
    }

    /* Self test. */
    failed = 0;
    for (i=0; i<16; i++) {
        unsigned val, got;

        val = 1 << i;
        w5100_writeRTR (val);
        got = w5100_readRTR();
        if (got != val) {
            failed++;
            fprintf (stderr, "w5100 self test failed: written %08x, got %08x\n",
                val, got);
        }
    }
    if (failed)
        exit (-1);
}

unsigned w5100_getTXFreeSize (unsigned sock)
{
    unsigned val, val2;

    for (;;) {
        /* Nonzero value expected. */
        val = w5100_readSnTX_FSR (sock);
        if (val == 0)
            return 0;

        /* Do it twice until value matches. */
        val2 = w5100_readSnTX_FSR (sock);
        if (val == val2) {
            W5100_DEBUG ("TX free = %u\n", val);
            return val;
        }
    }
}

unsigned w5100_getRXReceivedSize (unsigned sock)
{
    unsigned val, val2;

    for (;;) {
        /* Nonzero value expected. */
        val = w5100_readSnRX_RSR (sock);
        if (val == 0)
            return 0;

        /* Do it twice until value matches. */
        val2 = w5100_readSnRX_RSR (sock);
        if (val == val2) {
            W5100_DEBUG ("RX received = %u\n", val);
            return val;
        }
    }
}

void w5100_send_chunk (unsigned sock, const uint8_t *data, unsigned len)
{
    unsigned ptr = w5100_readSnTX_WR (sock);
    unsigned offset = ptr & TXBUF_MASK;
    unsigned dstAddr = offset + SBASE[sock];

    W5100_DEBUG ("send chunk: TX write pointer = %04x\n", ptr);
    if (offset + len > TXBUF_SIZE) {
        // Wrap around circular buffer
        unsigned size = TXBUF_SIZE - offset;
        w5100_write (dstAddr, data, size);
        w5100_write (SBASE[sock], data + size, len - size);
        W5100_DEBUG ("TX %04x-%04x, %04x-%04x\n", dstAddr, dstAddr+size-1, SBASE[sock], SBASE[sock]+len-size-1);
    } else {
        w5100_write (dstAddr, data, len);
        W5100_DEBUG ("TX %04x-%04x\n", dstAddr, dstAddr+len-1);
    }

    ptr += len;
    w5100_writeSnTX_WR (sock, ptr);
    W5100_DEBUG ("set TX write pointer = %04x\n", ptr);
}

void w5100_recv_chunk (unsigned sock, uint8_t *data, unsigned len)
{
    unsigned ptr = w5100_readSnRX_RD (sock);

    W5100_DEBUG ("recv chunk: RX pointer = %04x\n", ptr);
    w5100_read_data (sock, ptr, data, len);

    ptr += len;
    w5100_writeSnRX_RD (sock, ptr);
    W5100_DEBUG ("set RX pointer = %04x\n", ptr);
}

unsigned w5100_recv_peek (unsigned sock)
{
    unsigned ptr = w5100_readSnRX_RD (sock);
    unsigned char byte;

    w5100_read_data (sock, ptr, &byte, 1);
    return byte;
}

void w5100_socket_cmd (unsigned sock, int cmd)
{
    // Send command to socket
    w5100_writeSnCR (sock, cmd);

    // Wait for command to complete
    while (w5100_readSnCR (sock))
        ;
}

void w5100_read_data (unsigned sock, unsigned ptr, uint8_t *dst, unsigned len)
{
    unsigned offset = ptr & RXBUF_MASK;
    unsigned srcAddr = offset + RBASE[sock];

    if (offset + len > RXBUF_SIZE)
    {
        unsigned size = RXBUF_SIZE - offset;
        w5100_read (srcAddr, dst, size);
        w5100_read (RBASE[sock], dst + size, len - size);
        W5100_DEBUG ("RX %04x-%04x, %04x-%04x\n", srcAddr, srcAddr+size-1, SBASE[sock], SBASE[sock]+len-size-1);
    } else {
        w5100_read (srcAddr, dst, len);
        W5100_DEBUG ("RX %04x-%04x\n", srcAddr, srcAddr+len-1);
    }
}

unsigned w5100_write_byte (unsigned addr, int byte)
{
    uint8_t data[4];

    data[0] = 0xF0;
    data[1] = addr >> 8;
    data[2] = addr;
    data[3] = byte;
    ioctl (spi, SPICTL_IO8(4), data);
    return 1;
}

unsigned w5100_write (unsigned addr, const uint8_t *buf, unsigned len)
{
    uint8_t data[4];
    unsigned i;

    for (i=0; i<len; i++) {
        data[0] = 0xF0;
        data[1] = addr >> 8;
        data[2] = addr;
        data[3] = buf[i];
        ioctl (spi, SPICTL_IO8(4), data);
        addr++;
    }
    return len;
}

unsigned w5100_read_byte (unsigned addr)
{
    uint8_t data[4];

    data[0] = 0x0F;
    data[1] = addr >> 8;
    data[2] = addr;
    data[3] = 0xFF;
    ioctl (spi, SPICTL_IO8(4), data);

    return data[3];
}

unsigned w5100_read (unsigned addr, uint8_t *buf, unsigned len)
{
    uint8_t data[4];
    unsigned i;

    for (i=0; i<len; i++) {
        data[0] = 0x0F;
        data[1] = addr >> 8;
        data[2] = addr;
        data[3] = 0xFF;
        ioctl (spi, SPICTL_IO8(4), data);
        addr++;
        buf[i] = data[3];
    }
    return len;
}
