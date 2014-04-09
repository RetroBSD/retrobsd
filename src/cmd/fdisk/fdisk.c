/*
 * Simple fdisk program for RetroBSD
 * (c) 2012 Majenko Technologies
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/rdisk.h>
#include <ioctl.h>
#include <sys/stat.h>
#include <string.h>

#include "fdisk.h"

struct mbr mbr;
int blocks;

int strtonum(char *s)
{
	if(!s)
		return 0;
	if(s[0]==0)
		return 0;
	
	if(s[0]=='0' && s[1]=='x')
	{
		return strtol(s+2,NULL,16);
	}
	return atoi(s);
}

void usage()
{
	printf("Usage: fdisk -p /dev/rdX\n");
	printf("          Print partition table\n\n");
	printf("       fdisk -d /dev/rdX <num>\n");
	printf("          Delete partition <num>\n\n");
	printf("       fdisk [-w] -n /dev/rdX [<KB>]\n");
	printf("          Add a new partition at the end of the\n");
	printf("          partition table.  If size is specified\n");
	printf("          creates it of that size, otherwise fills\n");
	printf("          space to end of disk.  If you specify -w\n");
	printf("          then the MBR is wiped before partition\n");
	printf("          creation.\n");
	printf("       fdisk -a /dev/rdX <num>\n");
	printf("          Toggle active flag on partition\n");
	printf("       fdisk -T -t <type> /dev/rdX <num>\n");
	printf("          Set the type of a partition.\n");
}

int read_mbr(int fd)
{
	lseek(fd,0,SEEK_SET);
        if(read(fd,&mbr,sizeof(struct mbr)) != sizeof(struct mbr))
        {
                printf("Error reading MBR\n");
                return -1;
        }
	return 0;
}

int write_mbr(int fd)
{
	lseek(fd,0,SEEK_SET);
	if(write(fd,&mbr,sizeof(struct mbr)) != sizeof(struct mbr))
	{
		printf("Error writing MBR\n");
		return -1;
	}
	sync(fd);
	printf("Calling ioctl to reread the partition table\n");
	sleep(1);
	ioctl(fd,RDREINIT);
	close(fd);
	return 0;
}

void set_type(int pnum, int type)
{
	if(pnum<1 || pnum>4)
	{
		printf("Invalid partition number\n");
		exit(10);
	}
	if(mbr.partitions[pnum-1].lbalength>0)
		mbr.partitions[pnum-1].type=type;
}

void print_ptable()
{
	int i;
	printf("Nr    Start   Length Type\n");

	for(i=0; i<4; i++)
	{
		if(mbr.partitions[i].type!=0)
		{
			printf("%2d %8d %8d %02X %c\n",
				i+1,
				mbr.partitions[i].lbastart>>1,
				mbr.partitions[i].lbalength>>1,
				mbr.partitions[i].type,
				mbr.partitions[i].status & P_ACTIVE ? '*' : ' '
			);
		}
	}
}

void delete_part(int num)
{
	if(num < 1 || num > 4)
	{
		printf("Error: invalid partition number\n");
		return;
	}

	if(mbr.partitions[num-1].type==0)
	{
		printf("Error: partition does not exist\n");
		return;
	}

	mbr.partitions[num-1].type = 0;
	mbr.partitions[num-1].lbastart = 0;
	mbr.partitions[num-1].lbalength = 0;
	mbr.partitions[num-1].start.head = 0;
	mbr.partitions[num-1].start.sector = 0;
	mbr.partitions[num-1].start.cyllow = 0;
	mbr.partitions[num-1].start.cylhigh = 0;
	mbr.partitions[num-1].end.head = 0;
	mbr.partitions[num-1].end.sector = 0;
	mbr.partitions[num-1].end.cyllow = 0;
	mbr.partitions[num-1].end.cylhigh = 0;
}

void new_part(unsigned int size, int type)
{
	int num = 0;
	int i;
	int start;

	if(type<=0 || type >255)
	{
		printf("Invalid partition type\n");
		return;
	}

	for(i=4; i>0; i--)
	{
		if(mbr.partitions[i-1].type==0)
		{
			num = i;
		} else {
			break;
		}
	}

	if(num==0)
	{
		printf("Partition table full\n");
		return;
	}
	printf("Partition number %d\n",num);
	if(num==1)
	{
		start = 1;
	} else {
		start = (mbr.partitions[num-2].lbastart>>1) + (mbr.partitions[num-2].lbalength>>1);
	}

	// We must never start a partition at the beginning, or we'll wipe the
	// MBR...
	if(start==0)
		start=1;
	printf("Start: %lu\n",start);

	if(start+size > blocks)
	{
		printf("Partition too  big\n");
		return;
	}

	if(size==0)
	{
		size = blocks - start;
	}

	mbr.partitions[num-1].lbastart = start<<1;
	mbr.partitions[num-1].lbalength = size<<1;
	mbr.partitions[num-1].type = type;
}

void wipe_mbr()
{
	int i;
	for(i=0; i<4; i++)
	{
		mbr.partitions[i].type = 0;
		mbr.partitions[i].lbastart = 0;
		mbr.partitions[i].lbalength = 0;
		mbr.partitions[i].start.head = 0;
		mbr.partitions[i].start.sector = 0;
		mbr.partitions[i].start.cyllow = 0;
		mbr.partitions[i].start.cylhigh = 0;
		mbr.partitions[i].end.head = 0;
		mbr.partitions[i].end.sector = 0;
		mbr.partitions[i].end.cyllow = 0;
		mbr.partitions[i].end.cylhigh = 0;
	}
	mbr.biosdrive=0x80;
	mbr.sig = 'R'<<24 | 'T'<<16 | 'E'<<8 | 'R';
	mbr.bootsig = 0xAA55;
}

void toggle_active(int p)
{
	if(p<1 || p>4)
		return;

	if(mbr.partitions[p-1].status & P_ACTIVE)
	{
		mbr.partitions[p-1].status &= (~P_ACTIVE);
	} else {
		mbr.partitions[p-1].status |= (P_ACTIVE);
	}
}

int main(int argc, char *argv[])
{
	int opt;
	int action = A_NONE;
	int fd;
	char *device;
	int type = 0xB7;
	unsigned char wipe=0;

	while((opt = getopt(argc, argv, "Twpdnt:a")) != -1)
	{
		switch(opt)
		{
			case 'p':
				action = A_PRINT;
				break;
			case 'd':
				action = A_DELETE;
				break;
			case 'a':
				action = A_ACTIVE;
				break;
			case 'n':
				action = A_NEW;
				break;
			case 't':
				type = strtonum(optarg);
				break;
			case 'T':
				action = A_TYPE;
				break;
			case 'w':
				wipe = 1;
				break;
		}
	}

	if(action == A_NONE)
	{
		usage();
		return 10;
	}

	device = argv[optind++];
	if(!device)
	{
		usage();
		return 10;
	}

	fd = open(device,O_RDWR);
	if(!fd)
	{
		printf("Cannot open %d\n",device);
		return 10;
	}

	if(read_mbr(fd)==-1)
	{
		printf("Error reading MBR\n");
		return 10;
	}
	if(mbr.bootsig != 0xAA55)
	{
		printf("Partition table not valid.\n");
		wipe = 1;
	}	

	ioctl(fd,RDGETMEDIASIZE,&blocks);

	printf("%s: %d blocks of 1KB\n",device,blocks);

	switch(action)
	{
		case A_PRINT:
			print_ptable();
			break;
		case A_DELETE:
			delete_part(strtonum(argv[optind]));
			print_ptable();
			write_mbr(fd);
			break;
		case A_NEW:

			if(wipe==1)
				wipe_mbr();
			if(optind<argc)
				new_part(strtonum(argv[optind]),type);
			else
				new_part(0,type);
			print_ptable();
			write_mbr(fd);
			break;
		case A_ACTIVE:
			toggle_active(strtonum(argv[optind]));
			print_ptable();
			write_mbr(fd);
			break;
		case A_TYPE:
			set_type(strtonum(argv[optind]),type);
			print_ptable();
			write_mbr(fd);
			break;
	}

	if(fd)
		close(fd);
	return 0;
}
