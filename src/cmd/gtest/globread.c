
#include <stdio.h>
#include <sys/glob.h>

int main(int argc, char *argv[])
{
	int addr;
	if(argc != 2)
	{
		printf("Usage: %s <address>\n",argv[0]);
		return(10);
	}
	addr = atoi(argv[1]);
	printf("Byte at address %d: %d\n",addr,rdglob(addr));
	return 0;
}
