#ifndef lint
static char sccsid[] = "@(#)uuname.c	5.3 (Berkeley) 10/9/85";
#endif

#include "uucp.h"
#include <signal.h>

/*
 *      return list of all remote systems 
 *	recognized by uucp, or  (with -l) the local  uucp name.
 *
 *      return codes: 0 | 1  (can't read)
 */

struct timeb Now;
 
main(argc,argv)
char *argv[];
int argc;
{
	int ret;
	int i;
	int intrEXIT();
	FILE *np;
/* Increase buffers for s and prev.  cornell!pavel */
	char prev[1000];
	char s[1000];

	ret = chdir(Spool);
	ASSERT(ret >= 0, "CHDIR FAILED", Spool, ret);
	strcpy(Progname, "uuname");
	signal(SIGILL, (sig_t)intrEXIT);
	signal(SIGTRAP, (sig_t)intrEXIT);
	signal(SIGIOT, (sig_t)intrEXIT);
	signal(SIGEMT, (sig_t)intrEXIT);
	signal(SIGFPE, (sig_t)intrEXIT);
	signal(SIGBUS, (sig_t)intrEXIT);
	signal(SIGSEGV, (sig_t)intrEXIT);
	signal(SIGSYS, (sig_t)intrEXIT);
	signal(SIGINT, (sig_t)intrEXIT);
	signal(SIGHUP, (sig_t)intrEXIT);
	signal(SIGQUIT, (sig_t)intrEXIT);
	signal(SIGTERM, (sig_t)intrEXIT);

	if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'l') {
		uucpname(s);
		printf("%s\n",s);
		exit(0);
	}
        if(argc != 1) {printf("Usage: uuname [-l]\n"); exit(1);}
	if((np = fopen(SYSFILE,"r")) == NULL) {
		printf("%s (name file) protected\n",SYSFILE);
		exit(1);
	}
	while ( cfgets(s,sizeof(s),np) != NULL ) {
		for(i=0; s[i]!=' ' && s[i]!='\t'; i++)
			;
		s[i]='\0';
		if (strcmp(s, prev) == SAME)
			continue;
		if(s[0]=='x' && s[1]=='x' && s[2]=='x')
			continue;
		printf("%s\n",s);
		strcpy(prev, s);
	}
 
	exit(0);
}
intrEXIT(inter)
{
	exit(inter);
}

cleanup(code)
int code;
{
	exit(code);
}
