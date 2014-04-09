
#include <stdio.h>
#include <sys/glob.h>

int main(int argc, char *argv[])
{
	int addr = 0;
	int byte;

	while((byte = rdglob(addr))>=0)
	{
		if(addr%16 == 0)
		printf("\n%04X : ",addr);
		printf("%02X ",byte);
		addr++;
	}
	printf("\n");
	return 0;
}
