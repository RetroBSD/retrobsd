#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "rdisk.h"

#define BUFSZ 512

#define BOOT 1
#define SWAP 2
#define FS 3

void usage()
{
	printf("Usage: mkrd -out <output> [-boot <file>] [-swap <file>] [-fs <file>]\n");
	exit(10);
}

int main(int argc, char *argv[])
{
	struct mbr mbr;
	int i;
	struct stat sb;
	int pno = 0;
	unsigned int start = 2;
	int fd,fin;
	unsigned int r;
	char buf[BUFSZ];
	unsigned int tl;
	unsigned char ok = 0;
    int q __attribute__((unused));

	char *output = NULL;
	char *files[4] = {NULL,NULL,NULL,NULL};
	char types[4] = {0,0,0,0};

	int argnum = 1;

	bzero(&mbr,sizeof(struct mbr));

	mbr.biosdrive=0x80;
	mbr.sig = 'R'<<24 | 'T'<<16 | 'E'<<8 | 'R';
	mbr.bootsig = 0xAA55;

	for(argnum=1; argnum<argc; argnum++)
	{
		ok = 0;
		if(!strcmp(argv[argnum],"-out"))
		{
			argnum++;
			if(argnum>argc) usage();
			output = argv[argnum];
			printf("Output: %s\n",output);
			ok = 1;
		}

		if(!strcmp(argv[argnum],"-boot"))
		{
			argnum++;
			if(argnum>argc) usage();
			files[pno] = argv[argnum];
			types[pno] = BOOT;
			ok = 1;
			printf("Boot: %s\n",files[pno]);
			pno++;
		}

		if(!strcmp(argv[argnum],"-swap"))
		{
			argnum++;
			if(argnum>argc) usage();
			files[pno] = argv[argnum];
			types[pno] = SWAP;
			ok = 1;
			printf("Swap: %s\n",files[pno]);
			pno++;
		}

		if(!strcmp(argv[argnum],"-fs"))
		{
			argnum++;
			if(argnum>argc) usage();
			files[pno] = argv[argnum];
			types[pno] = FS;
			ok = 1;
			printf("Fs: %s\n",files[pno]);
			pno++;
		}

		if(ok==0)
		{
			printf("Unknown option %s\n",argv[argnum]);
			usage();
		}
	}

	if(!output) usage();

	for(i=0; i<4; i++)
	{
		if(types[i]!=0)
		{
			if(stat(files[i],&sb)==-1)
			{
				printf("Error: unable to stat %s\n",argv[i]);
				exit(10);
			}
			tl = sb.st_size;
			if(tl%BUFSZ != 0)
			{
				tl = (tl >> 9) + 1;
			} else {
				tl = tl >> 9;
			}

			mbr.partitions[i].lbastart = start;
			mbr.partitions[i].lbalength = tl;
			switch(types[i])
			{
				case BOOT:
					mbr.partitions[i].type = RDISK_FS;
					mbr.partitions[i].status = 0x80;
					break;
				case SWAP:
					mbr.partitions[i].type = RDISK_SWAP;
					mbr.partitions[i].status = 0x00;
					break;
				case FS:
					mbr.partitions[i].type = RDISK_FS;
					mbr.partitions[i].status = 0x00;
					break;
			}

			printf("Partition %d, start %u, length %u\n",
				i,
				mbr.partitions[i].lbastart,
				mbr.partitions[i].lbalength
			);

			start += tl;
		}
	}
	fd = open(output,O_WRONLY|O_CREAT|O_TRUNC,0666);
	if(!fd)
	{
		printf("Error: cannot open %s for writing\n",argv[1]);
		exit(10);
	}

	q = write(fd,&mbr,BUFSZ);

	for(i=0; i<4; i++)
	{
		if(types[i]!=0)
		{
			lseek(fd,mbr.partitions[i].lbastart<<9,SEEK_SET);
			printf("Loading data from %s...\n",files[i]);
			fin = open(files[i],O_RDONLY);
			bzero(buf,BUFSZ);
			while((r=read(fin,buf,BUFSZ))>0)
			{
				q = write(fd,buf,BUFSZ);
				bzero(buf,BUFSZ);
			}
			close(fin);
		}
	}

	close(fd);
	return 0;
}
