/*
 * Reboot ...
 *
 * Copyright (c) 1980,1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <pwd.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <sys/file.h>
#include <sys/reboot.h>
#include <sys/signal.h>

#define	OPTS	"lqnhdarsfRD"

main(argc, argv)
	int argc;
	char **argv;
{
	int howto;		/* reboot options argument */
	int needlog = 1;	/* tell syslog what's happening */
	int quickly = 0;	/* go down quickly & ungracefully */
	char *myname;		/* name we were invoked as */
	char args[20], *ap;	/* collected arguments for syslog */
	int i;
	char *rindex();

	if (myname = rindex(argv[0], '/'))
		myname++;
	else
		myname = argv[0];
	if (strcmp(myname, "halt") == 0)
		howto = RB_HALT;
	else if (strcmp(myname, "fasthalt") == 0)
		howto = RB_HALT|RB_NOFSCK;
	else if (strcmp(myname, "fastboot") == 0)
		howto = RB_NOFSCK;
	else if (strcmp(myname, "poweroff") == 0)
		howto = RB_HALT|RB_POWEROFF;
    else if (strcmp(myname, "bootloader") == 0)
        howto = RB_HALT|RB_BOOTLOADER;
	else
		howto = 0;

	ap = args;
	*ap++ = '-';
	*ap = '\0';
	while ((i = getopt(argc, argv, OPTS)) != EOF) {
		switch((char)i) {
			case 'l':  needlog = 0;		break;
			case 'q':  quickly++;		break;
			case 'n':  howto |= RB_NOSYNC;	break;
			case 'h':  howto |= RB_HALT;	break;
			case 'd':  howto |= RB_DUMP;	break;
			case 'a':  howto |= RB_ASKNAME;	break;
			case 'r':  howto |= RB_RDONLY;	break;
			case 's':  howto |= RB_SINGLE;	break;
			case 'f':  howto |= RB_NOFSCK;	break;
			case 'R':  howto |= RB_DFLTROOT; break;
			case 'D':  howto |= RB_AUTODEBUG; break;
			case 'p':  howto |= RB_HALT|RB_POWEROFF; break;
			case '?':
				fprintf(stderr,
					"usage: %s [-%s]\n", myname, OPTS);
				exit(EX_USAGE);
				/*NOTREACHED*/
		}
		if (index(args+1, (char)i) == 0) {
			*ap++ = (char)i;
			*ap = '\0';
		}
	}

	if ((howto & (RB_NOSYNC|RB_NOFSCK)) == (RB_NOSYNC|RB_NOFSCK)
	    && !(howto & RB_HALT)) {
		fprintf(stderr,
			"%s: no sync and no fsck are a dangerous combination; no fsck ignored.\n",
			myname);
		howto &= ~RB_NOFSCK;
	}
	if (needlog) {
		char *user;
		struct passwd *pw;

		user = getlogin();
		if (user == (char *)0 && (pw = getpwuid(getuid())))
			user = pw->pw_name;
		if (user == (char *)0)
			user = "root";
		openlog(myname, 0, LOG_AUTH);
		syslog(LOG_CRIT, "%s; %s by %s",
 			args, (howto&RB_HALT)?"halted":"rebooted", user);
	}
        /*
         * Do a sync early on so disks start transfers while we're killing
         * processes.
         */
	if (! (howto & RB_NOSYNC))
		sync();

        printf("killing processes...");
        fflush(stdout);
	(void) signal(SIGHUP, SIG_IGN);	/* for remote connections */
	if (kill(1, SIGTSTP) == -1) {
		fprintf(stderr, "%s: can\'t idle init\n", myname);
		exit(EX_NOPERM);
	}
	//sleep(1);
	(void) kill(-1, SIGTERM);	/* one chance to catch it */
	//sleep(1);
        printf(" done\n");

        /*
         * After the processes receive the TERM signal start the rest of the
         * buffers out to disk.  Wait five seconds between SIGTERM and SIGKILL so
         * the processes have a chance to clean up and exit nicely.
         */
	if (!(howto & RB_NOSYNC)) {
		sync();
                //sleep(1);
        }

	if (! quickly) {
		for (i = 1; ; i++) {
			if (kill(-1, SIGKILL) == -1) {
				if (errno == ESRCH)
					break;
				perror(myname);
				kill(1, SIGHUP);
				exit(EX_OSERR);
			}
			if (i > 5) {
				fprintf(stderr,
				    "CAUTION: some process(es) wouldn't die\n");
				break;
			}
			sleep(i);
		}
                if (! (howto & RB_NOSYNC)) {
                        markdown();
                        sync();
                        //sleep(1);
                }
        }
	reboot(howto);
	perror(myname);
	kill(1, SIGHUP);
	exit(EX_OSERR);
}


/*
 * Make shutdown entry in /usr/adm/utmp.
 */
#include <utmp.h>
#include <paths.h>

#define	SCPYN(a, b)	strncpy(a, b, sizeof(a))

markdown()
{
	struct utmp wtmp;
	register int f = open(_PATH_WTMP, O_WRONLY|O_APPEND);

	if (f >= 0) {
		bzero((char *)&wtmp, sizeof(wtmp));
		SCPYN(wtmp.ut_line, "~");
		SCPYN(wtmp.ut_name, "shutdown");
		SCPYN(wtmp.ut_host, "");
		(void) time(&wtmp.ut_time);
		write(f, (char *)&wtmp, sizeof(wtmp));
		close(f);
	}
}
