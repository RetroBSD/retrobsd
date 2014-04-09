#include <stdio.h>
#include <stdlib.h>

/*
 * la: print the load average.
 */
int main()
{
	unsigned vec[3];

	if (getloadavg(vec, 3) < 0) {
		perror("la: getloadavg");
		return 1;
	}
	printf("load %u.%02u %u.%02u %u.%02u\n",
                vec[0] / 100, vec[0] % 100,
                vec[1] / 100, vec[1] % 100,
                vec[2] / 100, vec[2] % 100);
	return 0;
}
