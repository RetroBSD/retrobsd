/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#ifdef SYSV
#include <sys/ioctl.h>
#endif
#include <pwd.h>
#include <fcntl.h>

#ifdef BSD
#include <sgtty.h>
#include <sys/time.h>
#include <sys/file.h>
#endif

#ifdef SYSV
#include <termios.h>
#endif

#include <signal.h>
#include <math.h>
#include <curses.h>

#define bcopy(a,b,c)	memcpy((b), (a), (c))
#define	bzero(a,b)	memset((a), '\0', (b))
#ifdef SYSV
#define	sgttyb	termios
#define	sg_erase c_cc[2]
#define	sg_kill c_cc[3]
#endif

#include "def.h"
#include "struct.h"
#include "extern.h"
#include "tunable.h"
