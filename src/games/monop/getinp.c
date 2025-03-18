#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define	reg	register

#define	LINE	70

static char	buf[257];

static int
comp(
char	*s1) {

	reg char	*sp, *tsp, c;

	if (buf[0] != '\0')
		for (sp = buf, tsp = s1; *sp; ) {
			c = isupper(*tsp) ? tolower(*tsp) : *tsp;
			tsp++;
			if (c != *sp++)
				return 0;
		}
	else if (*s1 != '\0')
		return 0;
	return 1;
}

int
getinp(
char	*prompt, 
char	*list[]) {

	reg int	i, n_match, match = 0;
	char	*sp;

	for (;;) {
inter:
		printf("%s", prompt);
		for (sp = buf; (*sp=getchar()) != '\n' && !feof(stdin); )
			if (*sp == -1)	/* check for interupted system call */
				goto inter;
			else if (sp != buf || *sp != ' ')
				sp++;
		if (feof(stdin)) {
			clearerr(stdin);
			continue;
		}
		if (buf[0] == '?' && buf[1] == '\n') {
			printf("Valid inputs are: ");
			for (i = 0, match = 18; list[i]; i++) {
				if ((match+=(n_match=strlen(list[i]))) > LINE) {
					printf("\n\t");
					match = n_match + 8;
				}
				if (*list[i] == '\0') {
					match += 8;
					printf("<RETURN>");
				}
				else
					printf("%s", list[i]);
				if (list[i+1])
					printf(", ");
				else
					putchar('\n');
				match += 2;
			}
			continue;
		}
		*sp = '\0';
		for (sp = buf; *sp; sp++)
			if (isupper(*sp))
				*sp = tolower(*sp);
		for (i = n_match = 0; list[i]; i++)
			if (comp(list[i])) {
				n_match++;
				match = i;
			}
		if (n_match == 1)
			return match;
		else if (buf[0] != '\0')
			printf("Illegal response: \"%s\".  Use '?' to get list of valid answers\n", buf);
	}
}
