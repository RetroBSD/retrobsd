/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "robots.h"
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
	int	s_uid;
	int	s_score;
	char	s_name[MAXNAME];
} SCORE;

typedef struct passwd	PASSWD;

char	*Scorefile = SCOREFILE;

int	Max_per_uid = MAX_PER_UID;

static SCORE	Top[MAXSCORES];

/*
 * cmp_sc:
 *	Compare two scores.
 */
static int
cmp_sc(a1, a2)
const void *a1, *a2;
{
        const SCORE *s1 = a1;
        const SCORE *s2 = a2;

	return s2->s_score - s1->s_score;
}

static void
set_name(scp)
register SCORE	*scp;
{
	register PASSWD	*pp;

	if ((pp = getpwuid(scp->s_uid)) == NULL)
		pp->pw_name = "???";
	strncpy(scp->s_name, pp->pw_name, MAXNAME);
}

/*
 * score:
 *	Post the player's score, if reasonable, and then print out the
 *	top list.
 */
void
score()
{
	register int	inf;
	register SCORE	*scp;
	register int	uid;
	register bool	done_show = FALSE;
	static int	numscores, max_uid;

	Newscore = FALSE;
	if ((inf = open(Scorefile, 2)) < 0) {
		perror(Scorefile);
		return;
	}

	if (read(inf, &max_uid, sizeof max_uid) == sizeof max_uid) {
		if (read(inf, Top, sizeof Top) != sizeof Top) {
		        perror("Error reading top scores");
			exit(-1);
		}
	} else {
		for (scp = Top; scp < &Top[MAXSCORES]; scp++)
			scp->s_score = -1;
		max_uid = Max_per_uid;
	}

	uid = getuid();
	if (Top[MAXSCORES-1].s_score <= Score) {
		numscores = 0;
		for (scp = Top; scp < &Top[MAXSCORES]; scp++)
			if (scp->s_score < 0 ||
			    (scp->s_uid == uid && ++numscores == max_uid)) {
				if (scp->s_score > Score)
					break;
				scp->s_score = Score;
				scp->s_uid = uid;
				set_name(scp);
				Newscore = TRUE;
				break;
			}
		if (scp == &Top[MAXSCORES]) {
			Top[MAXSCORES-1].s_score = Score;
			Top[MAXSCORES-1].s_uid = uid;
			set_name(&Top[MAXSCORES-1]);
			Newscore = TRUE;
		}
		if (Newscore)
			qsort(Top, MAXSCORES, sizeof Top[0], cmp_sc);
	}

	if (!Newscore) {
		Full_clear = FALSE;
		close(inf);
		return;
	}
	else
		Full_clear = TRUE;

	for (scp = Top; scp < &Top[MAXSCORES]; scp++) {
		if (scp->s_score < 0)
			break;
		move((scp - Top) + 1, 15);
		if (!done_show && scp->s_uid == uid && scp->s_score == Score)
			standout();
		printw(" %d\t%d\t%-8.8s ", (scp - Top) + 1, scp->s_score, scp->s_name);
		if (!done_show && scp->s_uid == uid && scp->s_score == Score) {
			standend();
			done_show = TRUE;
		}
	}
	Num_scores = scp - Top;
	refresh();

	if (Newscore) {
		lseek(inf, 0L, 0);
		if (write(inf, &max_uid, sizeof max_uid) != sizeof max_uid) {
		        perror("Error writing max uid");
			exit(-1);
		}
		if (write(inf, Top, sizeof Top) != sizeof Top) {
		        perror("Error writing top scores");
			exit(-1);
		}
	}
	close(inf);
}

/*
 * show_score:
 *	Show the score list for the '-s' option.
 */
void
show_score()
{
	register SCORE	*scp;
	register int	inf;
	static int	max_score;

	if ((inf = open(Scorefile, 0)) < 0) {
		perror(Scorefile);
		return;
	}

	for (scp = Top; scp < &Top[MAXSCORES]; scp++)
		scp->s_score = -1;

	if (read(inf, &max_score, sizeof max_score) != sizeof max_score) {
                perror("Error reading max scores");
                exit(-1);
        }
	if (read(inf, Top, sizeof Top) != sizeof Top) {
                perror("Error reading top scores");
                exit(-1);
        }
	close(inf);
	inf = 1;
	for (scp = Top; scp < &Top[MAXSCORES]; scp++)
		if (scp->s_score >= 0)
			printf("%d\t%d\t%.*s\n", inf++, scp->s_score,
                                (int)sizeof scp->s_name, scp->s_name);
}
