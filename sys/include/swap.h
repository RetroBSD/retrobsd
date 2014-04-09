#ifndef _SWAP_H
#define _SWAP_H

#ifdef KERNEL
#include "ioctl.h"
#else
#include <sys/ioctl.h>
#endif

#define TFALLOC _IOWR('s',1,off_t)

#ifdef KERNEL

extern int swopen(dev_t dev, int mode, int flag);
extern int swclose(dev_t dev, int mode, int flag);
extern void swstrategy(register struct buf *bp);
extern daddr_t swsize(dev_t dev);
extern int swcread(dev_t dev, register struct uio *uio, int flag);
extern int swcwrite(dev_t dev, register struct uio *uio, int flag);
extern int swcioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag);
extern int swcopen(dev_t dev, int mode, int flag);
extern int swcclose(dev_t dev, int mode, int flag);

extern const struct devspec swapbdevs[];
extern const struct devspec swapcdevs[];

#endif

#endif
