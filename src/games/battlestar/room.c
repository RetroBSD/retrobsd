/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */
#include "externs.h"

#ifdef EXT_MESSAGE_FILE
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>

static int desc = -1;

void
writedes()
{
	int compass;
	register char *p;
	register int c;
	char buf[BUFSIZ];

	if (desc < 0) {
		desc = open(BATTLESTRINGS, O_RDONLY, 0666);
                if (desc < 0) {
                        perror(BATTLESTRINGS);
                        exit(1);
                }
        }

	lseek(desc, (off_t) location[position].name, L_SET);
	if (read(desc, buf, sizeof(buf)) != sizeof(buf)) /*ignore*/;
	printf("\n\t%s\n", buf);
	if (beenthere[position] < 3) {
		compass = NORTH;
		lseek(desc, (off_t) location[position].desc, L_SET);
		if (read(desc, buf, sizeof(buf)) != sizeof(buf)) /*ignore*/;
		p = buf;
		while ((c = *p++)) {
			if (c != '-' && c != '*' && c != '+')
				putchar(c);
			else {
				if (c != '*')
					printf("%s", truedirec(compass, c));
				compass++;
			}
                }
	}
}

void
printobjs()
{
	register unsigned int *p = location[position].objects;
	register int n;
	char buf[BUFSIZ];

	printf("\n");
	for (n = 0; n < NUMOFOBJECTS; n++)
		if (testbit(p, n) && objdes[n]) {
			lseek(desc, (off_t) objdes[n], L_SET);
			if (read(desc, buf, sizeof(buf)) != sizeof(buf)) /*ignore*/;
			puts(buf);
		}
}

void
strprt(n)
        int n;
{
	char buf[BUFSIZ];

	if (lseek(desc, (off_t) objdes[n], L_SET) != objdes[n] ||
            read(desc, buf, sizeof(buf)) != sizeof(buf)) {
                perror(BATTLESTRINGS);
                exit(1);
        }
	printf("%s\n", buf);
}

#else /* EXT_MESSAGE_FILE */

void
writedes()
{
	int compass;
	register char *p;
	register int c;

	printf("\n\t%s\n", location[position].name);
	if (beenthere[position] < 3) {
		compass = NORTH;
		p = location[position].desc;
		while ((c = *p++) != 0) {
			if (c != '-' && c != '*' && c != '+')
				putchar(c);
			else {
				if (c != '*')
					printf("%s", truedirec(compass, c));
				compass++;
			}
                }
	}
}

void
printobjs()
{
	register unsigned int *p = location[position].objects;
	register int n;

	printf("\n");
	for (n = 0; n < NUMOFOBJECTS; n++)
		if (testbit(p, n) && objdes[n])
			puts(objdes[n]);
}
#endif /* EXT_MESSAGE_FILE */

void
whichway(here)
        struct room here;
{
	switch(direction) {

		case NORTH:
			left = here.west;
			right = here.east;
			ahead = here.north;
			back = here.south;
			break;

		case SOUTH:
			left = here.east;
			right = here.west;
			ahead = here.south;
			back = here.north;
			break;

		case EAST:
			left = here.north;
			right = here.south;
			ahead = here.east;
			back = here.west;
			break;

		case WEST:
			left = here.south;
			right = here.north;
			ahead = here.west;
			back = here.east;
			break;

	}
}

char *
truedirec(way, option)
        int way;
        char option;
{
	switch(way) {

		case NORTH:
			switch(direction) {
				case NORTH:
					return("ahead");
				case SOUTH:
					return(option == '+' ? "behind you" : "back");
				case EAST:
					return("left");
				case WEST:
					return("right");
			}

		case SOUTH:
			switch(direction) {
				case NORTH:
					return(option == '+' ? "behind you" : "back");
				case SOUTH:
					return("ahead");
				case EAST:
					return("right");
				case WEST:
					return("left");
			}

		case EAST:
			switch(direction) {
				case NORTH:
					return("right");
				case SOUTH:
					return("left");
				case EAST:
					return("ahead");
				case WEST:
					return(option == '+' ? "behind you" : "back");
			}

		case WEST:
			switch(direction) {
				case NORTH:
					return("left");
				case SOUTH:
					return("right");
				case EAST:
					return(option == '+' ? "behind you" : "back");
				case WEST:
					return("ahead");
			}

		default:
			printf("Error: room %d.  More than four directions wanted.", position);
			return("!!");
      }
}

void
newway(thisway)
        int thisway;
{
	switch(direction){

		case NORTH:
			switch(thisway){
				case LEFT:
					direction = WEST;
					break;
				case RIGHT:
					direction = EAST;
					break;
				case BACK:
					direction = SOUTH;
					break;
			}
			break;
		case SOUTH:
			switch(thisway){
				case LEFT:
					direction = EAST;
					break;
				case RIGHT:
					direction = WEST;
					break;
				case BACK:
					direction = NORTH;
					break;
			}
			break;
		case EAST:
			switch(thisway){
				case LEFT:
					direction = NORTH;
					break;
				case RIGHT:
					direction = SOUTH;
					break;
				case BACK:
					direction = WEST;
					break;
			}
			break;
		case WEST:
			switch(thisway){
				case LEFT:
					direction = SOUTH;
					break;
				case RIGHT:
					direction = NORTH;
					break;
				case BACK:
					direction = EAST;
					break;
			}
			break;
      }
}
