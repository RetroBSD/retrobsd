#ifndef _SRAMC_H
#define _SRAMC_H

#ifdef KERNEL

extern int sramc_read(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int sramc_write(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int sramc_open(int unit, int flag, int mode);
extern void sramc_init(int unit);
extern int sramc_size(int unit);

#endif

#endif
