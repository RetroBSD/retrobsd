#ifndef _SDRAMP_H
#define _SDRAMP_H

#ifdef KERNEL
extern int sdramp_open(dev_t dev, int flag, int mode);
extern int sdramp_close(dev_t dev, int flag, int mode);
extern daddr_t sdramp_size(dev_t dev);
extern void sdramp_strategy(struct buf *bp);
extern int sdramp_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
