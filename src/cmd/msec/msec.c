#include <stdio.h>
#include <sys/time.h>

void main()
{
	while(1)
	{
		printf("ms: %u\r",msec());
	}
}
