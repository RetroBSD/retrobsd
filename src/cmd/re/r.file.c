/*
 * Opening and saving files.
 *
 * RED editor for OS DEMOS
 * Alex P. Roudnev, Moscow, KIAE, 1984
 */
#include "r.defs.h"
#include <errno.h>

/*
 * Save a file from the channel n to a given filename.
 * When filename is NULL, use the name from file[n].name.
 * When no permission to write, ask to write to current directory.
 */
int savefile(filename, n)
    char *filename;
    int n;
{
    char *fname, *last_slash;
    int newfd, dir_writable, need_backup = 1;

    /* Get the directory name. */
    if (filename) {
        fname = filename;
    } else {
        fname = file[n].name;
        if (file[n].backup_done)
            need_backup = 0;
    }
    last_slash = strrchr (fname, '/');
    if (last_slash != 0) {
        *last_slash = '\0';
        if (access(fname, F_OK) < 0) {
            *last_slash = '/';
            error ("Directory does not exist.");
            return(0);
        }
        dir_writable = (access(fname, R_OK | W_OK) == 0);
        *last_slash = '/';
    } else {
        dir_writable = (access(".", R_OK | W_OK) == 0);
    }

    if (! dir_writable) {
        if (filename) {
            error ("Can't write to specified directory");
            return(0);
        }
        if (last_slash == 0) {
            error ("Can't write to current directory");
            return(0);
        }
        telluser("Press ^N to save to current directory", 0);
        keysym = -1;
        need_backup = 1;
        getkeysym();
        if (keysym != CCSETFILE)
            return(-1);
        if (access(".", R_OK | W_OK) < 0) {
            error ("Can't write to current directory");
            return(0);
        }
        fname = last_slash + 1; /* points to file base name */
    }

    /* Open fname for write, make backup. */
    if (need_backup) {
        char *backup = append (fname, "~");
        unlink(backup);
        if (link(fname, backup) < 0 && errno != ENOENT) {
            error ("Link failed!");
            free(backup);
            return(0);
        }
        if (! filename)
            file[n].backup_done = 1;
        free(backup);
    }
    unlink(fname);
    newfd = creat(fname, getpriv(n));
    if (newfd < 0) {
        error ("Creat failed!");
        return(0);
    }
    /* chown(fname, userid); */

    /* Copy the data. */
    telluser("save: ", 0);
    telluser(fname, 6);
    return segmwrite(file[n].chain, 0xfffff, newfd) == -1 ? 0 : 1;
}

/*
 * Write a segment chain into the file.
 * When maxlines>0 - put only maxlines lines of text.
 * When maxlines<0 - put only -maxlines paragraphs.
 * Return a count of written lines, or -1 on error.
 */
int segmwrite(ff, maxlines, newf)
    segment_t *ff;
    int maxlines, newf;
{
    register segment_t *f;
    register char *c;
    register int i;
    int j, k, bflag, tlines;

    if (cline_max < LBUFFER)
        cline_expand(LBUFFER);
    f = ff;
    bflag = 1;
    tlines = 0;
    while (f->fdesc && maxlines) {
        if (f->fdesc > 0) {
            i = 0;
            c = &f->data;
            for (j=f->nlines; j; j--) {
                if (maxlines < 0) {
                    /* Check the count of empty lines. */
                    if (bflag && *c != 1)
                        bflag = 0;
                    else if (bflag == 0 && *c == 1) {
                        bflag = 1;
                        if (++maxlines == 0)
                            break;
                    }
                }
                if (*c & 0200)
                    i += 128 * (*c++ & 0177);
                i += *c++;
                ++tlines;
                /* Check the line count. */
                if (maxlines > 0 && --maxlines == 0)
                    break;
            }
            lseek(f->fdesc, f->seek, 0);
            while (i) {
                j = (i < LBUFFER) ? i : LBUFFER;
                if (read(f->fdesc, cline, j) < 0)
                    /* ignore errors */;
                if (write(newf, cline, j) < 0) {
                    error("DANGER -- WRITE ERROR");
                    close(newf);
                    return(-1);
                }
                i -= j;
            }
        } else {
            j = f->nlines;
            if (maxlines < 0) {
                if (bflag == 0 && ++maxlines == 0)
                    j = 0;
                bflag = 1;
            } else {
                if (j > maxlines)
                    j = maxlines;
                maxlines -= j;
            }
            k = j;
            while (k)
                cline[--k] = '\n';
            if (j && write(newf, cline, j) < 0) {
                error("DANGER -- WRITE ERROR");
                close(newf);
                return(-1);
            }
            tlines += j;
        }
        f = f->next;
    }
    close(newf);
    return tlines;
}

/*
 * Open the file for editing, starting from the given line and column.
 * File is opened in the current window.  When file does not exist,
 * and mkflg==1, ask a user for a permission to create a file.
 * Return -1, when the file was not opened and not created.
 * When putflg==1, file is displayed on the screen.
 */
int editfile(filename, line, col, mkflg, puflg)
    char *filename;
    int line, col, mkflg, puflg;
{
    int i, j;
    register int fn;
    register char *c,*d;

    fn = -1;
    for (i=0; i<MAXFILES; ++i) {
        if (file[i].name != 0) {
            c = filename;
            d = file[i].name;
            while (*c++ == *d) {
                if (*(d++) == 0) {
                    fn = i;
                    break;
                }
            }
        }
    }
    if (fn < 0) {
        fn = open(filename, 0);  /* File exists? */
        if (fn >= 0) {
            if (fn >= MAXFILES) {
                error("Too many files -- editor limit!");
                close(fn);
                return(0);
            }
            j = checkpriv(fn);
            if (j == 0) {
                error("File read protected.");
                close(fn);
                return(0);
            }
            file[fn].writable = (j == 2) ? 1 : 0;
            telluser("Use: ",0);
            telluser(filename, 5);
        } else {
            if (! mkflg)
                return (-1);
            telluser("Press ^N to create new file: ",0);
            telluser(filename, 28);
            keysym = -1;
            getkeysym();
            if (keysym != CCSETFILE && keysym != 'Y' && keysym != 'y')
                return(-1);
            /* Find the directory. */
            for (c=d=filename; *c; c++)
                if (*c == '/')
                    d = c;
            if (d > filename) {
                *d = '\0';
                i = open(filename, 0);
            } else
                i = open(".", 0);

            if (i < 0) {
                error("Specified directory does not exist.");
                return(0);
            }
            if (checkpriv(i) != 2) {
                error("Can't write in:");
                telluser (filename, 21);
                return(0);
            }
            close(i);
            if (d > filename)
                *d = '/';

            /* Create file */
            fn = creat(filename, FILEMODE);
            close(fn);
            fn = open(filename, 0);
            if (fn < 0) {
                error("Create failed!");
                return(0);
            }
            if (fn >= MAXFILES) {
                close(fn);
                error("Too many files -- Editor limit!");
                return(0);
            }
            file[fn].writable = 1;
            if (chown(filename, userid, groupid) < 0)
                /* ignore errors */;
        }
        param_len = 0;   /* so its kept around */
        file[fn].name = filename;
    }
    /* Flush the output buffer, as here is a long operation. */
    dumpcbuf();
    wksp_switch();
    if (! file[fn].chain)
        file[fn].chain = file2segm(fn);
    curwksp->cursegm = file[fn].chain;
    curfile = curwksp->wfile = fn;
    curwksp->line = curwksp->segmline = 0;
    curwksp->topline = line;
    curwksp->offset = col;
    if (puflg) {
        drawlines(0, curwin->text_maxrow);
        poscursor(0, 0);
    }
    return(1);
}

/*
 * End a session and write all.
 * Return 0 on write error.
 */
int endit()
{
    register int i, ko = 1;

    for (i=0; i<MAXFILES; i++)
        if (file[i].chain && file[i].writable == EDITED)
            if (savefile(NULL, i) == 0)
                ko = 0;
    return(ko);
}
