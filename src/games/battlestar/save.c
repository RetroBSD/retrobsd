/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */
#include "externs.h"

void
restore()
{
	char *home;
	char home1[100];
	register int n;
	int tmp;
	register FILE *fp;

	home = getenv("HOME");
	strcpy(home1, home);
	strcat(home1, "/Bstar");
	if ((fp = fopen(home1, "r")) == 0) {
fatal:		perror(home1);
		return;
	}
	if (fread(&WEIGHT, sizeof WEIGHT, 1, fp) != 1) goto fatal;
	if (fread(&CUMBER, sizeof CUMBER, 1, fp) != 1) goto fatal;
	if (fread(&clk, sizeof clk, 1, fp) != 1) goto fatal;
	if (fread(&tmp, sizeof tmp, 1, fp) != 1) goto fatal;
	location = tmp ? dayfile : nightfile;
	for (n = 1; n <= NUMOFROOMS; n++) {
		if (fread(location[n].link, sizeof location[n].link, 1, fp) != 1) goto fatal;
		if (fread(location[n].objects, sizeof location[n].objects, 1, fp) != 1) goto fatal;
	}
	if (fread(inven, sizeof inven, 1, fp) != 1) goto fatal;
	if (fread(wear, sizeof wear, 1, fp) != 1) goto fatal;
	if (fread(injuries, sizeof injuries, 1, fp) != 1) goto fatal;
	if (fread(notes, sizeof notes, 1, fp) != 1) goto fatal;
	if (fread(&direction, sizeof direction, 1, fp) != 1) goto fatal;
	if (fread(&position, sizeof position, 1, fp) != 1) goto fatal;
	if (fread(&Time, sizeof Time, 1, fp) != 1) goto fatal;
	if (fread(&fuel, sizeof fuel, 1, fp) != 1) goto fatal;
	if (fread(&torps, sizeof torps, 1, fp) != 1) goto fatal;
	if (fread(&carrying, sizeof carrying, 1, fp) != 1) goto fatal;
	if (fread(&encumber, sizeof encumber, 1, fp) != 1) goto fatal;
	if (fread(&rythmn, sizeof rythmn, 1, fp) != 1) goto fatal;
	if (fread(&followfight, sizeof followfight, 1, fp) != 1) goto fatal;
	if (fread(&ate, sizeof ate, 1, fp) != 1) goto fatal;
	if (fread(&snooze, sizeof snooze, 1, fp) != 1) goto fatal;
	if (fread(&meetgirl, sizeof meetgirl, 1, fp) != 1) goto fatal;
	if (fread(&followgod, sizeof followgod, 1, fp) != 1) goto fatal;
	if (fread(&godready, sizeof godready, 1, fp) != 1) goto fatal;
	if (fread(&win, sizeof win, 1, fp) != 1) goto fatal;
	if (fread(&wintime, sizeof wintime, 1, fp) != 1) goto fatal;
	if (fread(&matchlight, sizeof matchlight, 1, fp) != 1) goto fatal;
	if (fread(&matchcount, sizeof matchcount, 1, fp) != 1) goto fatal;
	if (fread(&loved, sizeof loved, 1, fp) != 1) goto fatal;
	if (fread(&pleasure, sizeof pleasure, 1, fp) != 1) goto fatal;
	if (fread(&power, sizeof power, 1, fp) != 1) goto fatal;
	if (fread(&ego, sizeof ego, 1, fp) != 1) goto fatal;

	fclose(fp);
}

void
save()
{
	char *home;
	char home1[100];
	register int n;
	int tmp;
	FILE *fp;

	home = getenv("HOME");
	strcpy(home1, home);
	strcat(home1, "/Bstar");
	if ((fp = fopen(home1, "w")) == 0) {
		perror(home1);
		return;
	}
	printf("Saved in %s.\n", home1);
	fwrite(&WEIGHT, sizeof WEIGHT, 1, fp);
	fwrite(&CUMBER, sizeof CUMBER, 1, fp);
	fwrite(&clk, sizeof clk, 1, fp);
	tmp = location == dayfile;
	fwrite(&tmp, sizeof tmp, 1, fp);
	for (n = 1; n <= NUMOFROOMS; n++) {
		fwrite(location[n].link, sizeof location[n].link, 1, fp);
		fwrite(location[n].objects, sizeof location[n].objects, 1, fp);
	}
	fwrite(inven, sizeof inven, 1, fp);
	fwrite(wear, sizeof wear, 1, fp);
	fwrite(injuries, sizeof injuries, 1, fp);
	fwrite(notes, sizeof notes, 1, fp);
	fwrite(&direction, sizeof direction, 1, fp);
	fwrite(&position, sizeof position, 1, fp);
	fwrite(&Time, sizeof Time, 1, fp);
	fwrite(&fuel, sizeof fuel, 1, fp);
	fwrite(&torps, sizeof torps, 1, fp);
	fwrite(&carrying, sizeof carrying, 1, fp);
	fwrite(&encumber, sizeof encumber, 1, fp);
	fwrite(&rythmn, sizeof rythmn, 1, fp);
	fwrite(&followfight, sizeof followfight, 1, fp);
	fwrite(&ate, sizeof ate, 1, fp);
	fwrite(&snooze, sizeof snooze, 1, fp);
	fwrite(&meetgirl, sizeof meetgirl, 1, fp);
	fwrite(&followgod, sizeof followgod, 1, fp);
	fwrite(&godready, sizeof godready, 1, fp);
	fwrite(&win, sizeof win, 1, fp);
	fwrite(&wintime, sizeof wintime, 1, fp);
	fwrite(&matchlight, sizeof matchlight, 1, fp);
	fwrite(&matchcount, sizeof matchcount, 1, fp);
	fwrite(&loved, sizeof loved, 1, fp);
	fwrite(&pleasure, sizeof pleasure, 1, fp);
	fwrite(&power, sizeof power, 1, fp);
	fwrite(&ego, sizeof ego, 1, fp);

	fclose(fp);
}
