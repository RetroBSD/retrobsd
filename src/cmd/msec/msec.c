#include <stdio.h>
#include <sys/time.h>

int main()
{
	while(1)
	{
		printf("ms: %u\r",msec());
	}
}
