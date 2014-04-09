/* $Header: sm.c,v 7.0 86/10/08 15:13:35 lwall Exp $ */

/* $Log:	sm.c,v $
 * Revision 7.0  86/10/08  15:13:35  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main()
{
    char screen[23][90];
    register int y;
    register int x;
    int tmpy, tmpx;

    for (x=0; x<79; x++)
	screen[0][x] = ' ';
    screen[0][79] = '\0';

    if (fgets(screen[0], 90, stdin) == 0) {
        perror("stdin");
        return 1;
    }
    if (isdigit(screen[0][0])) {
	int numstars = atoi(screen[0]);

	for (y=0; y<23; y++) {
	    for (x=0; x<79; x++)
		screen[y][x] = ' ';
	    screen[y][79] = '\0';
	}

	for ( ; numstars; numstars--) {
	    if (scanf("%d %d\n", &tmpy, &tmpx) != 2) {
                perror("two numbers expected");
                return 1;
            }

	    y = tmpy;
	    x = tmpx;
	    screen[y][x+x] = '*';
	}

	for (y=0; y<23; y++) {
	    printf("%s\n",screen[y]);
	}
    }
    else {
	register int numstars = 0;

	for (y=1; y<23; y++) {
	    for (x=0; x<79; x++)
		screen[y][x] = ' ';
	    screen[y][79] = '\0';
	}

	for (y=1; y<23; y++) {
	    if (fgets(screen[y], 90, stdin) == 0) {
                perror("stdin");
                return 1;
            }
	}

	for (y=0; y<23; y++) {
	    for (x=0; x<80; x += 2) {
		if (screen[y][x] == '*') {
		    numstars++;
		}
		else if (screen[y][x] == '\t' || screen[y][x+1] == '\t') {
		    fprintf(stderr,"Cannot have tabs in starmap--please expand.\n");
		    exit(1);
		}
	    }
	}

	printf("%d\n",numstars);

	for (y=0; y<23; y++) {
	    for (x=0; x<80; x += 2) {
		if (screen[y][x] == '*') {
		    printf("%d %d\n",y,x/2);
		}
	    }
	}
    }
    exit(0);
}
