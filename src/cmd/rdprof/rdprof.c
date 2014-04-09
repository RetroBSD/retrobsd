#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/rdisk.h>
#include <time.h>

void usage()
{
	printf("Usage: rdprof [-w] [-s <blocksize>] [-r repeat] [-b blocks] -d <device>\n");
	exit(10);
}

int numblocks(int fd)
{
	int size=0;
	ioctl(fd,RDGETMEDIASIZE,&size);
	return size;
}

void read_test(char *dev, unsigned int blocks, int blocksize, int repeats)
{
	int fd;
	int size;
	char buffer[blocksize];
	unsigned int toread;
	unsigned int start_time;
	unsigned int end_time;
	unsigned int time_taken;
	int i;
	float bps;

	fd = open(dev,O_RDONLY);

	if(!fd)
	{
		printf("Error: unable to open %s for reading.\n",dev);
		exit(10);
	}
	size = numblocks(fd);
	if(size==0)
	{
		printf("Error: unable to get media size.\n");
		close(fd);
		exit(0);
	}
	if(blocks>size)
	{
		printf("More blocks requested than media size.  Reducing.\n");
		blocks = size;
	}
	if(blocks==0)
	{
		blocks=size;
	}

	printf("Testing read of %s from 0 to %d with %d byte blocks...\n",
		dev,blocks-1,blocksize);

	start_time = msec();
	for(i=0; i<repeats; i++)
	{
		lseek(fd,0,SEEK_SET);
		toread = blocks * 1024;
		while(toread > 0)
		{
			if(toread>blocksize)
			{
				toread -= read(fd,buffer,blocksize);
			} else {
				toread -= read(fd,buffer,toread);
			}
		}
	}
	end_time = msec();
	close(fd);
	time_taken = end_time - start_time;
	bps = (float)((blocks*repeats) * 1024) / ((float)time_taken / 1000.0);

	printf("Time taken: %f seconds.  Speed: %f KBytes/second\n",
		time_taken / 1000.0, bps/1024.0);
}

void write_test(char *dev, unsigned int blocks, int blocksize, int repeats)
{
	int fd;
	int size;
	char buffer[blocksize];
	unsigned int towrite;
	unsigned int start_time;
	unsigned int end_time;
	unsigned int time_taken;
	int i;
	float bps;

	for(size=0; size<blocksize; size++)
	{
		buffer[size]=size;
	}

	fd = open(dev,O_RDWR);

	if(!fd)
	{
		printf("Error: unable to open %s for writing.\n",dev);
		exit(10);
	}
	size = numblocks(fd);
	if(size==0)
	{
		printf("Error: unable to get media size.\n");
		close(fd);
		exit(0);
	}
	if(blocks>size)
	{
		printf("More blocks requested than media size.  Reducing.\n");
		blocks = size;
	}
	if(blocks==0)
	{
		blocks=size;
	}

	printf("Testing write of %s from 0 to %d with %d byte blocks...\n",
		dev,blocks-1,blocksize);

	start_time = msec();
	for(i=0; i<repeats; i++)
	{
		lseek(fd,0,SEEK_SET);
		towrite = blocks * 1024;
		while(towrite > 0)
		{
			if(towrite>blocksize)
			{
				towrite -= write(fd,buffer,blocksize);
			} else {
				towrite -= write(fd,buffer,towrite);
			}
		}
	}
	end_time = msec();
	close(fd);
	time_taken = end_time - start_time;
        bps = (float)((blocks*repeats) * 1024) / ((float)time_taken / 1000.0);

        printf("Time taken: %f seconds.  Speed: %f KBytes/second\n",
                time_taken / 1000.0, bps/1024.0);
}

void main(int argc, char *argv[])
{
	int opt;
	unsigned char write_enable = 0;
	char *device = NULL;
	unsigned int blocks = 0;
	int blocksize = 1024;
	int repeats = 1;

	printf("RetroDisk Profiler\n");

	while((opt = getopt(argc,argv,"wd:b:s:r:"))!=-1)
	{
		switch(opt)
		{
			case 'w':
				printf("Write tests enabled!\n");
				write_enable=1;
				break;
			case 'd':
				device = optarg;
				break;
			case 'b':
				blocks = atoi(optarg);
				break;
			case 's':
				blocksize = atoi(optarg);
				break;
			case 'r':
				repeats = atoi(optarg);
				break;
		}
	}
	
	if(device==NULL)
	{
		usage();
	}

	read_test(device,blocks,blocksize,repeats);
	if(write_enable==1)
		write_test(device,blocks,blocksize,repeats);
}

