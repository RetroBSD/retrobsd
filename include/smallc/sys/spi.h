/*
 * Ioctl definitions for SPI driver.
 */
#define SPICTL_SETMODE      0x20007000              /* set SPI mode */
#define SPICTL_SETRATE      0x20007001              /* set clock rate, kHz */
#define SPICTL_SETSELPIN    0x20007002              /* set select pin */
#define SPICTL_IO8(n)      (0xc0007003 | (n)<<16)   /* transfer n*8 bits */
#define SPICTL_IO16(n)     (0xc0007004 | (n)<<16)   /* transfer n*16 bits */
#define SPICTL_IO32(n)     (0xc0007005 | (n)<<16)   /* transfer n*32 bits */
