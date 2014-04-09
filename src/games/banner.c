/*
 * banner - prints large signs
 * banner [-w#] [-p@] [-t] message ...
 *
 * Written by Vadim Antonov, MSU 28.09.1985
 * Ported to RetroBSD by Serge Vakulenko, 2012
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#define MAXMSG 100      /* max chars in message */
#define DWIDTH 160      /* max output line width */
#define SPACEW 5        /* space width */
#define DPUNCH "#"      /* symbol to use for printing */

struct font_t {
        char code;
        char glyph [7];
};

int width = DWIDTH;	/* -w option: scrunch letters to 80 columns */
char *punch = DPUNCH;	/* -p option: punch symbol, default # */
int trace;
char line [DWIDTH], *linep;
char message [MAXMSG];
int nchars;

/*
 * Font 7x7, variable width
 */
#define ROW(a,b,c,d,e,f,g) (a<<6 | b<<5 | c<<4 | d<<3 | e<<2 | f<<1 | g)
#define _ 0
#define O 1

const struct font_t font[] = {
	{ '0', {
		ROW (_,O,O,O,O,O,_),
		ROW (O,_,_,_,_,O,O),
		ROW (O,_,_,_,O,_,O),
		ROW (O,_,_,O,_,_,O),
		ROW (O,_,O,_,_,_,O),
		ROW (O,O,_,_,_,_,O),
		ROW (_,O,O,O,O,O,_),
	}}, { '1', {
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,O,_,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,O,O,_,_,_),
	}}, { '2', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
	}}, { '3', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,O,O,_,_),
		ROW (_,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { '4', {
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,O,_,_),
		ROW (_,_,O,_,O,_,_),
		ROW (_,O,_,_,O,_,_),
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,_,O,_,_),
	}}, { '5', {
		ROW (O,O,O,O,O,O,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,O,_,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { '6', {
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { '7', {
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
	}}, { '8', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { '9', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
	}}, { ';', {
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '+', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (O,O,O,O,O,O,O),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '!', {
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '"', {
		ROW (O,_,O,_,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '#', {
		ROW (_,_,O,_,O,_,_),
		ROW (_,_,O,_,O,_,_),
		ROW (O,O,O,O,O,O,O),
		ROW (_,_,O,_,O,_,_),
		ROW (O,O,O,O,O,O,O),
		ROW (_,_,O,_,O,_,_),
		ROW (_,_,O,_,O,_,_),
	}}, { '$', {
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
	}}, { '%', {
		ROW (_,O,O,_,_,_,O),
		ROW (_,O,O,_,_,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,O,O),
		ROW (O,_,_,_,_,O,O),
	}}, { '&', {
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,O,_,_,O),
		ROW (O,_,_,_,O,O,_),
		ROW (O,_,_,_,O,O,_),
		ROW (_,O,O,O,_,_,O),
	}}, { '\'', {
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '(', {
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
	}}, { ')', {
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '=', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '-', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { ':', {
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '*', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,_,O,_,O,_),
		ROW (_,_,O,O,O,_,_),
		ROW (O,O,O,O,O,O,O),
		ROW (_,_,O,O,O,_,_),
		ROW (_,O,_,O,_,O,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '\\', {
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,_,_,O),
	}}, { '|', {
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '>', {
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '<', {
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,_,O,_,_,_),
	}}, { '.', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
	}}, { ',', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '_', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
	}}, { '[', {
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,_,_,_),
	}}, { ']', {
		ROW (O,O,O,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (O,O,O,O,_,_,_),
	}}, { '{', {
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
	}}, { '}', {
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '/', {
		ROW (_,_,_,_,_,_,O),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { '?', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,O,_,_,_),
	}}, { '^', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '~', {
		ROW (O,O,O,O,O,O,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { '@', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,_,_,_,_,O,_),
		ROW (_,O,O,_,_,O,_),
		ROW (O,_,_,O,_,O,_),
		ROW (O,_,_,O,_,O,_),
		ROW (_,O,O,_,O,_,_),
	}}, { '`', {
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
	}}, { 'A', {
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,O,_,_),
		ROW (_,O,_,_,_,O,_),
		ROW (O,_,_,_,_,_,O),
		ROW (O,O,O,O,O,O,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
	}}, { 'a', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,O,O,_,O,_),
	}}, { 'B', {
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,O,O,O,O,_,_),
	}}, { 'b', {
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,O,O,O,_,_,_),
	}}, { 'C', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { 'c', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,O,O,O,_,_),
	}}, { 'D', {
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,O,O,O,_,_,_),
	}}, { 'd', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,O,O,O,_,_),
	}}, { 'E', {
		ROW (O,O,O,O,O,O,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
	}}, { 'e', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,O,O,_,_,_),
	}}, { 'F', {
		ROW (O,O,O,O,O,O,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { 'f', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
	}}, { 'G', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,O,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,O,_),
	}}, { 'g', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,O,O,_,_,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,O,O,O,_,_,_),
	}}, { 'H', {
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,O,O,O,O,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
	}}, { 'h', {
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
	}}, { 'I', {
		ROW (O,O,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,_,_,_,_),
	}}, { 'i', {
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
	}}, { 'J', {
		ROW (_,_,O,O,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (_,O,O,_,_,_,_),
	}}, { 'j', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
	}}, { 'K', {
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
	}}, { 'k', {
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (O,_,O,_,_,_,_),
		ROW (O,_,_,O,_,_,_),
	}}, { 'L', {
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,O,O,O,O,O,_),
	}}, { 'l', {
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
	}}, { 'M', {
		ROW (O,_,_,_,_,_,O),
		ROW (O,O,_,_,_,O,O),
		ROW (O,_,O,_,O,_,O),
		ROW (O,_,_,O,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
	}}, { 'm', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,_,O,_,_,_),
		ROW (O,_,O,_,O,_,_),
		ROW (O,_,O,_,O,_,_),
		ROW (O,_,O,_,O,_,_),
		ROW (O,_,O,_,O,_,_),
	}}, { 'N', {
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,O,_,_,_,O,_),
		ROW (O,_,O,_,_,O,_),
		ROW (O,_,_,O,_,O,_),
		ROW (O,_,_,_,O,O,_),
		ROW (O,_,_,_,_,O,_),
	}}, { 'n', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,O,O,_,_,_),
		ROW (O,O,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
	}}, { 'O', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { 'o', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,O,O,_,_,_),
	}}, { 'P', {
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { 'p', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,O,O,O,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { 'Q', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,O,_,O,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,O,O,_,O,_),
	}}, { 'q', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,O,_,_,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (_,O,O,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
	}}, { 'R', {
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,O,O,O,O,_,_),
		ROW (O,_,_,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,_,O,_),
	}}, { 'r', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,O,O,_,_,_),
		ROW (O,O,_,_,O,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (O,_,_,_,_,_,_),
	}}, { 'S', {
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,O,O,O,_,_),
		ROW (_,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { 's', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,O,O,O,_,_),
		ROW (O,_,_,_,_,_,_),
		ROW (_,O,O,O,_,_,_),
		ROW (_,_,_,_,O,_,_),
		ROW (O,O,O,O,_,_,_),
	}}, { 'T', {
		ROW (O,O,O,O,O,O,O),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
	}}, { 't', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
	}}, { 'U', {
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (O,_,_,_,_,O,_),
		ROW (_,O,O,O,O,_,_),
	}}, { 'u', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,O,O,_,O,_),
	}}, { 'V', {
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (_,O,_,_,_,O,_),
		ROW (_,_,O,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
	}}, { 'v', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
	}}, { 'W', {
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,O,_,_,O),
		ROW (O,_,O,_,O,_,O),
		ROW (_,O,_,_,_,O,_),
	}}, { 'w', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,O,_,O,_,_),
		ROW (O,_,O,_,O,_,_),
		ROW (_,O,_,O,_,_,_),
	}}, { 'X', {
		ROW (O,_,_,_,_,_,O),
		ROW (_,O,_,_,_,O,_),
		ROW (_,_,O,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,O,_,_),
		ROW (_,O,_,_,_,O,_),
		ROW (O,_,_,_,_,_,O),
	}}, { 'x', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,O,_,_,_),
		ROW (O,_,_,_,O,_,_),
	}}, { 'Y', {
		ROW (O,_,_,_,_,_,O),
		ROW (O,_,_,_,_,_,O),
		ROW (_,O,_,_,_,O,_),
		ROW (_,_,O,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,_,O,_,_,_),
	}}, { 'y', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (O,_,_,_,O,_,_),
		ROW (_,O,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,_,O,_,_,_,_),
	}}, { 'Z', {
		ROW (O,O,O,O,O,O,O),
		ROW (_,_,_,_,_,O,_),
		ROW (_,_,_,_,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,O,O,O,O),
	}}, { 'z', {
		ROW (_,_,_,_,_,_,_),
		ROW (_,_,_,_,_,_,_),
		ROW (O,O,O,O,O,_,_),
		ROW (_,_,_,O,_,_,_),
		ROW (_,_,O,_,_,_,_),
		ROW (_,O,_,_,_,_,_),
		ROW (O,O,O,O,O,_,_),
        }},
};

/*
 * Draw a symbol
 */
int setchar (linep, glyph)
        char *linep;
        char glyph[7];
{
        register int mask, w, c, morebits;

        w = 0;
        morebits = glyph[0] | glyph[1] | glyph[2] | glyph[3] |
                glyph[4] | glyph[5] | glyph[6];
        for (mask = 0x80; morebits && mask != 0; mask >>= 1) {
                c = 0;
                if (glyph[0] & mask)
                        c |= 0x01;
                if (glyph[1] & mask)
                        c |= 0x02;
                if (glyph[2] & mask)
                        c |= 0x04;
                if (glyph[3] & mask)
                        c |= 0x08;
                if (glyph[4] & mask)
                        c |= 0x10;
                if (glyph[5] & mask)
                        c |= 0x20;
                if (glyph[6] & mask)
                        c |= 0x40;
                *linep++ = c;
                w++;
                morebits &= ~mask;
        }
        return w;
}

int main (argc, argv)
        int argc;
        char **argv;
{
        int i, j;

	if (argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
		case 'w':
			width = atoi(&argv[1][2]);
			if (width == 0)
				width = 80;
			break;
		case 'p':
			punch = &argv[1][2];
			if (*punch == 0)
				punch = DPUNCH;
			break;
		case 't':
			trace++;
			break;
		default:
			printf("bad switch %s\n",argv[1]);
			break;
		}
		argc--;
		argv++;
	}

	/* Have now read in the data. Next get the message to be printed. */
	if (argc > 1) {
		strcpy(message, argv[1]);
		for (i=2; i<argc; i++) {
			strcat(message, " ");
			strcat(message, argv[i]);
		}
	} else {
		fprintf(stderr,"Message: ");
		if (! gets(message))
		    return 0;
	}
	nchars = strlen(message);
	if (trace)
		printf("Message '%s'\n", message);

	/*
         * Clear image.
         */
	for (j=0; j<DWIDTH; j++)
                line[j] = 0;

	/*
         * Now have message. Draw it one character at a time.
         */
        linep = line;
	for (i=0; i<nchars; i++) {
                const struct font_t *f;
                char c = message[i];

	        if (linep >= line + width)
		        break;
		if (trace)
			printf("Char #%d: %c\n", i, c);
                if (c == ' ') {
                        linep += SPACEW;
                        continue;
                }
                for (f=font; f->code!=0; f++) {
                        if (f->code == c) {
                                linep += setchar (linep, f->glyph) + 1;
                                break;
                        }
                }
	}

	/*
         * Print the resulting image.
         */
        for (i = 1; i < 0200 ; i <<= 1) {
                char *limit = &line[width-1];
                while (! (*limit & i))
                        limit--;
                for (linep = line; linep <= limit; linep++) {
                        if (*linep & i)
                                fputs (punch, stdout);
                        else
                                putchar (' ');
                }
                putchar ('\n');
        }

	exit(0);
}
