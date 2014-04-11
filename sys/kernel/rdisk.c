#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "debug.h"
#include "ioctl.h"
#include "rdisk.h"
#include "conf.h"

#define Q2(X) #X
#define QUOTE(X) Q2((X))

extern struct buf *getnewbuf();

/*
 * Variable naming conventions
 * 
 * root - this is the number of the device entry in the disks[] array
 * part - the minor number of a device entry, which represents the partition
 *        number, or 0 for the whole disk
 * unit - the physical device number of a specific device type.  Equates
 *        to the .unit entry in the diskentry structure for a device.
 */

extern int card_read(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int card_write(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int sdinit(int unit, int flag);
extern int sddeinit(int unit);
extern void sd_preinit(int unit);
extern int sdopen(int unit, int flags, int mode);
extern int sdsize(int unit);

#ifdef SRAMC_ENABLED
#include <rd_sramc.h>
#endif
#ifdef SDRAMP_ENABLED
#include <rd_sdramp.h>
#endif
#ifdef FLASH_ENABLED
#include <rd_flash.h>
#endif
#ifdef MRAMS_ENABLED
#include <rd_mrams.h>
#endif

int no_deinit(int u) { return 0; }
void no_preinit(int u) { return; }
int no_init(int u, int v) { return 0; }
int no_open(int u, int a, int b) { return 0; }
int no_size(int u) { return 0; }
int no_read(int u, unsigned int o, char *dat, unsigned int bs) { return 0; }
int no_write(int u, unsigned int o, char *dat, unsigned int bs) { return 0; }

const struct devspec rd0devs[] = { { 0, "rd0" }, { 1, "rd0a" }, { 2, "rd0b" }, { 3, "rd0c" }, { 4, "rd0d" }, { 0, 0 } };
const struct devspec rd1devs[] = { { 0, "rd1" }, { 1, "rd1a" }, { 2, "rd1b" }, { 3, "rd1c" }, { 4, "rd1d" }, { 0, 0 } };
const struct devspec rd2devs[] = { { 0, "rd2" }, { 1, "rd2a" }, { 2, "rd2b" }, { 3, "rd2c" }, { 4, "rd2d" }, { 0, 0 } };
const struct devspec rd3devs[] = { { 0, "rd3" }, { 1, "rd3a" }, { 2, "rd3b" }, { 3, "rd3c" }, { 4, "rd3d" }, { 0, 0 } };

// This is the list of physical storage devices on the system.
// Uncomment the ones you want below.  Maximum 4 at the moment.
// They number, in the order of this list, rd0, rd1, rd2 and rd3.

const struct diskentry disks[] = {

	{sd_preinit, sdinit, sddeinit, sdopen, sdsize, card_read, card_write, 0, RD_DEFAULT},

#ifdef SD1_PORT
	{sd_preinit, sdinit, sddeinit, sdopen, sdsize, card_read, card_write, 1, RD_DEFAULT},
#endif

#ifdef SRAMC_ENABLED
	{sramc_init, no_init, no_deinit, sramc_open, sramc_size, sramc_read, sramc_write, 0, RD_PREPART},
#endif

#ifdef SDRAMP_ENABLED
	{sdramp_preinit, no_init, no_deinit, sdramp_open, sdramp_size, sdramp_read, sdramp_write, 0, RD_PREPART},
#endif	

#ifdef FLASH_ENABLED
	{flash_init, no_init, no_deinit, flash_open, flash_size, flash_read, flash_write, 0, RD_READONLY},
#endif

#ifdef MRAMS_ENABLED
	{mrams_preinit, no_init, no_deinit, no_open, mrams_size, mrams_read, mrams_write, 0, RD_DEFAULT},
#endif

};

#define NRDSK sizeof(disks)/sizeof(struct diskentry)
#define MAXDEV NRDSK-1

#ifdef UCB_METER
int rddk = -1;
#endif

struct diskflags dflags[NRDSK];

static inline struct buf *read_mbr(int root)
{
	if(root>MAXDEV) return NULL;

	int rv;
	int unit = disks[root].unit;

	struct buf *bp = getnewbuf();

	DEBUG8("rd%d: read mbr from device %d\n",root,unit);
	rv = disks[root].read(unit,0,bp->b_addr,512);
	if(rv==0)
	{
		DEBUG8("rd%d: mbr read FAIL\n",root);
		brelse(buf);
		return NULL;
	}
	DEBUG8("rd%d: mbr read OK\n",root);

	return bp;
}


static inline int init_device(int root,int flag)
{
	int i;
	int e;
	if(root>MAXDEV) return ENODEV;

	struct buf *bp;
	struct mbr *mbr;

	int unit = disks[root].unit;

	e = disks[root].init(unit,flag);

	if(e!=0)
		return e;

	DEBUG8("rd%d: about to read mbr\n",root);
	bp = read_mbr(root);
	if(!bp)
		return ENXIO;
	DEBUG8("rd%d: mbr read\n",root);

	mbr = (struct mbr *)bp->b_addr;

	DEBUG5("rd%d: partition types: %02X %02X %02X %02X\n",root,
		mbr->partitions[0].type,
		mbr->partitions[1].type,
		mbr->partitions[2].type,
		mbr->partitions[3].type
	);
	DEBUG8("rd%d: partition 1 start: %p length: %p\n",root,
		mbr->partitions[0].lbastart, mbr->partitions[0].lbalength
	);
	DEBUG8("rd%d: partition 2 start: %p length: %p\n",root,
		mbr->partitions[1].lbastart, mbr->partitions[1].lbalength
	);
	DEBUG8("rd%d: partition 3 start: %p length: %p\n",root,
		mbr->partitions[2].lbastart, mbr->partitions[2].lbalength
	);
	DEBUG8("rd%d: partition 4 start: %p length: %p\n",root,
		mbr->partitions[3].lbastart, mbr->partitions[3].lbalength
	);

	for(i=0; i<4; i++)
	{
		dflags[root].start[i] = mbr->partitions[i].lbastart>>1;
		dflags[root].len[i] = mbr->partitions[i].lbalength>>1;
	}
	dflags[root].blocks = disks[root].size(unit);
	brelse(bp);
	return 0;
}

static inline int deinit_device(int root)
{
	if(root>MAXDEV) return ENODEV;
	return disks[root].deinit(disks[root].unit);
}

static inline int open_device(int root, int flag)
{
	int e;
	if(root>MAXDEV) return ENODEV;

	DEBUG3("rd%d: opening\n",root);
	if(dflags[root].opens==0)
	{
		DEBUG3("rd%d: init device\n",root);
		e = init_device(root, flag);
		if(e!=0)
			return e;
	}
	dflags[root].opens++;

	DEBUG3("rd%d: opened: %d\n",root,dflags[root].opens);
	return 0;
}

static inline int close_device(int root)
{
	if(root>MAXDEV) return ENODEV;
	if(dflags[root].opens==0)
		return ENXIO;
	dflags[root].opens--;
	if(dflags[root].opens==0)
	{
		deinit_device(root);
	}
	DEBUG3("rd%d: closed: %d\n",root,dflags[root].opens);
	return 0;
}

int rdopen(dev_t dev, int mode, int flag)
{
	int e;
	int root = major(dev);
	if(root>MAXDEV) return ENODEV;

	int unit = disks[root].unit;

	e=open_device(root,flag);
	if(e!=0)
		return e;
	e=disks[root].open(unit,mode,flag);
	if(e!=0)
		return e;

	return 0;
}

int rdclose(dev_t dev, int mode, int flag)
{
	int root = major(dev);
	if(root>MAXDEV) return ENODEV;
	close_device(root);
	return 0;
}

daddr_t rdsize(dev_t dev)
{
	int root = major(dev);
	if(root>MAXDEV) return ENODEV;

	int part = minor(dev);
	int unit = disks[root].unit;
	unsigned int blocks;

	if(part==0)
	{
		return disks[root].size(unit);
	} else {
		if(rdopen(dev,0,S_SILENT)!=0)
			return 0;
		blocks=dflags[root].len[part-1];
		rdclose(dev,0,0);
		DEBUG3("rd%d%c: get partition size: %d\n",root,part+'a'-1,blocks);
		return blocks;
	}
}

void rdstrategy(register struct buf *bp)
{
	int root = major(bp->b_dev);
    static int mutex = 0;
	if(root>MAXDEV) return;

    mutex++;
    if(mutex>1)
    {
        led_control(LED_SWAP,1);
    } else {
        led_control(LED_DISK,0);
    }

	int part = minor(bp->b_dev);
	int unit = disks[root].unit;

	int offset=0;
	int s;

	if(part>0)
		offset = dflags[root].start[part-1];

	offset += (bp->b_blkno);

    if (bp->b_dev == swapdev) {
        led_control(LED_SWAP,1);
    } else {
        led_control(LED_DISK,1);
    }

    s = splbio();

#ifdef UCB_METER
        if (rddk >= 0) {
                dk_busy |= 1 << (rddk + root);
                dk_xfer[rddk + root]++;
                dk_bytes[rddk + root] += bp->b_bcount;
        }
#endif


	if (bp->b_flags & B_READ) {
		disks[root].read(unit, offset, bp->b_addr, bp->b_bcount);
	} else {
		if(!(disks[root].settings & RD_READONLY))
			disks[root].write(unit, offset, bp->b_addr, bp->b_bcount);
	}

	biodone(bp);
    if (bp->b_dev == swapdev) {
        led_control(LED_SWAP,0);
    } else {
        led_control(LED_DISK,0);
    }
	splx(s);
    mutex--;
}

void update_mbr(int unit)
{
	
}

int rdioctl (dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
        int *val;

        val = (int *)addr;

        if(cmd == RDGETMEDIASIZE)
	{
		*val = rdsize(dev);
        }
	if(cmd == RDREINIT)
	{
		bflush(major(dev));
		init_device(major(dev),S_SILENT);
	
	}
	return 0;
}

void rdisk_init()
{
	int i;

	//printf("Prepartition Schema: %s\n",prepartition_schema);

#ifdef UCB_METER
	dk_alloc(&rddk,NRDSK,"rd");
#endif

	for(i=0; i<NRDSK; i++)
	{
		disks[i].pre_init(disks[i].unit);
		if(disks[i].settings & RD_PREPART)
		{
		}
	}
}

void rdisk_list_partitions(unsigned char type)
{
	int i,j;
	int e;
	struct buf *bp;
	struct mbr *mbr;
	for(i=0; i<NRDSK; i++)
	{
		e = init_device(i,S_SILENT);
		if(e!=0)
			continue;
		printf("Disk: rd%d: %d KB\n",i,rdsize(makedev(i,0)));
		bp = read_mbr(i);
		if(!bp)
			continue;
		mbr = (struct mbr *)bp->b_addr;
		for(j=1; j<5; j++)
		{
			if(mbr->partitions[j-1].type==type)
			{
				printf("  rd%d%c: %d KB\n",i,'a'+j-1,rdsize(makedev(i,j)));
			}
		}
		brelse(bp);
	}
}

int rdisk_num_disks()
{
	return NRDSK;
}

dev_t get_boot_device()
{
// If a root device has been specified, then we can short cut all this and just
// use that device.
#ifdef ROOT
	return ROOT;
#else
	dev_t bd = -1;
	int i,j,e;
	struct buf *bp;
	struct mbr *mbr;

	for(i=0; i<NRDSK; i++)
	{
		e = rdopen(makedev(i,0),0,S_SILENT);
		if(e==0)
		{
			bp = read_mbr(i);
			if(bp)
			{
				mbr = (struct mbr *)bp->b_addr;

				for(j=0; j<4; j++)
				{
					if(mbr->partitions[j].type==RDISK_FS)
					{
						if(mbr->partitions[j].status & P_ACTIVE)
						{
							brelse(bp);
							rdclose(makedev(i,0),0,0);
							return makedev(i,j+1);
						}
					}
				}
				brelse(bp);
			}
			rdclose(makedev(i,0),0,0);
		}
	}
	
	return bd;
#endif
}

dev_t get_swap_device()
{
// If a swap device has been specified, then we can short cut all this and just
// use that device.
#ifdef SWAP
	return SWAP;
#else

	dev_t bd = -1;
	int i,j,e;
	unsigned int max_size = 0;
	struct buf *bp;
	struct mbr *mbr;

	// First we look for the first active swap device

	for(i=0; i<NRDSK; i++)
	{
		e = rdopen(makedev(i,0),0,S_SILENT);
		if(e==0)
		{
			bp = read_mbr(i);
			if(bp)
			{
				mbr = (struct mbr *)bp->b_addr;

				for(j=0; j<4; j++)
				{
					if(mbr->partitions[j].type==RDISK_SWAP)
					{
						// If this partition is the biggest so far
						// then store it.  We'll use this if
						// there is no active partition.
						if(mbr->partitions[j].lbalength>max_size)
						{
							max_size = mbr->partitions[j].lbalength;
							bd = makedev(i,j+1);
						}

						// If it is active, then use it.
						if(mbr->partitions[j].status & P_ACTIVE)
						{
							brelse(bp);
							rdclose(makedev(i,0),0,0);
							return makedev(i,j+1);
						}
					}
				}
				brelse(bp);
			}
			rdclose(makedev(i,0),0,0);
		}
	}

	// There is no active partition, so we'll use the biggest one we found.
	return bd;
#endif
}

unsigned char partition_type(dev_t dev)
{
	struct buf *bp;
	struct mbr *mbr;
	unsigned char pt;

	if(minor(dev)<1 || minor(dev)>4)
		return 0;
	if(rdopen(dev,0,S_SILENT)==0)
	{
		bp = read_mbr(major(dev));
		rdclose(dev,0,0);
		if(!bp)
		{
			brelse(bp);
			return 0;
		}
		mbr = (struct mbr *)bp->b_addr;
		pt = mbr->partitions[minor(dev)-1].type;
		brelse(bp);
		return pt;
	}
	return 0;
}

int atoi(char *s)
{
	int i = 0;
	char *q;
	for(q=s; *q; q++)
	{
		if(*q < '0' || *q > '9')
		{
			return i;
		}
		i = i * 10;
		i += *q - '0';
	}
	return i;
}

int strcmp(char *s1, char *s2)
{
	char *p1,*p2;
	p1 = s1;
	p2 = s2;
	while(*p1 && *p2)
	{
		if(*p1 < *p2) return -1;
		if(*p1 > *p2) return 1;
		p1++;
		p2++;
	}
    if(*p1 < *p2) return -1;
    if(*p1 > *p2) return 1;
	return 0;
}

/*

Prepartition format:

mrams0:fs@1024,swap@2048,fs@1022 sdramp0:...

*/

struct buf *prepartition_device(char *devname)
{

#ifndef PARTITION
	char *prepartition_schema = "";
#else
	char *prepartition_schema = (char *)QUOTE(PARTITION);
#endif
	char *p,*q;
	struct buf *bp;
	struct mbr *mbr;
	int pnum = 0;
	int start = 2;
	char dev[9];
	char size[9];
	char type[5];
	int pos = 0;

	p = prepartition_schema+1;
	q = p;

	printf("PP Schema: %s\n",prepartition_schema);

	bp = getnewbuf();
	if(!bp)
	{
		return NULL;
	}

	mbr = (struct mbr *)bp->b_addr;

	while(*p)
	{
		while(*q && *q != ' ')
			q++;
		if(*q == ' ')
		{
			*q = 0;
			q++;
		}

		pos = 0;
		while((*p) && (*p != ':'))
		{
			dev[pos++] = *p;
			dev[pos] = 0;
			p++;
		}
	
		if((*p) == ':')
		{
			p++;
		} else {
			printf("Device Format Error (%c)\n",*p);
			brelse(bp);
			return NULL;
		}

		while((*p) && (*p != ' '))
		{
			pos = 0;
			while((*p) && (*p != '@'))
			{
				type[pos++] = *p;
				type[pos] = 0;
				p++;
			}

			if((*p) == '@')
			{
				p++;
			} else {
				printf("Type Format Error\n");
				brelse(bp);
				return NULL;
			}

			pos = 0;
			while((*p) && (*p != ',') && (*p != ' ') && (*p != ')'))
			{
				size[pos++] = *p;
				size[pos] = 0;
				p++;
			}

			printf("Found partition on %s of type %s and size %s\n",
				dev,type,size);

			if(!strcmp(dev,devname))
			{
				if(!strcmp("sw",type))
					mbr->partitions[pnum].type=RDISK_SWAP;
				if(!strcmp("sa",type))
				{
					mbr->partitions[pnum].type=RDISK_SWAP;
					mbr->partitions[pnum].status = 0x80;
				}
				if(!strcmp("fs",type))
					mbr->partitions[pnum].type=RDISK_FS;

				mbr->partitions[pnum].lbastart = start;
				mbr->partitions[pnum].lbalength = atoi(size)<<1;
				start += mbr->partitions[pnum].lbalength;
				pnum++;
			}
			p++;
		}
	}
	
	if(pnum > 0)
	{
		mbr->bootsig = 0xAA55;
		mbr->biosdrive = 0x80;
		mbr->sig = 'R'<<24 | 'T'<<16 | 'E'<<8 | 'R';
		return bp;
	}

	brelse(bp);
	return NULL;
}
