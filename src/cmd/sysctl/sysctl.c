/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>

#include <netinet/in.h>
#if 0
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

struct ctlname topname[] = CTL_NAMES;
struct ctlname kernname[] = CTL_KERN_NAMES;
struct ctlname vmname[] = CTL_VM_NAMES;
#ifdef CTL_NET_NAMES
struct ctlname netname[] = CTL_NET_NAMES;
#endif
struct ctlname hwname[] = CTL_HW_NAMES;
struct ctlname username[] = CTL_USER_NAMES;
struct ctlname debugname[CTL_DEBUG_MAXID];
struct ctlname machdepname[] = CTL_MACHDEP_NAMES;
char names[BUFSIZ];

struct list {
	struct	ctlname *list;
	int	size;
};
struct list toplist = { topname, CTL_MAXID };
struct list secondlevel[] = {
	{ 0, 0 },			/* CTL_UNSPEC */
	{ kernname, KERN_MAXID },	/* CTL_KERN */
	{ vmname, VM_MAXID },		/* CTL_VM */
	{ 0, 0 },			/* CTL_FS */
#ifdef CTL_NET_NAMES
	{ netname, NET_MAXID },		/* CTL_NET */
#else
	{ 0, 0 },
#endif
	{ 0, CTL_DEBUG_MAXID },		/* CTL_DEBUG */
	{ hwname, HW_MAXID },		/* CTL_HW */
	{ machdepname, CPU_MAXID },	/* CTL_MACHDEP */
	{ username, USER_MAXID },	/* CTL_USER_NAMES */
};

int	Aflag, aflag, nflag, wflag;

extern char *optarg;
extern int optind, errno;

/*
 * Variables requiring special processing.
 */
#define	CLOCK		0x0001
#define	BOOTTIME	0x0002
#define	CONSDEV		0x0004

static void usage(void);
static void debuginit(void);
static void listall(char *prefix, struct list *lp);
static void parse(char *string, int flags);
static int findname(char *string, char *level, char **bufp, struct list *namelist);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, lvl1;

	while ((ch = getopt(argc, argv, "Aanw")) != EOF) {
		switch (ch) {

		case 'A':
			Aflag = 1;
			break;

		case 'a':
			aflag = 1;
			break;

		case 'n':
			nflag = 1;
			break;

		case 'w':
			wflag = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (Aflag || aflag) {
		debuginit();
		for (lvl1 = 1; lvl1 < CTL_MAXID; lvl1++)
			listall(topname[lvl1].ctl_name, &secondlevel[lvl1]);
		exit(0);
	}
	if (argc == 0)
		usage();
	while (argc-- > 0)
		parse(*argv, 1);
	exit(0);
}

/*
 * List all variables known to the system.
 */
void
listall(prefix, lp)
	char *prefix;
	struct list *lp;
{
	int lvl2;
	char *cp, name[BUFSIZ];

	if (lp->list == 0)
		return;
	strcpy(name, prefix);
	cp = &name[strlen(name)];
	*cp++ = '.';
	for (lvl2 = 0; lvl2 < lp->size; lvl2++) {
		if (lp->list[lvl2].ctl_name == 0)
			continue;
		strcpy(cp, lp->list[lvl2].ctl_name);
		parse(name, Aflag);
	}
}

/*
 * Parse a name into a MIB entry.
 * Lookup and print out the MIB entry if it exists.
 * Set a new value if requested.
 */
void
parse(string, flags)
	char *string;
	int flags;
{
	int indx, type, state, len;
	size_t size;
	int special = 0;
	void *newval = (void *)0;
	int intval, newsize = 0;
	long longval;
	struct list *lp;
	int mib[CTL_MAXNAME];
	char *cp, *bufp, buf[BUFSIZ], strval[BUFSIZ];

	bufp = buf;
	strcpy(buf, string);
	if ((cp = strchr(string, '=')) != NULL) {
		if (!wflag) {
			fprintf(stderr, "Must specify -w to set variables\n");
			exit(2);
		}
		*strchr(buf, '=') = '\0';
		*cp++ = '\0';
		while (isspace(*cp))
			cp++;
		newval = (void *)cp;
		newsize = strlen(cp);
	}
	if ((indx = findname(string, "top", &bufp, &toplist)) == -1)
		return;
	mib[0] = indx;
	if (indx == CTL_DEBUG)
		debuginit();
	lp = &secondlevel[indx];
	if (lp->list == 0) {
		fprintf(stderr, "%s: class is not implemented\n",
		    topname[indx]);
		return;
	}
	if (bufp == NULL) {
		listall(topname[indx].ctl_name, lp);
		return;
	}
	if ((indx = findname(string, "second", &bufp, lp)) == -1)
		return;
	mib[1] = indx;
	type = lp->list[indx].ctl_type;
	len = 2;
	switch (mib[0]) {

	case CTL_KERN:
		switch (mib[1]) {
		case KERN_PROF:
			fprintf(stderr,
				"kern.prof =  not supported in 2.11BSD\n");
			return;
		case KERN_INODE:
		case KERN_FILE:
		case KERN_TEXT:
			if (flags == 0)
				return;
			fprintf(stderr,
			    "Use pstat to view %s information\n", string);
			return;
		case KERN_PROC:
			if (flags == 0)
				return;
			fprintf(stderr,
			    "Use ps to view %s information\n", string);
			return;
		case KERN_CLOCKRATE:
			special |= CLOCK;
			break;
		case KERN_BOOTTIME:
			special |= BOOTTIME;
			break;
		}
		break;

	case CTL_HW:
		break;

	case CTL_VM:
		if (mib[1] == VM_LOADAVG) {
			unsigned loads[3];

			getloadavg(loads, 3);
			if (!nflag)
				fprintf(stdout, "%s: ", string);
			fprintf(stdout, "%u.%02u %u.%02u %u.02u\n",
                                loads[0] / 100, loads[0] % 100,
                                loads[1] / 100, loads[1] % 100,
                                loads[2] / 100, loads[2] % 100);
			return;
		}
		if (flags == 0)
			return;
		fprintf(stderr,
		    "Use vmstat or pstat to view %s information\n", string);
		return;

	case CTL_NET:
#ifdef PF_INET
		if (mib[1] == PF_INET) {
			len = sysctl_inet(string, &bufp, mib, flags, &type);
			if (len >= 0)
				break;
			return;
		}
#endif
		if (flags == 0)
			return;
		fprintf(stderr, "Use netstat to view %s information\n", string);
		return;

	case CTL_DEBUG:
		mib[2] = CTL_DEBUG_VALUE;
		len = 3;
		break;

	case CTL_MACHDEP:
		if (mib[1] == CPU_CONSDEV)
			special |= CONSDEV;
		break;

	case CTL_FS:
	case CTL_USER:
		break;

	default:
		fprintf(stderr, "Illegal top level value: %d\n", mib[0]);
		return;

	}
doit:
	if (bufp) {
		fprintf(stderr, "name %s in %s is unknown\n", *bufp, string);
		return;
	}
	if (newsize > 0) {
		switch (type) {
		case CTLTYPE_INT:
			intval = atoi(newval);
			newval = (void *)&intval;
			newsize = sizeof intval;
			break;

		case CTLTYPE_LONG:
			sscanf(newval, "%ld", &longval);
			newval = (void *)&longval;
			newsize = sizeof longval;
			break;
		}
	}
	size = BUFSIZ;
	if (sysctl(mib, len, buf, &size, newsize ? newval : (void *)0, newsize) == -1) {
		if (flags == 0)
			return;
		switch (errno) {
		case EOPNOTSUPP:
			fprintf(stderr, "%s: value is not available\n", string);
			return;
		case ENOTDIR:
			fprintf(stderr, "%s: specification is incomplete\n",
			    string);
			return;
		case ENOMEM:
			fprintf(stderr, "%s: type is unknown to this program\n",
			    string);
			return;
		default:
			perror(string);
			return;
		}
	}
	if (special & CLOCK) {
		struct clockinfo *clkp = (struct clockinfo *)buf;

		if (!nflag)
			fprintf(stdout, "%s: ", string);
		fprintf(stdout,
		    "hz = %d, tick = %d, profhz = %d, stathz = %d\n",
		    clkp->hz, clkp->tick, clkp->profhz, clkp->stathz);
		return;
	}
	if (special & BOOTTIME) {
		struct timeval *btp = (struct timeval *)buf;

		if (!nflag)
			fprintf(stdout, "%s = %s", string,
			    ctime(&btp->tv_sec));
		else
			fprintf(stdout, "%d\n", btp->tv_sec);
		return;
	}
	if (special & CONSDEV) {
		dev_t dev = *(dev_t *)buf;

		if (!nflag)
			fprintf(stdout, "%s = %s\n", string,
			    devname(dev, S_IFCHR));
		else
			fprintf(stdout, "0x%x\n", dev);
		return;
	}
	switch (type) {
	case CTLTYPE_INT:
		if (newsize == 0) {
			if (!nflag)
				fprintf(stdout, "%s = ", string);
			fprintf(stdout, "%d\n", *(int *)buf);
		} else {
			if (!nflag)
				fprintf(stdout, "%s: %d -> ", string,
				    *(int *)buf);
			fprintf(stdout, "%d\n", *(int *)newval);
		}
		return;

	case CTLTYPE_STRING:
		if (newsize == 0) {
			if (!nflag)
				fprintf(stdout, "%s = ", string);
			fprintf(stdout, "%s\n", buf);
		} else {
			if (!nflag)
				fprintf(stdout, "%s: %s -> ", string, buf);
			fprintf(stdout, "%s\n", newval);
		}
		return;

	case CTLTYPE_LONG:
		if (newsize == 0) {
			if (!nflag)
				fprintf(stdout, "%s = ", string);
			fprintf(stdout, "%ld\n", *(long *)buf);
		} else {
			if (!nflag)
				fprintf(stdout, "%s: %ld -> ", string,
				    *(long *)buf);
			fprintf(stdout, "%ld\n", *(long *)newval);
		}
		return;

	case CTLTYPE_STRUCT:
		fprintf(stderr, "%s: unknown structure returned\n",
		    string);
		return;

	default:
	case CTLTYPE_NODE:
		fprintf(stderr, "%s: unknown type returned\n",
		    string);
		return;
	}
}

/*
 * Initialize the set of debugging names
 */
void
debuginit()
{
	int mib[3], loc, i;
	size_t size;

	if (secondlevel[CTL_DEBUG].list != 0)
		return;
	secondlevel[CTL_DEBUG].list = debugname;
	mib[0] = CTL_DEBUG;
	mib[2] = CTL_DEBUG_NAME;
	for (loc = 0, i = 0; i < CTL_DEBUG_MAXID; i++) {
		mib[1] = i;
		size = BUFSIZ - loc;
		if (sysctl(mib, 3, &names[loc], &size, NULL, 0) == -1)
			continue;
		debugname[i].ctl_name = &names[loc];
		debugname[i].ctl_type = CTLTYPE_INT;
		loc += size;
	}
}

#ifdef PF_INET
struct ctlname inetname[] = CTL_IPPROTO_NAMES;
struct ctlname ipname[] = IPCTL_NAMES;
struct ctlname icmpname[] = ICMPCTL_NAMES;
struct ctlname udpname[] = UDPCTL_NAMES;
struct list inetlist = { inetname, IPPROTO_MAXID };
struct list inetvars[] = {
	{ ipname, IPCTL_MAXID },	/* ip */
	{ icmpname, ICMPCTL_MAXID },	/* icmp */
	{ 0, 0 },			/* igmp */
	{ 0, 0 },			/* ggmp */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },			/* tcp */
	{ 0, 0 },
	{ 0, 0 },			/* egp */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },			/* pup */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ udpname, UDPCTL_MAXID },	/* udp */
};

/*
 * handle internet requests
 */
int
sysctl_inet(string, bufpp, mib, flags, typep)
	char *string;
	char **bufpp;
	int mib[];
	int flags;
	int *typep;
{
	struct list *lp;
	int indx;

	if (*bufpp == NULL) {
		listall(string, &inetlist);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, &inetlist)) == -1)
		return (-1);
	mib[2] = indx;
	if (indx <= IPPROTO_UDP && inetvars[indx].list != NULL)
		lp = &inetvars[indx];
	else if (!flags)
		return (-1);
	else {
		fprintf(stderr, "%s: no variables defined for this protocol\n",
		    string);
		return (-1);
	}
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "fourth", bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	*typep = lp->list[indx].ctl_type;
	return (4);
}
#endif /* PF_INET */

/*
 * Scan a list of names searching for a particular name.
 */
int
findname(string, level, bufp, namelist)
	char *string;
	char *level;
	char **bufp;
	struct list *namelist;
{
	char *name;
	int i;

	if (namelist->list == 0 || (name = strsep(bufp, ".")) == NULL) {
		fprintf(stderr, "%s: incomplete specification\n", string);
		return (-1);
	}
	for (i = 0; i < namelist->size; i++)
		if (namelist->list[i].ctl_name != NULL &&
		    strcmp(name, namelist->list[i].ctl_name) == 0)
			break;
	if (i == namelist->size) {
		fprintf(stderr, "%s level name %s in %s is invalid\n",
		    level, name, string);
		return (-1);
	}
	return (i);
}

void
usage()
{
	(void)fprintf(stderr, "usage:\t%s\n\t%s\n\t%s\n\t%s\n",
	    "sysctl [-n] variable ...", "sysctl [-n] -w variable=value ...",
	    "sysctl [-n] -a", "sysctl [-n] -A");
	exit(1);
}
