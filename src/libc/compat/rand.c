/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
static	long	randx = 1;

void
srand(x)
        unsigned x;
{
	randx = x;
}

int
rand()
{
        randx = randx * 1103515245 + 12345;
	return (randx >> 16) & 0x7fff;
}
