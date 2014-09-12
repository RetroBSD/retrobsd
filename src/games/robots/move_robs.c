/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "robots.h"
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

/*
 * sign:
 *	Return the sign of the number
 */
static int
sign(n)
int	n;
{
	if (n < 0)
		return -1;
	else if (n > 0)
		return 1;
	else
		return 0;
}

/*
 * move_robots:
 *	Move the robots around
 */
void
move_robots(was_sig)
int	was_sig;
{
	register COORD	*rp;

	if (Real_time)
		signal(SIGALRM, move_robots);
#ifdef DEBUG
	move(Min.y, Min.x);
	addch(inch());
	move(Max.y, Max.x);
	addch(inch());
#endif
	for (rp = Robots; rp < &Robots[MAXROBOTS]; rp++) {
		if (rp->y < 0)
			continue;
		mvaddch(rp->y, rp->x, ' ');
		Field[rp->y][rp->x]--;
		rp->y += sign(My_pos.y - rp->y);
		rp->x += sign(My_pos.x - rp->x);
		if (rp->y <= 0)
			rp->y = 0;
		else if (rp->y >= Y_FIELDSIZE)
			rp->y = Y_FIELDSIZE - 1;
		if (rp->x <= 0)
			rp->x = 0;
		else if (rp->x >= X_FIELDSIZE)
			rp->x = X_FIELDSIZE - 1;
		Field[rp->y][rp->x]++;
	}

	Min.y = Y_FIELDSIZE;
	Min.x = X_FIELDSIZE;
	Max.y = 0;
	Max.x = 0;
	for (rp = Robots; rp < &Robots[MAXROBOTS]; rp++)
		if (rp->y < 0)
			continue;
		else if (rp->y == My_pos.y && rp->x == My_pos.x)
			Dead = TRUE;
		else if (Field[rp->y][rp->x] > 1) {
			mvaddch(rp->y, rp->x, HEAP);
			rp->y = -1;
			Num_robots--;
			if (Waiting)
				Wait_bonus++;
			add_score(ROB_SCORE);
		}
		else {
			mvaddch(rp->y, rp->x, ROBOT);
			if (rp->y < Min.y)
				Min.y = rp->y;
			if (rp->x < Min.x)
				Min.x = rp->x;
			if (rp->y > Max.y)
				Max.y = rp->y;
			if (rp->x > Max.x)
				Max.x = rp->x;
		}

	if (was_sig) {
		refresh();
		if (Dead || Num_robots <= 0)
			longjmp(End_move, 1);
	}

#ifdef DEBUG
	standout();
	move(Min.y, Min.x);
	addch(inch());
	move(Max.y, Max.x);
	addch(inch());
	standend();
#endif
	if (Real_time)
		alarm(3);
}

/*
 * add_score:
 *	Add a score to the overall point total
 */
void
add_score(add)
int	add;
{
	Score += add;
	move(Y_SCORE, X_SCORE);
	printw("%d", Score);
}
