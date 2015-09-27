#ifndef _SDRAMP_H
#define _SDRAMP_H

#ifdef KERNEL

extern int sdramp_write(int unit, unsigned blockno, char* data, unsigned nbytes);
extern int sdramp_read (int unit, unsigned blockno, char *data, unsigned nbytes);
extern void sdramp_preinit(int unit);
extern int sdramp_size(int unit);
extern int sdramp_open(int unit, int a, int b);

#endif

#endif
