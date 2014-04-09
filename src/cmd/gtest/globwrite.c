
#include <stdio.h>
#include <sys/glob.h>

int main(int argc, char *argv[])
{
	int addr,val;
	if(argc != 3)
	{
		printf("Usage: %s <address> <value>\n",argv[0]);
		return(10);
	}
	addr = atoi(argv[1]);
	val = atoi(argv[2]);
	wrglob(addr,val&0xFF);
}
