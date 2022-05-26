#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int
freeze(s)
        char *s;
{
        int fd;
	size_t len;

	len = (size_t) sbrk(0);
	if((fd = creat(s, 0666)) < 0) {
		perror(s);
		return(1);
	}
	write(fd, &len, sizeof(len));
	write(fd, (char *)0, len);
	close(fd);
	return(0);
}

int
thaw(s)
        char *s;
{
        int fd;
	unsigned int *len;

	if(*s == 0) {
		fprintf(stderr, "empty restore file\n");
		return(1);
	}
	if((fd = open(s, 0)) < 0) {
		perror(s);
		return(1);
	}
	read(fd, &len, sizeof(len));
	(void) brk(len);
	read(fd, (char *)0, len);
	close(fd);
	return(0);
}
