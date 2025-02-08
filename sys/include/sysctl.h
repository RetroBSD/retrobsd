/*
 * Copyright (c) 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
#ifndef _SYS_SYSCTL_H_
#define _SYS_SYSCTL_H_

/*
 * These are for the eproc, etext, einode, efile and map structures.
 */
#ifndef KERNEL
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/map.h>
#endif

/*
 * Definitions for sysctl call.  The sysctl call uses a hierarchical name
 * for objects that can be examined or modified.  The name is expressed as
 * a sequence of integers.  Like a file path name, the meaning of each
 * component depends on its place in the hierarchy.  The top-level and kern
 * identifiers are defined here, and other identifiers are defined in the
 * respective subsystem header files.
 */

#define CTL_MAXNAME     12  /* largest number of components supported */

/*
 * Each subsystem defined by sysctl defines a list of variables
 * for that subsystem. Each name is either a node with further
 * levels defined below it, or it is a leaf of some particular
 * type given below. Each sysctl level defines a set of name/type
 * pairs to be used by sysctl(1) in manipulating the subsystem.
 */
struct ctlname {
    char    *ctl_name;      /* subsystem name */
    int     ctl_type;       /* type of name */
};
#define CTLTYPE_NODE    1   /* name is a node */
#define CTLTYPE_INT     2   /* name describes a 16-bit integer */
#define CTLTYPE_STRING  3   /* name describes a string */
#define CTLTYPE_LONG    4   /* name describes a 32-bit number */
#define CTLTYPE_STRUCT  5   /* name describes a structure */

/*
 * Top-level identifiers
 */
#define CTL_UNSPEC      0   /* unused */
#define CTL_KERN        1   /* "high kernel": proc, limits */
#define CTL_VM          2   /* virtual memory */
#define CTL_FS          3   /* file system, mount type is next */
#define CTL_NET         4   /* network, see socket.h */
#define CTL_DEBUG       5   /* debugging parameters */
#define CTL_HW          6   /* generic cpu/io */
#define CTL_MACHDEP     7   /* machine dependent */
#define CTL_USER        8   /* user-level */
#define CTL_MAXID       9   /* number of valid top-level ids */

#ifndef KERNEL
#define CTL_NAMES { \
    { 0, 0 }, \
    { "kern", CTLTYPE_NODE }, \
    { "vm", CTLTYPE_NODE }, \
    { "fs", CTLTYPE_NODE }, \
    { "net", CTLTYPE_NODE }, \
    { "debug", CTLTYPE_NODE }, \
    { "hw", CTLTYPE_NODE }, \
    { "machdep", CTLTYPE_NODE }, \
    { "user", CTLTYPE_NODE }, \
}
#endif

/*
 * CTL_KERN identifiers
 */
#define KERN_OSTYPE         1   /* string: system version */
#define KERN_OSRELEASE      2   /* string: system release */
#define KERN_OSREV          3   /* int: system revision */
#define KERN_VERSION        4   /* string: compile time info */
#define KERN_MAXINODES      5   /* int: max inodes */
#define KERN_MAXPROC        6   /* int: max processes */
#define KERN_MAXFILES       7   /* int: max open files */
#define KERN_ARGMAX         8   /* int: max arguments to exec */
#define KERN_SECURELVL      9   /* int: system security level */
#define KERN_HOSTNAME       10  /* string: hostname */
#define KERN_HOSTID         11  /* int: host identifier */
#define KERN_CLOCKRATE      12  /* struct: struct clockrate */
#define KERN_INODE          13  /* struct: inode structures */
#define KERN_PROC           14  /* struct: process entries */
#define KERN_FILE           15  /* struct: file entries */
#define KERN_PROF           16  /* node: kernel profiling info */
#define KERN_POSIX1         17  /* int: POSIX.1 version */
#define KERN_NGROUPS        18  /* int: # of supplemental group ids */
#define KERN_JOB_CONTROL    19  /* int: is job control available */
#define KERN_SAVED_IDS      20  /* int: saved set-user/group-ID */
#define KERN_BOOTTIME       21  /* struct: time kernel was booted */
#define KERN_MAXTEXTS       22  /* int: # of text entries */
#define KERN_TEXT           23  /* struct: text entries */
#define KERN_ACCTTHRESH     24  /* int: accounting daemon threshold */
#define KERN_MAXID          25  /* number of valid kern ids */

#ifndef KERNEL
#define CTL_KERN_NAMES { \
    { 0, 0 }, \
    { "ostype", CTLTYPE_STRING }, \
    { "osrelease", CTLTYPE_STRING }, \
    { "osrevision", CTLTYPE_LONG }, \
    { "version", CTLTYPE_STRING }, \
    { "maxinodes", CTLTYPE_INT }, \
    { "maxproc", CTLTYPE_INT }, \
    { "maxfiles", CTLTYPE_INT }, \
    { "argmax", CTLTYPE_INT }, \
    { "securelevel", CTLTYPE_INT }, \
    { "hostname", CTLTYPE_STRING }, \
    { "hostid", CTLTYPE_LONG }, \
    { "clockrate", CTLTYPE_STRUCT }, \
    { "inode", CTLTYPE_STRUCT }, \
    { "proc", CTLTYPE_STRUCT }, \
    { "file", CTLTYPE_STRUCT }, \
    { "profiling", CTLTYPE_NODE }, \
    { "posix1version", CTLTYPE_INT }, \
    { "ngroups", CTLTYPE_INT }, \
    { "job_control", CTLTYPE_INT }, \
    { "saved_ids", CTLTYPE_INT }, \
    { "boottime", CTLTYPE_STRUCT }, \
    { "maxtexts", CTLTYPE_INT }, \
    { "text", CTLTYPE_STRUCT }, \
    { "acctthresh", CTLTYPE_INT }, \
}
#endif

/*
 * KERN_PROC subtypes
 */
#define KERN_PROC_ALL       0   /* everything */
#define KERN_PROC_PID       1   /* by process id */
#define KERN_PROC_PGRP      2   /* by process group id */
#define KERN_PROC_SESSION   3   /* by session of pid - NOT IN 2.11 */
#define KERN_PROC_TTY       4   /* by controlling tty */
#define KERN_PROC_UID       5   /* by effective uid */
#define KERN_PROC_RUID      6   /* by real uid */

/*
 * KERN_PROC subtype ops return arrays of augmented proc structures:
 */
struct kinfo_proc {
    struct proc kp_proc;        /* proc structure */
    struct eproc {
        struct  proc *e_paddr;  /* address of proc */
        dev_t   e_tdev;         /* controlling tty dev */
        pid_t   e_tpgid;        /* tty process group id */
        uid_t   e_ruid;         /* real uid */
    } kp_eproc;
};

/*
 * KERN_INODE returns an array of augmented inode structures:
*/
struct  kinfo_inode {
    struct inode    *kp_inodep; /* address of inode */
    struct inode    kp_inode;   /* inode structure */
};

/*
 * KERN_FILE returns an array of augmented file structures:
*/
struct  kinfo_file {
    struct file *kp_filep;      /* address of file */
    struct file kp_file;        /* file structure */
};

/*
 * CTL_HW identifiers
 */
#define HW_MACHINE      1       /* string: machine class */
#define HW_MODEL        2       /* string: specific machine model */
#define HW_NCPU         3       /* int: number of cpus */
#define HW_BYTEORDER    4       /* int: machine byte order */
#define HW_PHYSMEM      5       /* int: total memory */
#define HW_USERMEM      6       /* int: non-kernel memory */
#define HW_PAGESIZE     7       /* int: software page size */
#define HW_DISKNAMES    8       /* strings: disk drive names */
#define HW_DISKSTATS    9       /* struct: diskstats[] */
#define HW_MAXID        10      /* number of valid hw ids */

#ifndef KERNEL
#define CTL_HW_NAMES { \
    { 0, 0 }, \
    { "machine", CTLTYPE_STRING }, \
    { "model", CTLTYPE_STRING }, \
    { "ncpu", CTLTYPE_INT }, \
    { "byteorder", CTLTYPE_INT }, \
    { "physmem", CTLTYPE_LONG }, \
    { "usermem", CTLTYPE_LONG }, \
    { "pagesize", CTLTYPE_INT }, \
    { "disknames", CTLTYPE_STRUCT }, \
    { "diskstats", CTLTYPE_STRUCT }, \
}
#endif

#ifndef KERNEL
/*
 * CTL_USER definitions
 */
#define USER_CS_PATH            1   /* string: _CS_PATH */
#define USER_BC_BASE_MAX        2   /* int: BC_BASE_MAX */
#define USER_BC_DIM_MAX         3   /* int: BC_DIM_MAX */
#define USER_BC_SCALE_MAX       4   /* int: BC_SCALE_MAX */
#define USER_BC_STRING_MAX      5   /* int: BC_STRING_MAX */
#define USER_COLL_WEIGHTS_MAX   6   /* int: COLL_WEIGHTS_MAX */
#define USER_EXPR_NEST_MAX      7   /* int: EXPR_NEST_MAX */
#define USER_LINE_MAX           8   /* int: LINE_MAX */
#define USER_RE_DUP_MAX         9   /* int: RE_DUP_MAX */
#define USER_POSIX2_VERSION     10  /* int: POSIX2_VERSION */
#define USER_POSIX2_C_BIND      11  /* int: POSIX2_C_BIND */
#define USER_POSIX2_C_DEV       12  /* int: POSIX2_C_DEV */
#define USER_POSIX2_CHAR_TERM   13  /* int: POSIX2_CHAR_TERM */
#define USER_POSIX2_FORT_DEV    14  /* int: POSIX2_FORT_DEV */
#define USER_POSIX2_FORT_RUN    15  /* int: POSIX2_FORT_RUN */
#define USER_POSIX2_LOCALEDEF   16  /* int: POSIX2_LOCALEDEF */
#define USER_POSIX2_SW_DEV      17  /* int: POSIX2_SW_DEV */
#define USER_POSIX2_UPE         18  /* int: POSIX2_UPE */
#define USER_STREAM_MAX         19  /* int: POSIX2_STREAM_MAX */
#define USER_TZNAME_MAX         20  /* int: POSIX2_TZNAME_MAX */
#define USER_MAXID              21  /* number of valid user ids */

#define CTL_USER_NAMES { \
    { 0, 0 }, \
    { "cs_path", CTLTYPE_STRING }, \
    { "bc_base_max", CTLTYPE_INT }, \
    { "bc_dim_max", CTLTYPE_INT }, \
    { "bc_scale_max", CTLTYPE_INT }, \
    { "bc_string_max", CTLTYPE_INT }, \
    { "coll_weights_max", CTLTYPE_INT }, \
    { "expr_nest_max", CTLTYPE_INT }, \
    { "line_max", CTLTYPE_INT }, \
    { "re_dup_max", CTLTYPE_INT }, \
    { "posix2_version", CTLTYPE_INT }, \
    { "posix2_c_bind", CTLTYPE_INT }, \
    { "posix2_c_dev", CTLTYPE_INT }, \
    { "posix2_char_term", CTLTYPE_INT }, \
    { "posix2_fort_dev", CTLTYPE_INT }, \
    { "posix2_fort_run", CTLTYPE_INT }, \
    { "posix2_localedef", CTLTYPE_INT }, \
    { "posix2_sw_dev", CTLTYPE_INT }, \
    { "posix2_upe", CTLTYPE_INT }, \
    { "stream_max", CTLTYPE_INT }, \
    { "tzname_max", CTLTYPE_INT }, \
}
#endif

/*
 * CTL_DEBUG definitions
 *
 * Second level identifier specifies which debug variable.
 * Third level identifier specifies which stucture component.
 */
#define CTL_DEBUG_NAME      0   /* string: variable name */
#define CTL_DEBUG_VALUE     1   /* int: variable value */
#define CTL_DEBUG_MAXID     20

/*
 * Locking and stats
 */
struct sysctl_lock {
    int sl_lock;
    int sl_want;
    int sl_locked;
};

#ifdef  KERNEL

#ifdef  DEBUG
/*
 * CTL_DEBUG variables.
 *
 * These are declared as separate variables so that they can be
 * individually initialized at the location of their associated
 * variable. The loader prevents multiple use by issuing errors
 * if a variable is initialized in more than one place. They are
 * aggregated into an array in debug_sysctl(), so that it can
 * conveniently locate them when querried. If more debugging
 * variables are added, they must also be declared here and also
 * entered into the array.
 */
struct ctldebug {
    char    *debugname; /* name of debugging variable */
    int     *debugvar;  /* pointer to debugging variable */
};
extern struct ctldebug debug0, debug1, debug2, debug3, debug4;
extern struct ctldebug debug5, debug6, debug7, debug8, debug9;
extern struct ctldebug debug10, debug11, debug12, debug13, debug14;
extern struct ctldebug debug15, debug16, debug17, debug18, debug19;
#endif  /* DEBUG */

extern struct sysctl_lock memlock;

/*
 * Internal sysctl function calling convention:
 *
 *  (*sysctlfn)(name, namelen, oldval, oldlenp, newval, newlen);
 *
 * The name parameter points at the next component of the name to be
 * interpreted.  The namelen parameter is the number of integers in
 * the name.
 */
typedef int (sysctlfn) (int *name, u_int namelen,
    void *oldp, size_t *oldlenp, void *newp, size_t newlen);

/*
 * Get old / set new parameters for an integer value.
 */
int sysctl_int (void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, int *valp);

/*
 * As above, but read-only.
 */
int sysctl_rdint (void *oldp, size_t *oldlenp, void *newp, int val);

/*
 * Get old / set new parameters for an long value.
 */
int sysctl_long (void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, long *valp);

/*
 * As above, but read-only.
 */
int sysctl_rdlong (void *oldp, size_t *oldlenp, void *newp, long val);

/*
 * Get old / set new parameters for a string value.
 */
int sysctl_string (void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, char *str, int maxlen);

/*
 * As above, but read-only.
 */
int sysctl_rdstring (void *oldp, size_t *oldlenp, void *newp, const char *str);

/*
 * Get old parameters for a structure.
 */
int sysctl_rdstruct (void *oldp, size_t *oldlenp,
    void *newp, void *sp, int len);

#else   /* !KERNEL */

int sysctl (int *name, u_int namelen, void *oldp, size_t *oldlenp,
        void *newp, size_t newlen);

int __sysctl (int *name, u_int namelen, void *oldp, size_t *oldlenp,
        void *newp, size_t newlen);

#endif  /* KERNEL */
#endif  /* !_SYS_SYSCTL_H_ */
