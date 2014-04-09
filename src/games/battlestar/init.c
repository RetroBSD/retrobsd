/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */
#include "externs.h"
#include <pwd.h>

static char *list[] = {	/* hereditary wizards */
	"riggle",
	"chris",
	"games",
	"edward",
	"comay",
	"yee",
	"dmr",
	"ken",
	0
};

static char *badguys[] = {
	"wnj",
	"root",
	"ted",
	0
};

static void
getutmp(uname)
	char *uname;
{
	struct passwd *ptr;

	ptr = getpwuid(getuid());
	strcpy(uname, ptr ? ptr->pw_name : "");
}

static int
checkout(uname)
	register char *uname;
{
	register char **ptr;

	for (ptr = list; *ptr; ptr++)
		if (strcmp(*ptr, uname) == 0)
			return 1;
	for (ptr = badguys; *ptr; ptr++)
		if (strcmp(*ptr, uname) == 0) {
			printf("You are the Poor anti-wizard %s.  Good Luck!\n",
				uname);
			CUMBER = 3;
			WEIGHT = 9;	/* that'll get him! */
			clk = 10;
			setbit(location[7].objects, WOODSMAN);	/* viper room */
			setbit(location[20].objects, WOODSMAN);	/* laser " */
			setbit(location[13].objects, DARK);	/* amulet " */
			setbit(location[8].objects, ELF);	/* closet */
			return 0;	/* anything else, Chris? */
		}
	return 0;
}

static int
wizard(uname)
	char *uname;
{
	int flag;

	flag = checkout(uname);
	if (flag)
		printf("You are the Great wizard %s.\n", uname);
	return flag;
}

void
initialize(startup)
	int startup;
{
	register struct objs *p;

	puts("Version 4.2, fall 1984.");
	puts("First Adventure game written by His Lordship, the honorable");
	puts("Admiral D.W. Riggle\n");
	srand(getpid());
	getutmp(uname);
	wiz = wizard(uname);
	wordinit();
	if (startup) {
		location = dayfile;
		direction = NORTH;
		Time = 0;
		snooze = CYCLE * 1.5;
		position = 22;
		setbit(wear, PAJAMAS);
		fuel = TANKFULL;
		torps = TORPEDOES;
		for (p = dayobjs; p->room != 0; p++)
			setbit(location[p->room].objects, p->obj);
	} else
		restore();
	signal(SIGINT, die);
}
