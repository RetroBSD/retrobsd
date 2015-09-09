#include <sys/param.h>
#include <sys/inode.h>
#include <sys/dir.h>
#include <sys/namei.h>
#include <sys/exec.h>
#include <sys/user.h>
#include <sys/systm.h>

int
exec_script_check(struct exec_params *epp)
{
    char *cp;
    struct nameidata nd;
    struct nameidata *ndp;
    int error;
    struct inode *ip = 0;

    /*
     * We come here with the first line of the executable
     * script file.
     * Check is to see if it starts with the magic marker:  #!
     */
    if (epp->hdr.sh[0] != '#' || epp->hdr.sh[1] != '!' || epp->sh.interpreted)
        return ENOEXEC;
    epp->sh.interpreted = 1;

    /*
     * If setuid/gid scripts were to be disallowed this is where it would
     * have to be done.
     * u.u_uid = uid;
     * u.u_gid = u_groups[0];
     */

    /*
     * The first line of the text file
     * should be one line on the format:
     * #! <interpreter-name> <interpreter-argument>\n
     */
    cp = &epp->hdr.sh[2];
    while (cp < &epp->hdr.sh[MIN(epp->hdr_len, SHSIZE)]) {
        if (*cp == '\t')
            *cp = ' ';
        else if (*cp == '\n') {
            *cp = '\0';
            break;
        }
        cp++;
    }
    if (cp == &epp->hdr.sh[MIN(epp->hdr_len, SHSIZE)])
        return ENOEXEC;

    /*
     * Pick up script interpreter file name
     */
    cp = &epp->hdr.sh[2];
    while (*cp == ' ')
        cp++;
    if (!*cp)
        return ENOEXEC;
    bzero(&nd, sizeof nd);
    ndp = &nd;
    ndp->ni_dirp = cp;
    while (*cp && *cp != ' ')
        cp++;
    if (*cp != '\0') {
        *cp++ = 0;
        while (*cp && *cp == ' ')
            cp++;
        if (*cp) {
            if ((error = copystr(cp, epp->sh.interparg, sizeof epp->sh.interparg, NULL)))
                goto done;
        }
    }

    /*
     * the interpreter is the new file to exec
     */
    ndp->ni_nameiop = LOOKUP | FOLLOW;
    ip = namei (ndp);
    if (ip == NULL)
        return u.u_error;
    if ((error = copystr(ndp->ni_dent.d_name, epp->sh.interpname, sizeof epp->sh.interpname, NULL)))
        goto done;

    /*
     * Everything set up, do the recursive exec()
     */
    if (epp->ip)
        iput(epp->ip);
    epp->ip = ip;
    error = exec_check(epp);
done:
    if (ip)
        iput(ip);
    return error;
}
