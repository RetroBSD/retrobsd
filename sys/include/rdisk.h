#ifndef _RDISK_H
#define _RDISK_H

#include "conf.h"

#define RDISK_FS 	0xB7
#define RDISK_SWAP 	0xB8

#define RD_DEFAULT	0x00000000UL
#define RD_READONLY	0x00000001UL
#define RD_PREPART	0x00000002UL

#define S_SILENT	0x8000

#define RAMDISK_PARTSPEC(n,t,s,l)       \
m->partitions[n].type=t;                \
m->partitions[n].lbastart=s;            \
m->partitions[n].lbalength=l;

struct diskentry {
	void (*pre_init)(int unit);
	int (*init)(int unit,int flag);
	int (*deinit)(int unit);
	int (*open)(int unit, int mode, int flags);
	int (*size)(int unit);
	int (*read)(int unit, unsigned int offset, char *data, unsigned int bcount);
	int (*write)(int unit, unsigned int offset, char *data, unsigned int bcount);
	unsigned char unit;
	unsigned int settings;
};

struct diskflags {
	unsigned char opens;
	unsigned int start[4];
	unsigned int len[4];
	unsigned int blocks;
} __attribute__((packed));

struct chs {
	unsigned char head;
	struct {
		unsigned cylhigh:2;
		unsigned sector:6;
	} __attribute__((packed));
	unsigned char cyllow;
}__attribute__((packed));

struct partition {
#define P_ACTIVE 0x80
	unsigned char status;
	struct chs start;
	unsigned char type;
	struct chs end;
	unsigned long lbastart;
	unsigned long lbalength;
};

struct mbr {
	unsigned char bootstrap1[218];
	unsigned short pad0000;
	unsigned char biosdrive;
	unsigned char secs;
	unsigned char mins;
	unsigned char hours;
	unsigned char bootstrap2[216];
	unsigned int sig;
	unsigned short pad0001;
	struct partition partitions[4];
	unsigned short bootsig;
}__attribute__((packed));

#ifdef KERNEL
extern int rdopen(dev_t dev, int flag, int mode);
extern int rdclose(dev_t dev, int flag, int mode);
extern daddr_t rdsize(dev_t dev);
extern void rdstrategy(register struct buf *bp);
extern int partition_size(dev_t dev);
extern int rdioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag);
extern void rdisk_init();
extern void rdisk_list_partitions(unsigned char type);
extern int rdisk_num_disks();

extern dev_t get_boot_device();
extern dev_t get_swap_device();
extern unsigned char partition_type(dev_t dev);
extern struct buf *prepartition_device(char *devname);

extern const struct devspec rd0devs[];
extern const struct devspec rd1devs[];
extern const struct devspec rd2devs[];
extern const struct devspec rd3devs[];
#endif 

#define RDGETMEDIASIZE _IOR('r',1,int)
#define RDREINIT _IO('r',2)

#endif
