/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

#define Exit(a) flushb();return(a)

extern int exitval;

echo(argc, argv)
char **argv;
{
	register char   *cp;
	register int    i, wd;
	int     j;
	int nonl = 0;   /* echo -n */

	if(--argc == 0) {
		prc_buff('\n');
		Exit(0);
	}

	if ( cf(argv[1], "-n" ) == 0 ){
		nonl++;
		argv++;
		argc--;
	}

	for(i = 1; i <= argc; i++)
	{
		sigchk();
		for(cp = argv[i]; *cp; cp++)
		{
			if(*cp == '\\')
			switch(*++cp)
			{
				case 'b':
					prc_buff('\b');
					continue;

				case 'c':
					Exit(0);

				case 'f':
					prc_buff('\f');
					continue;

				case 'n':
					prc_buff('\n');
					continue;

				case 'r':
					prc_buff('\r');
					continue;

				case 't':
					prc_buff('\t');
					continue;

				case 'v':
					prc_buff('\v');
					continue;

				case '\\':
					prc_buff('\\');
					continue;
				case '\0':
					j = wd = 0;
					while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
						wd <<= 3;
						wd |= (*cp - '0');
					}
					prc_buff(wd);
					--cp;
					continue;

				default:
					cp--;
			}
			prc_buff(*cp);
		}
		if( nonl ) prc_buff( ' ' );
		else       prc_buff(i == argc? '\n': ' ');
	}
	Exit(0);
}
