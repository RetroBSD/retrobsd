/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#ifdef CROSS
char *recfile = "/usr/local/games/snakerawscores";
#else
char *recfile = "/games/lib/snakerawscores";
#endif

#define MAXPLAYERS 256

struct	player	{
	short	uids;
	short	scores;
	char	*name;
} players[MAXPLAYERS], temp;

int main()
{
	short	uid, score;
	FILE	*fd;
	int	noplayers = 0;
	int	i, j, notsorted;
	short	whoallbest, allbest;
	char	*q;
	struct	passwd	*p;

	fd = fopen(recfile, "r");
	if (fd == NULL) {
		perror(recfile);
		exit(1);
	}
	printf("Snake players scores to date\n");
	if (! fread(&whoallbest, sizeof(short), 1, fd) ||
            ! fread(&allbest, sizeof(short), 1, fd)) {
                printf("error reading scores\n");
                exit(2);
        }
	for (uid=2;;uid++) {
		if(! fread(&score, sizeof(short), 1, fd))
			break;
		if (score > 0) {
			if (noplayers > MAXPLAYERS) {
				printf("too many players\n");
				exit(2);
			}
			players[noplayers].uids = uid;
			players[noplayers].scores = score;
			p = getpwuid(uid);
			if (p == NULL)
				continue;
			q = p -> pw_name;
			players[noplayers].name = malloc(strlen(q)+1);
			strcpy(players[noplayers].name, q);
			noplayers++;
		}
	}

	/* bubble sort scores */
	for (notsorted=1; notsorted; ) {
		notsorted = 0;
		for (i=0; i<noplayers-1; i++)
			if (players[i].scores < players[i+1].scores) {
				temp = players[i];
				players[i] = players[i+1];
				players[i+1] = temp;
				notsorted++;
			}
	}

	j = 1;
	for (i=0; i<noplayers; i++) {
		printf("%d:\t$%d\t%s\n", j, players[i].scores, players[i].name);
		if (players[i].scores > players[i+1].scores)
			j = i+2;
	}
	exit(0);
}
