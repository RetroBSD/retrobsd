#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "debug.h"

extern unsigned char flash_data[]      asm("_binary_flash_img_start");
extern unsigned char flash_data_size[] asm("_binary_flash_img_size");
extern unsigned char flash_data_end[]  asm("_binary_flash_img_end");

/*
 * Query disk size, for swapping.
 */
daddr_t flash_size(int unit)
{
	size_t fsize = (size_t)((void *)flash_data_size)>>10;


	DEBUG3("flash%d: %u kbytes\n", unit, fsize);
	return fsize;
}

int flash_open (int unit, int flag, int mode)
{
        if (unit > 0)
                return ENXIO;
	return 0;
}

int flash_read(int unit, unsigned int offset, char *buf, unsigned int bcount)
{
	unsigned int i;
	if(unit>0)
		return ENXIO;

	offset = offset<<10;
	if(!buf)
		return 0;

	for(i=0; i<bcount; i++)
		buf[i] = flash_data[offset+i];

	return 0;
}

int flash_write(int unit, unsigned int offset, char *buf, unsigned int bcount)
{
	return 0;
}

void flash_init(int unit, int flag)
{
	if(!(flag & S_SILENT)) printf("flash%d: %d bytes\n",unit,flash_size(unit)*1024);
	flash_read(0,69,NULL,512);
}
