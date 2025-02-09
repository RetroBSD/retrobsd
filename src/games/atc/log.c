/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */
#include "include.h"

int compar(a, b)
	SCORE	*a, *b;
{
	if (b->planes == a->planes)
		return (b->time - a->time);
	else
		return (b->planes - a->planes);
}

#define SECAMIN		60
#define MINAHOUR	60
#define HOURADAY	24
#define SECAHOUR	(SECAMIN * MINAHOUR)
#define SECADAY		(SECAHOUR * HOURADAY)
#define DAY(t)		((t) / SECADAY)
#define HOUR(t)		(((t) % SECADAY) / SECAHOUR)
#define MIN(t)		(((t) % SECAHOUR) / SECAMIN)
#define SEC(t)		((t) % SECAMIN)

char	*
timestr(int t)
{
	static char	s[80];

	if (DAY(t) > 0)
		(void)sprintf(s, "%dd+%02dhrs", DAY(t), HOUR(t));
	else if (HOUR(t) > 0)
		(void)sprintf(s, "%d:%02d:%02d", HOUR(t), MIN(t), SEC(t));
	else if (MIN(t) > 0)
		(void)sprintf(s, "%d:%02d", MIN(t), SEC(t));
	else if (SEC(t) > 0)
		(void)sprintf(s, ":%02d", SEC(t));
	else
		*s = '\0';

	return (s);
}

void log_score(int list_em)
{
	register int	i, fd, num_scores = 0, good, changed = 0, found = 0;
	struct passwd	*pw;
	FILE		*fp;
	char		*cp, logstr[BUFSIZ];
	SCORE		score[100], thisscore;
	struct utsname	name;

	strcpy(logstr, SPECIAL_DIR);
	strcat(logstr, LOG);

	umask(0);
	fd = open(logstr, O_CREAT|O_RDWR, 0644);
	if (fd < 0) {
		perror(logstr);
		return;
	}
	/*
	 * This is done to take advantage of stdio, while still
	 * allowing a O_CREAT during the open(2) of the log file.
	 */
	fp = fdopen(fd, "r+");
	if (fp == NULL) {
		perror(logstr);
		return;
	}
#ifdef BSD
	if (flock(fileno(fp), LOCK_EX) < 0)
#endif
#ifdef SYSV
	while (lockf(fileno(fp), F_LOCK, 1) < 0)
#endif
	{
		perror("flock");
		return;
	}
	for (;;) {
		good = fscanf(fp, "%s %s %s %d %d %d",
			score[num_scores].name,
			score[num_scores].host,
			score[num_scores].game,
			&score[num_scores].planes,
			&score[num_scores].time,
			&score[num_scores].real_time);
		if (good != 6 || ++num_scores >= NUM_SCORES)
			break;
	}
	if (!test_mode && !list_em) {
		if ((pw = (struct passwd *) getpwuid(getuid())) == NULL) {
			fprintf(stderr,
				"getpwuid failed for uid %d.  Who are you?\n",
				getuid());
			return;
		}
		strcpy(thisscore.name, pw->pw_name);
		uname(&name);
		strcpy(thisscore.host, name.sysname);

		cp = strrchr(file, '/');
		if (cp == NULL) {
			fprintf(stderr, "log: where's the '/' in %s?\n", file);
			return;
		}
		cp++;
		strcpy(thisscore.game, cp);

		thisscore.time = clocktick;
		thisscore.planes = safe_planes;
		thisscore.real_time = time(0) - start_time;

		for (i = 0; i < num_scores; i++) {
			if (strcmp(thisscore.name, score[i].name) == 0 &&
			    strcmp(thisscore.host, score[i].host) == 0 &&
			    strcmp(thisscore.game, score[i].game) == 0) {
				if (thisscore.time > score[i].time) {
					score[i].time = thisscore.time;
					score[i].planes = thisscore.planes;
					score[i].real_time =
						thisscore.real_time;
					changed++;
				}
				found++;
				break;
			}
		}
		if (!found) {
			for (i = 0; i < num_scores; i++) {
				if (thisscore.time > score[i].time) {
					if (num_scores < NUM_SCORES)
						num_scores++;
					bcopy(&score[i],
						&score[num_scores - 1],
						sizeof (score[i]));
					bcopy(&thisscore, &score[i],
						sizeof (score[i]));
					changed++;
					break;
				}
			}
		}
		if (!found && !changed && num_scores < NUM_SCORES) {
			bcopy(&thisscore, &score[num_scores],
				sizeof (score[num_scores]));
			num_scores++;
			changed++;
		}

		if (changed) {
			if (found)
				puts("You beat your previous score!");
			else
				puts("You made the top players list!");
			qsort(score, num_scores, sizeof (*score), compar);
			rewind(fp);
			for (i = 0; i < num_scores; i++)
				fprintf(fp, "%s %s %s %d %d %d\n",
					score[i].name, score[i].host,
					score[i].game, score[i].planes,
					score[i].time, score[i].real_time);
		} else {
			if (found)
				puts("You didn't beat your previous score.");
			else
				puts("You didn't make the top players list.");
		}
		putchar('\n');
	}
#ifdef BSD
	flock(fileno(fp), LOCK_UN);
#endif
#ifdef SYSV
	/* lock will evaporate upon close */
#endif
	fclose(fp);
	printf("%2s:  %-8s  %-8s  %-18s  %4s  %9s  %4s\n", "#", "name", "host",
		"game", "time", "real time", "planes safe");
	puts("-------------------------------------------------------------------------------");
	for (i = 0; i < num_scores; i++) {
		cp = strchr(score[i].host, '.');
		if (cp != NULL)
			*cp = '\0';
		printf("%2d:  %-8s  %-8s  %-18s  %4d  %9s  %4d\n", i + 1,
			score[i].name, score[i].host, score[i].game,
			score[i].time, timestr(score[i].real_time),
			score[i].planes);
	}
	putchar('\n');
}
