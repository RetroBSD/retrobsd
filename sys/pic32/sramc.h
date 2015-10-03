#ifndef _SRAMC_H
#define _SRAMC_H

#ifdef KERNEL
extern int sramc_open(dev_t dev, int flag, int mode);
extern int sramc_close(dev_t dev, int flag, int mode);
extern daddr_t sramc_size(dev_t dev);
extern void sramc_strategy(struct buf *bp);
extern int sramc_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
