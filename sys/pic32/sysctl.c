/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "ioctl.h"
#include "proc.h"
#include "kernel.h"
#include "file.h"
#include "inode.h"
#include "sysctl.h"
#include "cpu.h"
#include "tty.h"
#include "systm.h"
#include "dk.h"
#include "vmsystm.h"
#include "ptrace.h"
#include "namei.h"
#include "vmmeter.h"
#include "map.h"
#include "conf.h"

/*
 * Errno messages.
 */
static const char *errlist[] = {
	"Undefined error: 0",			/*  0 - ENOERROR */
	"Operation not permitted",		/*  1 - EPERM */
	"No such file or directory",		/*  2 - ENOENT */
	"No such process",			/*  3 - ESRCH */
	"Interrupted system call",		/*  4 - EINTR */
	"Input/output error",			/*  5 - EIO */
	"Device not configured",		/*  6 - ENXIO */
	"Argument list too long",		/*  7 - E2BIG */
	"Exec format error",			/*  8 - ENOEXEC */
	"Bad file descriptor",			/*  9 - EBADF */
	"No child processes",			/* 10 - ECHILD */
	"No more processes",			/* 11 - EAGAIN */
	"Cannot allocate memory",		/* 12 - ENOMEM */
	"Permission denied",			/* 13 - EACCES */
	"Bad address",				/* 14 - EFAULT */
	"Block device required",		/* 15 - ENOTBLK */
	"Device busy",				/* 16 - EBUSY */
	"File exists",				/* 17 - EEXIST */
	"Cross-device link",			/* 18 - EXDEV */
	"Operation not supported by device",	/* 19 - ENODEV */
	"Not a directory",			/* 20 - ENOTDIR */
	"Is a directory",			/* 21 - EISDIR */
	"Invalid argument",			/* 22 - EINVAL */
	"Too many open files in system",	/* 23 - ENFILE */
	"Too many open files",			/* 24 - EMFILE */
	"Inappropriate ioctl for device",	/* 25 - ENOTTY */
	"Text file busy",			/* 26 - ETXTBSY */
	"File too large",			/* 27 - EFBIG */
	"No space left on device",		/* 28 - ENOSPC */
	"Illegal seek",				/* 29 - ESPIPE */
	"Read-only file system",		/* 30 - EROFS */
	"Too many links",			/* 31 - EMLINK */
	"Broken pipe",				/* 32 - EPIPE */

/* math software */
	"Numerical argument out of domain",	/* 33 - EDOM */
	"Result too large",			/* 34 - ERANGE */

/* non-blocking and interrupt i/o */
	"Resource temporarily unavailable",	/* 35 - EWOULDBLOCK */
	"Operation now in progress",		/* 36 - EINPROGRESS */
	"Operation already in progress",	/* 37 - EALREADY */

/* ipc/network software -- argument errors */
	"Socket operation on non-socket",	/* 38 - ENOTSOCK */
	"Destination address required",		/* 39 - EDESTADDRREQ */
	"Message too long",			/* 40 - EMSGSIZE */
	"Protocol wrong type for socket",	/* 41 - EPROTOTYPE */
	"Protocol not available",		/* 42 - ENOPROTOOPT */
	"Protocol not supported",		/* 43 - EPROTONOSUPPORT */
	"Socket type not supported",		/* 44 - ESOCKTNOSUPPORT */
	"Operation not supported",		/* 45 - EOPNOTSUPP */
	"Protocol family not supported",	/* 46 - EPFNOSUPPORT */
						/* 47 - EAFNOSUPPORT */
	"Address family not supported by protocol family",
	"Address already in use",		/* 48 - EADDRINUSE */
	"Can't assign requested address",	/* 49 - EADDRNOTAVAIL */

/* ipc/network software -- operational errors */
	"Network is down",			/* 50 - ENETDOWN */
	"Network is unreachable",		/* 51 - ENETUNREACH */
	"Network dropped connection on reset",	/* 52 - ENETRESET */
	"Software caused connection abort",	/* 53 - ECONNABORTED */
	"Connection reset by peer",		/* 54 - ECONNRESET */
	"No buffer space available",		/* 55 - ENOBUFS */
	"Socket is already connected",		/* 56 - EISCONN */
	"Socket is not connected",		/* 57 - ENOTCONN */
	"Can't send after socket shutdown",	/* 58 - ESHUTDOWN */
	"Too many references: can't splice",	/* 59 - ETOOMANYREFS */
	"Operation timed out",			/* 60 - ETIMEDOUT */
	"Connection refused",			/* 61 - ECONNREFUSED */

	"Too many levels of symbolic links",	/* 62 - ELOOP */
	"File name too long",			/* 63 - ENAMETOOLONG */

/* should be rearranged */
	"Host is down",				/* 64 - EHOSTDOWN */
	"No route to host",			/* 65 - EHOSTUNREACH */
	"Directory not empty",			/* 66 - ENOTEMPTY */

/* quotas & mush */
	"Too many processes",			/* 67 - EPROCLIM */
	"Too many users",			/* 68 - EUSERS */
	"Disc quota exceeded",			/* 69 - EDQUOT */

/* Network File System */
	"Stale NFS file handle",		/* 70 - ESTALE */
	"Too many levels of remote in path",	/* 71 - EREMOTE */
	"RPC struct is bad",			/* 72 - EBADRPC */
	"RPC version wrong",			/* 73 - ERPCMISMATCH */
	"RPC prog. not avail",			/* 74 - EPROGUNAVAIL */
	"Program version wrong",		/* 75 - EPROGMISMATCH */
	"Bad procedure for program",		/* 76 - EPROCUNAVAIL */

	"No locks available",			/* 77 - ENOLCK */
	"Function not implemented",		/* 78 - ENOSYS */
	"Inappropriate file type or format",	/* 79 - EFTYPE */
	"Authentication error",			/* 80 - EAUTH */
	"Need authenticator",			/* 81 - ENEEDAUTH */
};

/*
 * Kernel symbol name list.
 */
static const struct {
        const char *name;
        int addr;
} nlist[] = {
        { "_boottime",      (int) &boottime     },  /* vmstat */
        { "_cnttys",        (int) &cnttys       },  /* pstat */
        { "_cp_time",       (int) &cp_time      },  /* iostat  vmstat */
        { "_dk_busy",       (int) &dk_busy      },  /* iostat */
        { "_dk_name",       (int) &dk_name      },  /* iostat  vmstat */
        { "_dk_ndrive",     (int) &dk_ndrive    },  /* iostat  vmstat */
        { "_dk_unit",       (int) &dk_unit      },  /* iostat  vmstat */
        { "_dk_bytes",      (int) &dk_bytes     },  /* iostat */
        { "_dk_xfer",       (int) &dk_xfer      },  /* iostat  vmstat */
        { "_file",          (int) &file         },  /* pstat */
        { "_forkstat",      (int) &forkstat     },  /* vmstat */
#ifdef UCB_METER
        { "_freemem",       (int) &freemem      },  /* vmstat */
#endif
        { "_hz",            (int) &hz           },  /* ps */
        { "_inode",         (int) &inode        },  /* pstat */
        { "_ipc",           (int) &ipc          },  /* ps */
        { "_lbolt",         (int) &lbolt        },  /* ps */
        { "_memlock",       (int) &memlock      },  /* ps */
        { "_nchstats",      (int) &nchstats     },  /* vmstat */
        { "_nproc",         (int) &nproc        },  /* ps      pstat */
        { "_nswap",         (int) &nswap        },  /* pstat */
        { "_proc",          (int) &proc         },  /* ps      pstat */
        { "_runin",         (int) &runin        },  /* ps */
        { "_runout",        (int) &runout       },  /* ps */
        { "_selwait",       (int) &selwait      },  /* ps */
        { "_swapmap",       (int) &swapmap      },  /* pstat */
        { "_tk_nin",        (int) &tk_nin       },  /* iostat */
        { "_tk_nout",       (int) &tk_nout      },  /* iostat */
        { "_total",         (int) &total        },  /* vmstat */
        { "_u",             (int) &u            },  /* ps */
#if NPTY > 0
        { "_npty",          (int) &npty         },  /* pstat */
        { "_pt_tty",        (int) &pt_tty       },  /* pstat */
#endif
#ifdef UCB_METER
        { "_rate",          (int) &rate         },  /* vmstat */
        { "_sum",           (int) &sum          },  /* vmstat */
#endif
        { "_bdevsw",        (int) &bdevsw       },
        { "_cdevsw",        (int) &cdevsw       },
        { 0, 0 },
};

/*
 * ucall allows user level code to call various kernel functions.
 * Autoconfig uses it to call the probe and attach routines of the
 * various device drivers.
 */
void
ucall()
{
	register struct a {
		int priority;
		int (*routine)();
		int arg1;
		int arg2;
	} *uap = (struct a *)u.u_arg;
	int s;

	if (!suser())
		return;
	switch (uap->priority) {
	case 0:
		s = spl0();
		break;
	default:
		s = splhigh();
		break;
	}
	u.u_rval = (*uap->routine) (uap->arg1, uap->arg2);
	splx(s);
}

/*
 * Fetch the word at addr from flash memory or i/o port.
 * This system call is required on PIC32 because in user mode
 * the access to flash memory region is not allowed.
 */
void
ufetch()
{
        unsigned addr = *(unsigned*) u.u_arg & ~3;

        /* Check root privileges */
	if (! suser())
		return;

        /* Low memory address - assume peripheral i/o space.  */
	if (addr < 0x90000)
                addr += 0xbf800000;

        /* Check address */
	if (! (addr >= 0x9d000000 && addr < 0x9d000000 + FLASH_SIZE) &&
	    ! (addr >= 0xbd000000 && addr < 0xbd000000 + FLASH_SIZE) &&

            /* Boot flash memory */
	    ! (addr >= 0x9fc00000 && addr < 0x9fc00000 + 12*1024) &&
	    ! (addr >= 0xbfc00000 && addr < 0xbfc00000 + 12*1024) &&

	    /* Peripheral registers */
	    ! (addr >= 0xbf800000 && addr < 0xbf810000) &&
	    ! (addr >= 0xbf880000 && addr < 0xbf890000)) {
                u.u_error = EFAULT;
		return;
        }
	u.u_rval = *(unsigned*) addr;
}

/*
 * Store the word at addr of i/o port.
 */
void
ustore()
{
	register struct a {
	    unsigned addr;
	    unsigned value;
	} *uap = (struct a *)u.u_arg;
        unsigned addr = uap->addr & ~3;

        /* Check root privileges */
	if (! suser())
		return;

        /* Low memory address - assume peripheral i/o space.  */
	if (addr < 0x90000)
                addr += 0xbf800000;

        /* Check address */
	if (! (addr >= 0xbf800000 && addr < 0xbf810000) &&
	    ! (addr >= 0xbf880000 && addr < 0xbf890000)) {
                u.u_error = EFAULT;
		return;
        }
	*(unsigned*) addr = uap->value;
}

/*
 * This was moved here when the TMSCP portion was added.  At that time it
 * became (even more) system specific and didn't belong in kern_sysctl.c
 */
int
cpu_sysctl (name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
        int i;

	switch (name[0]) {
	case CPU_CONSDEV:
		if (namelen != 1)
			return ENOTDIR;
		return sysctl_rdstruct (oldp, oldlenp, newp,
				&cnttys[0].t_dev, sizeof &cnttys[0].t_dev);
#if NTMSCP > 0
	case CPU_TMSCP:
		/* All sysctl names at this level are terminal */
		if (namelen != 2)
			return ENOTDIR;
		switch (name[1]) {
		case TMSCP_CACHE:
			return sysctl_int (oldp, oldlenp, newp,
				newlen, &tmscpcache);
		case TMSCP_PRINTF:
			return sysctl_int (oldp, oldlenp, newp,
				newlen, &tmscpprintf);
		default:
		}
#endif
	case CPU_ERRMSG:
		if (namelen != 2)
			return ENOTDIR;
                if (name[1] < 1 ||
                    name[1] >= sizeof(errlist) / sizeof(errlist[0]))
			return EOPNOTSUPP;
		return sysctl_string(oldp, oldlenp, 0, 0,
                        (char*) errlist[name[1]],
                        1 + strlen(errlist[name[1]]));

	case CPU_NLIST:
		for (i=0; nlist[i].name; i++) {
		        if (strncmp (newp, nlist[i].name, newlen) == 0) {
			        int addr = nlist[i].addr;
                                if (! oldp)
                                        return 0;
                                if (*oldlenp < sizeof(int))
                                        return ENOMEM;
                                *oldlenp = sizeof(int);
                                return copyout ((caddr_t) &addr, (caddr_t) oldp, sizeof(int));
			}
		}
		return EOPNOTSUPP;

        case CPU_TIMO_CMD:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_cmd);
        case CPU_TIMO_SEND_OP:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_send_op);
        case CPU_TIMO_SEND_CSD:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_send_csd);
        case CPU_TIMO_READ:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_read);
        case CPU_TIMO_WAIT_CMD:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_wait_cmd);
        case CPU_TIMO_WAIT_WDATA:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_wait_wdata);
        case CPU_TIMO_WAIT_WDONE:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_wait_wdone);
        case CPU_TIMO_WAIT_WSTOP:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_wait_wstop);
        case CPU_TIMO_WAIT_WIDLE:
		return sysctl_int(oldp, oldlenp, newp, newlen, &sd_timo_wait_widle);

	default:
		return EOPNOTSUPP;
	}
	/* NOTREACHED */
}
