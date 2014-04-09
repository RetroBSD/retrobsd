/*
 * From the 4.4-Lite2 CD's csh sources and modified appropriately.
*/
#include "sh.h"
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include "sh.exec.h"

extern	char	*justabs[];	/* in sh.exec.c */

/* Also by:
 *  Andreas Luik <luik@isaak.isa.de>
 *  I S A  GmbH - Informationssysteme fuer computerintegrierte Automatisierung
 *  Azenberstr. 35
 *  D-7000 Stuttgart 1
 *  West-Germany
 * is the executable() routine below and changes to iscommand().
 * Thanks again!!
 */

/*
 * executable() examines the pathname obtained by concatenating dir and name
 * (dir may be NULL), and returns 1 either if it is executable by us, or
 * if dir_ok is set and the pathname refers to a directory.
 * This is a bit kludgy, but in the name of optimization...
 */
static int
executable(dir, name, dir_ok)
    char   *dir, *name;
    bool    dir_ok;
{
    struct stat stbuf;
    char    path[MAXPATHLEN + 1];
    register char *dp, *sp;
    char   *strname;

    if (dir && *dir) {
	for (dp = path, sp = dir; *sp; *dp++ = *sp++)
	    if (dp == &path[MAXPATHLEN + 1]) {
		*--dp = '\0';
		break;
	    }
	for (sp = name; *sp; *dp++ = *sp++)
	    if (dp == &path[MAXPATHLEN + 1]) {
		*--dp = '\0';
		break;
	    }
	*dp = '\0';
	strname = path;
    }
    else
	strname = name;
    return (stat(strname, &stbuf) != -1 &&
	    ((S_ISREG(stbuf.st_mode) &&
    /* save time by not calling access() in the hopeless case */
	      (stbuf.st_mode & (S_IXOTH | S_IXGRP | S_IXUSR)) &&
	      access(strname, X_OK) == 0) ||
	     (dir_ok && S_ISDIR(stbuf.st_mode))));
}

static int
iscommand(name)
    char   *name;
{
    register char **pv;
    register char *sav;
    register struct varent *v;
    bool slash = any(name, '/');
    int hashval = 0, hashval1, i;

    v = adrof("path");
    if (v == 0 || v->vec[0] == 0 || slash)
	pv = justabs;
    else
	pv = v->vec;
    sav = strspl("/", name);	/* / command name for postpending */
    if (havhash)
	hashval = hashname(name);
    i = 0;
    do {
	if (!slash && pv[0][0] == '/' && havhash) {
	    hashval1 = hash(hashval, i);
	    if (!bit(xhash, hashval1))
		goto cont;
	}
	if (pv[0][0] == 0 || eq(pv[0], ".")) {	/* don't make ./xxx */
	    if (executable(NULL, name, 0)) {
		xfree(sav);
		return i + 1;
	    }
	}
	else {
	    if (executable(*pv, sav, 0)) {
		xfree(sav);
		return i + 1;
	    }
	}
cont:
	pv++;
	i++;
    } while (*pv);
    xfree(sav);
    return 0;
}

static void
tellmewhat(lex)
    struct wordent *lex;
{
    int i;
    struct biltins *bptr;
    register struct wordent *sp = lex->next;
    bool    aliased = 0;
    register char   *s2;
    char *s0, *s1, *cmd;
    char    qc;

    if (adrof1(sp->word, &aliases)) {
	alias(lex);
	sp = lex->next;
	aliased = 1;
    }

    s0 = sp->word;		/* to get the memory freeing right... */

    /* handle quoted alias hack */
    if ((*(sp->word) & (QUOTE | TRIM)) == QUOTE)
	(sp->word)++;

    /* do quoting, if it hasn't been done */
    s1 = s2 = sp->word;
    while (*s2)
	switch (*s2) {
	case '\'':
	case '"':
	    qc = *s2++;
	    while (*s2 && *s2 != qc)
		*s1++ = *s2++ | QUOTE;
	    if (*s2)
		s2++;
	    break;
	case '\\':
	    if (*++s2)
		*s1++ = *s2++ | QUOTE;
	    break;
	default:
	    *s1++ = *s2++;
	}
    *s1 = '\0';

    for (bptr = bfunc; bptr < &bfunc[nbfunc]; bptr++) {
	if (eq(sp->word, bptr->bname)) {
	    if (aliased)
		prlex(lex);
	    (void) printf("%s: shell built-in command.\n", sp->word);
	    sp->word = s0;	/* we save and then restore this */
	    return;
	}
    }

    sp->word = cmd = globone(sp->word);

    if ((i = iscommand(strip(sp->word))) != 0) {
	register char **pv;
	register struct varent *v;
	bool    slash = any(sp->word, '/');

	v = adrof("path");
	if (v == 0 || v->vec[0] == 0 || slash)
	    pv = justabs;
	else
	    pv = v->vec;

	while (--i)
	    pv++;
	if (pv[0][0] == 0 || eq(pv[0], ".")) {
	    if (!slash) {
		sp->word = strspl("./", sp->word);
		prlex(lex);
		xfree(sp->word);
	    }
	    else
		prlex(lex);
	    sp->word = s0;	/* we save and then restore this */
	    xfree(cmd);
	    return;
	}
	s1 = strspl(*pv, "/");
	sp->word = strspl(s1, sp->word);
	xfree(s1);
	prlex(lex);
	xfree(sp->word);
    }
    else {
	if (aliased)
	    prlex(lex);
	(void) printf("%s: Command not found.\n", sp->word);
    }
    sp->word = s0;		/* we save and then restore this */
    xfree(cmd);
}

/* The dowhich() is by:
 *  Andreas Luik <luik@isaak.isa.de>
 *  I S A  GmbH - Informationssysteme fuer computerintegrierte Automatisierung
 *  Azenberstr. 35
 *  D-7000 Stuttgart 1
 *  West-Germany
 * Thanks!!
 */
/*ARGSUSED*/
void
dowhich(v, c)
    register char **v;
    struct command *c;
{
    struct wordent lex[3];
    struct varent *vp;

    lex[0].next = &lex[1];
    lex[1].next = &lex[2];
    lex[2].next = &lex[0];

    lex[0].prev = &lex[2];
    lex[1].prev = &lex[0];
    lex[2].prev = &lex[1];

    lex[0].word = "";
    lex[2].word = "\n";

    while (*++v) {
	if ((vp = adrof1(*v, &aliases)) != NULL) {
	    (void) printf("%s: \t aliased to ", *v);
	    blkpr(vp->vec);
	    (void) putchar('\n');
	}
	else {
	    lex[1].word = *v;
	    tellmewhat(lex);
	}
    }
}
