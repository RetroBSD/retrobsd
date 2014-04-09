#include <stdio.h>
#include <varargs.h>
#include <sys/fcntl.h>
#include "dup.h"

/* Debugging routines */
#define TRACEF "/usr/abs/sh/trace"

static int fp = (-1);
unsigned was_traced = 0;

trace( fmt, va_alist )
char *fmt;
va_dcl
{
	va_list args;
	char buf[256];

	if( fp < 0){
		fp = creat( TRACEF, 0644 );
		if( fp < 0 ) exit(13);
		fcntl( fp, F_SETFD, EXCLOSE );
	}

	va_start( args );
/*        vsprintf( buf, fmt, args );   */
	strcpy( buf, fmt );
	write( fp, buf, strlen(buf));
	va_end( args );
	was_traced++;
}

