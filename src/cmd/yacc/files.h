/*
 *	@(#)files	4.1.1	(2.11BSD)	1996/10/24
 */
/* this file has the location of the parser, and the size of the progam desired */
/* It may also contain definitions to override various defaults: for example,
 * WORD32 tells yacc that there are at least 32 bits per int */
/* on some systems, notably IBM, the names for the output files and tempfiles must
 * also be changed  */

/*
 * #ifndef WORD32
 * #define WORD32
 * #endif
 */
/* location of the parser text file */
#define PARSER "/usr/share/misc/yaccpar.c"

/* basic size of the Yacc implementation */
/* # define HUGE */
/* #define MEDIUM */
#define TINY
