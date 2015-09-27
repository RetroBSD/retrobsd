#ifndef _SD_H
#define _SD_H

#ifdef KERNEL
extern int sdopen(dev_t dev, int flag, int mode);
extern int sdclose(dev_t dev, int flag, int mode);
extern daddr_t sdsize(dev_t dev);
extern void sdstrategy(register struct buf *bp);
extern int sdioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag);

extern const struct devspec sddevs[];
#endif

#endif
