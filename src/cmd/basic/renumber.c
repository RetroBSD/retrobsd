/*
 * MICRO-BASIC 2.1 line renumbering program.
 *
 * This program reads a MICRO-BASIC source file, and write a new one with
 * all lines renumbered. All IF/THEN, GOTO, GOSUB and ORDER statements are
 * adjusted to reflect the new line numbers.
 *
 * Copyright 1993-2000 Dave Dunfield
 * All rights reserved.
 *
 * Permission granted for personal (non-commercial) use only.
 *
 * Compile command: cc renumber -fop
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#   include </usr/include/ctype.h>
#else
#   include <stdio.h>
#   include <ctype.h>
#endif
#include <stdlib.h>

#define	MAX_LINES	10000	/* Maximum # lines in input program */

/*
 * Case insensitive test of 'ptr' beginning with 'string'
 */
int begins_with(char *ptr, char *string)
{
	while(*string)
		if(toupper(*ptr++) != *string++)
			return 0;
	return -1;
}

int main(int argc, char *argv[])
{
	unsigned i, l, m;
	char buffer[200], *ptr, *ptr1;
	static unsigned lines[MAX_LINES], ltop = 0, increment = 10, start = 10;
	static FILE *fp, *fp1;

	printf("MICRO-BASIC 2.1 line renumbering program\n");

	if (argc < 3) {
		fprintf(stderr, "Use: renumber old new [start [increment]]\n\n");
		fprintf(stderr, "Copyright 1993-2000 Dave Dunfield\n");
		fprintf(stderr, "All rights reserved.\n");
		return 1;
	}

	fp = fopen(argv[1], "rvq");
	fp1 = fopen(argv[2], "wvq");
	if(argc > 3)
		start = atoi(argv[3]);
	if(argc > 4)
		increment = atoi(argv[4]);

/* Pass #1: Scan and record all line numbers */
	m = 0;
	while(fgets(buffer, sizeof(buffer)-1, fp)) {
		if(!(l = atoi(buffer))) {
			printf("Invalid line number following line %u\n", m);
			return 0;
		}
		if(l <= m) {
			printf("Improper line sequence following line %u\n", m);
			return 0;
		}
		lines[ltop++] = m = l;
	}

/* Pass #2: Copy lines and replace line numbers */
	rewind(fp);
	m = 0;
	while(fgets(ptr1 = ptr = buffer, sizeof(buffer), fp)) {
		/* Replace line number with one from the new sequence */
fixnum:		while(isspace(*ptr))
			++ptr;
		while(ptr1 < ptr)
			putc(*ptr1++, fp1);
		if(isdigit(*ptr)) {
			++m;
			l = atoi(ptr);
			while(isdigit(*ptr))
				++ptr;
			for(i=0; i < ltop; ++i)
				if(lines[i] == l) {
					fprintf(fp1, "%u", i * increment + start);
					break;
				}
		}
		while(*(ptr1 = ptr)) {
			if(begins_with(ptr, "THEN")) {
				ptr += 4;
				goto fixnum;
			}
			if(begins_with(ptr, "ORDER")) {
				ptr += 5;
				goto fixnum;
			}
			if(begins_with(ptr, "GOTO")) {
				ptr += 4;
				goto fixnum;
			}
			if(begins_with(ptr, "GOSUB")) {
				ptr += 5;
				goto fixnum;
			}
			if (*ptr != '\r' && *ptr != '\n')
				putc(*ptr, fp1);
			++ptr;
		}
		putc('\n', fp1);
	}

	fclose(fp1);
	fclose(fp);
	printf("%u lines read, %u fixups\n", ltop, m);
	return 0;
}
