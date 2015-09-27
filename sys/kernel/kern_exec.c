/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/inode.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/namei.h>
#include <sys/fs.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/signalvar.h>
#include <sys/exec.h>
#include <machine/debug.h>

/*
 * exec system call, with and without environments.
 */
struct execa {
    char    *fname;
    char    **argp;
    char    **envp;
};

/*
 * This is the internal form of the exec() function.
 *
 * It is called with the inode of the executable
 * and the exec_params structure, which is used to
 * keep information during the exec.
 *
 * It returns the error code, and with the ip still locked.
 */
int exec_check(struct exec_params *epp)
{
    int error, i, r;

    DEBUG("Entering exec_check\n");
    if (access (epp->ip, IEXEC))
        return ENOEXEC;
    if ((u.u_procp->p_flag & P_TRACED) && access (epp->ip, IREAD))
        return ENOEXEC;
    if ((epp->ip->i_mode & IFMT) != IFREG ||
        (epp->ip->i_mode & (IEXEC | (IEXEC>>3) | (IEXEC>>6))) == 0)
        return EACCES;

    /*
     * Read in first few bytes of file for segment sizes, magic number:
     *  407 = plain executable
     * Also an ASCII line beginning with #! is
     * the file name of a ``interpreter'' and arguments may be prepended
     * to the argument list if given here.
     *
     * INTERPRETER NAMES ARE LIMITED IN LENGTH.
     *
     * ONLY ONE ARGUMENT MAY BE PASSED TO THE INTERPRETER FROM
     * THE ASCII LINE.
     */

    /*
     * Read the first 'SHSIZE' bytes from the file to execute
     */
    DEBUG("Read header %d bytes from %d\n", sizeof(epp->hdr), 0);
    epp->hdr.sh[0] = '\0';      /* for zero length files */
    error = rdwri (UIO_READ, epp->ip,
               (caddr_t) &epp->hdr, sizeof(epp->hdr),
               (off_t)0, IO_UNIT, &r);
    if (error)
        return error;
    epp->hdr_len = sizeof(epp->hdr) - r;

    /*
     * Given the first part of the image
     * loop through the exec file handlers to find
     * someone who can handle this file format.
     */
    error = ENOEXEC;
    DEBUG("Trying %d exec formats\n", nexecs);
    for (i = 0; i < nexecs && error != 0; i++) {
        DEBUG("Trying exec format %d : %s\n", i, execsw[i].es_name);
        if (execsw[i].es_check == NULL)
            continue;
        error = (*execsw[i].es_check)(epp);
        if (error == 0)
            break;
    }
    return error;
}

void
execv()
{
    struct execa *arg = (struct execa *)u.u_arg;

    arg->envp = NULL;
    execve();
}

void
execve()
{
    struct execa *uap = (struct execa *)u.u_arg;
    int error;
    struct inode *ip;
    struct nameidata nd;
    register struct nameidata *ndp = &nd;
    struct exec_params eparam;

    DEBUG("execve ('%s', ['%s', '%s', ...])\n", uap->fname, uap->argp[0], uap->argp[1]);
    NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
    ip = namei (ndp);
    if (ip == NULL) {
        DEBUG("execve: file '%s' not found\n", uap->fname);
        return;
    }
    /*
     * The exec_param structure is used to
     * keep information about the executable during exec's processing
     */
    bzero(&eparam, sizeof eparam);
    eparam.userfname = uap->fname;
    eparam.userargp = uap->argp;
    eparam.userenvp = uap->envp;
    eparam.uid = u.u_uid;
    eparam.gid = u.u_groups[0];

    if (ip->i_fs->fs_flags & MNT_NOEXEC) {
        u.u_error = EACCES;
        DEBUG("EACCES\n");
        goto done;
    }
    if ((ip->i_fs->fs_flags & MNT_NOSUID) == 0) {
        if (ip->i_mode & ISUID)
            eparam.uid = ip->i_uid;
        if (ip->i_mode & ISGID)
            eparam.gid = ip->i_gid;
    }

    eparam.ip = ip;
    DEBUG("calling exec_check()\n");
    if ((error = exec_check(&eparam)))
        u.u_error = error;
    DEBUG("back from exec_check()\n");
done:
    exec_alloc_freeall(&eparam);
    if (ip)
        iput(ip);
}
