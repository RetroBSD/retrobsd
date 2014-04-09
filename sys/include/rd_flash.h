#ifndef _RD_FLASH_H
#define _RD_FLASH_H

#ifdef KERNEL

extern int flash_read(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int flash_write(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int flash_open(int unit, int flag, int mode);
extern int flash_size(int unit);
extern void flash_init(int unit, int flag);

#endif

#endif
