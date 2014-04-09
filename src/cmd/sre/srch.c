/*  A weekend project to try to emulate the original RAND text editor 
    Copyright (C) 2011  Mike Stabenfeldt

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#include "edit.h"
extern globalStruct globalData;

getWordUnderCursor(windowStruct *w, char **wordP)
{
int i, j, len, wlen;
fileStruct *fs;
lineStruct *ls;
char *line;

    fs = w->fileInfo;
    getCursorLine(w, &ls);
    len = getChars(fs, ls, ls->numChars+1, &line);
    if( len < w->cursorX )
	{
NOTHING:
	dingMsg("No word under cursor");
	return(-1);
	}

    if( !isalnum(line[w->cursorX-1]) )
	goto NOTHING;

    /* Backup to beginning of word */
    for(i=w->cursorX-1; i>0; i--)
	if( !isalnum(line[i]) )
	    {
	    i++;
	    break;
	    }

    for(j=i+1; j<len; j++)
	if( !isalnum(line[j]) )
	    break;

    wlen = 1+j-i;
    if(wlen > globalData.wordBufferSize )
	{
	if( wlen < 80 )
	    wlen = 80;
	if( globalData.wordBuffer )
	    free(globalData.wordBuffer);
	globalData.wordBuffer = (char *) malloc(wlen);
	globalData.wordBufferSize = wlen;
	}

    wlen = j-i;
    memcpy(globalData.wordBuffer, &line[i], wlen);
    globalData.wordBuffer[wlen] = '\0';
    *wordP = globalData.wordBuffer;
    return(i);
}

searchDnCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls;
int i, cx, startLineNo, startX, foundLineNo, foundX;
int keyLen, lineLen, needRedraw=0;
char *tmp, *line;

    if( !(fs=w->fileInfo) )
	{
	emsg("Open a file before search");
	return(0);
	}
    if( argc > 1 )
	{
	if( argv[1][0] )
	    {
	    if( (i=strlen(argv[1])+1) >= globalData.srchBufferSize )
		{
		if( globalData.srchBuffer )
		    free(globalData.srchBuffer);
		if( i < 128 )
		    i = 128;
		globalData.srchBufferSize = i;
		globalData.srchBuffer = (char *)malloc(i);
		}
	    strcpy(globalData.srchBuffer, argv[1]);
	    }
	else    /* Search for word under cursor */
	    {
	    if( getWordUnderCursor(w, &tmp) == -1)
		{
		updateCursor(w);
		return(0);
		}
	    if( (i=strlen(tmp)+1) >= globalData.srchBufferSize )
		{
		if( globalData.srchBuffer )
		    free(globalData.srchBuffer);
		if( i < 128 )
		    i = 128;
		globalData.srchBufferSize = i;
		globalData.srchBuffer = (char *)malloc(i);
		}
	    strcpy(globalData.srchBuffer, tmp);
	    }
	}

    if( !globalData.srchBuffer || globalData.srchBuffer[0] == '\0' )
	{
	dingMsg("Nothing to search for");
	updateCursor(w);
	return(0);
	}

    tmp = (char *) alloca(strlen(globalData.srchBuffer) + 8);
    strcpy(tmp, "+SRCH: ");
    strcpy(&tmp[7], globalData.srchBuffer);
    emsg(tmp);

    foundLineNo = startLineNo = w->cursorLineNo;
    startX = w->leftOffset + w->cursorX -1;
    for(i=1, ls=w->topLine; i<startLineNo; i++,ls=ls->next)
	if( !ls )
	    goto NOTFOUND;

    keyLen = strlen(globalData.srchBuffer);
    cx = startX+1;
    for( ; ls; foundLineNo++, ls=ls->next)
	{
	if( w->rangeBotNum && foundLineNo + w->topOffset > w->rangeBotNum )
	    break;
	if( ls->numChars != 0 )
	    {
	    lineLen = getChars(fs, ls, ls->numChars+1, &line);
	    for( ; cx<lineLen-keyLen+1; cx++)
		{
		if( strncmp(&line[cx], globalData.srchBuffer, keyLen) == 0)
		    {
		    foundX = cx;
		    goto FOUND;
		    }
		}
	    }
	cx = 0;
	}
NOTFOUND:
    dingMsg("Search failed");
    updateCursor(w);
    return(0);

FOUND:
    if( 1+foundX - w->leftOffset >= w->width-1 ||
	    foundX < w->leftOffset )
	{
	needRedraw = 1;
	/* Can we right the screen */
	if( foundX < w->width-1 )
	    w->leftOffset = 0;
	else
	    w->leftOffset = 2+foundX - w->width;
	w->cursorX = 1+foundX - w->leftOffset;
	}
    else
	w->cursorX = 1+foundX - w->leftOffset;
    /* Do we need to adjust vertical ? */
    if( w->cursorLineNo != foundLineNo )
	w->cursorLine = NULL;
    if( foundLineNo < w->height )
	{       /* Nope */
	w->cursorLineNo = foundLineNo;
	updateStatus();
	if( needRedraw )
	    redrawWindow(w);
	else
	    updateCursor(w);
	}
    else
	gotoline(w, w->topOffset + foundLineNo);
/*  emsg("srch key found"); */
    return(1);
}

searchUpCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls;
int i, cx, startLineNo, startX, foundLineNo, foundX;
int keyLen, lineLen, needRedraw=0;
char *tmp, *line;

    if( !(fs=w->fileInfo) )
	{
	emsg("Open a file before search");
	return(0);
	}

    if( argc > 1 )
	{
	if( argv[1][0] )
	    {
	    if( (i=strlen(argv[1])+1) >= globalData.srchBufferSize )
		{
		if( globalData.srchBuffer )
		    free(globalData.srchBuffer);
		if( i < 128 )
		    i = 128;
		globalData.srchBufferSize = i;
		globalData.srchBuffer = (char *)malloc(i);
		}
	    strcpy(globalData.srchBuffer, argv[1]);
	    }
	else    /* Search for word under cursor */
	    {
	    if( getWordUnderCursor(w, &tmp) == -1)
		{
		updateCursor(w);
		return(0);
		}
	    if( (i=strlen(tmp)+1) >= globalData.srchBufferSize )
		{
		if( globalData.srchBuffer )
		    free(globalData.srchBuffer);
		if( i < 128 )
		    i = 128;
		globalData.srchBufferSize = i;
		globalData.srchBuffer = (char *)malloc(i);
		}
	    strcpy(globalData.srchBuffer, tmp);
	    }
	}

    if( !globalData.srchBuffer || globalData.srchBuffer[0] == '\0' )
	{
	dingMsg("Nothing to search for");
	updateCursor(w);
	return(0);
	}

    tmp = (char *) alloca(strlen(globalData.srchBuffer) + 8);
    strcpy(tmp, "-SRCH: ");
    strcpy(&tmp[7], globalData.srchBuffer);
    emsg(tmp);

    startX = w->leftOffset + w->cursorX -1;
    /* Get down to current line, or last avail line */
    for(i=1, ls=w->topLine; i<w->cursorLineNo; i++, ls=ls->next)
	if( !ls->next )
	    break;
    foundLineNo = startLineNo = i;

    keyLen = strlen(globalData.srchBuffer);
    cx = startX-1;
    for( ; ls; foundLineNo--, ls=ls->prev)
	{
	if( foundLineNo + w->topOffset < w->rangeTopNum )
	    break;
	if( ls->numChars != 0 )
	    {
	    lineLen = getChars(fs, ls, ls->numChars+1, &line);
	    if( cx == 0 )
		cx = lineLen-keyLen;
	    for( ; cx>=0; cx--)
		{
		if( strncmp(&line[cx], globalData.srchBuffer, keyLen) == 0)
		    {
		    foundX = cx;
		    goto FOUND;
		    }
		}
	    }
	cx = 0;
	}
NOTFOUND:
    dingMsg("Search failed");
    updateCursor(w);
    return(0);

FOUND:
    if( 1+foundX - w->leftOffset >= w->width-1 ||
	    foundX < w->leftOffset )
	{
	needRedraw = 1;
	/* Can we right the screen */
	if( foundX < w->width-1 )
	    w->leftOffset = 0;
	else
	    w->leftOffset = 2+foundX - w->width;
	w->cursorX = 1+foundX - w->leftOffset;
	}
    else
	w->cursorX = 1+foundX - w->leftOffset;
    /* Do we need to adjust vertical ? */

    if( foundLineNo >= 1 )
	{       /* Nope */
	if( w->cursorLineNo != foundLineNo )
	    {
	    w->cursorLine = NULL;
	    w->cursorLineNo = foundLineNo;
	    }
	updateStatus();
	if( needRedraw )
	    redrawWindow(w);
	updateCursor(w);
	}
    else
	gotoline(w, w->topOffset + foundLineNo);
    return(1);
}

matchCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls;
char *line;
int i, len, searchC[2], cnt;
int startLineNo, foundLineNo, startX, foundX;
int lineLen, needRedraw;
char msg[20];

    if( !(fs=w->fileInfo) )
	{
	emsg("Open a file before match");
	return(0);
	}

    getCursorLine(w, &ls);

    /* Get character under cursor */
    len = getChars(fs, ls, ls->numChars+1, &line);
    i = w->cursorX + w->leftOffset - 1;
    if( len < i )
	{
NOTHING:
	dingMsg("No []{}() under cursor");
	updateCursor(w);
	return(-1);
	}

    if( line[i] == '(' )
	{
	searchC[0] = '(';
	searchC[1] = ')';
	cnt = 1;
	}
    else if( line[i] == ')' )
	{
	searchC[0] = '(';
	searchC[1] = ')';
	cnt = -1;
	}
    else if( line[i] == '[' )
	{
	searchC[0] = '[';
	searchC[1] = ']';
	cnt = 1;
	}
    else if( line[i] == ']' )
	{
	searchC[0] = '[';
	searchC[1] = ']';
	cnt = -1;
	}
    else if( line[i] == '{' )
	{
	searchC[0] = '{';
	searchC[1] = '}';
	cnt = 1;
	}
    else if( line[i] == '}' )
	{
	searchC[0] = '{';
	searchC[1] = '}';
	cnt = -1;
	}
    else
	goto NOTHING;

    sprintf(msg, "match %c%c", searchC[0], searchC[1]);
    emsg(msg);

    startX = w->leftOffset + w->cursorX -1;
    foundLineNo = startLineNo = w->cursorLineNo;

    if( cnt < 0 )   /* looking backward */
	{
	for( ; ls; foundLineNo--, ls=ls->prev)
	    {
	    if( foundLineNo + w->topOffset < w->rangeTopNum )
		break;
	    if( ls->numChars != 0 )
		{
		lineLen = getChars(fs, ls, ls->numChars+1, &line);
		if( startX >= 0 )
		    foundX = startX-1;
		else
		    foundX = lineLen - 1;
		for( ; foundX>=0; foundX--)
		    {
		    if( line[foundX] == searchC[0] )
			cnt++;
		    else if( line[foundX] == searchC[1] )
			cnt--;
		    if( cnt == 0 )
			goto FOUND;
		    }
		}
	    startX = -1;
	    }
	}
    else
	{
	for( ; ls; foundLineNo++, ls=ls->next)
	    {
	    if( w->rangeBotNum && foundLineNo + w->topOffset > w->rangeBotNum )
		break;
	    if( ls->numChars != 0 )
		{
		lineLen = getChars(fs, ls, ls->numChars+1, &line);
		if( startX >= 0 )
		    foundX = startX+1;
		else
		    foundX = 0;
		for( ; foundX<lineLen; foundX++)
		    {
		    if( line[foundX] == searchC[0] )
			cnt++;
		    else if( line[foundX] == searchC[1] )
			cnt--;
		    if( cnt == 0 )
			goto FOUND;
		    }
		}
	    startX = -1;
	    }
	}
NOTFOUND:
    dingMsg("match failed");
    updateCursor(w);
    return(0);

FOUND:
    needRedraw = 0;
    if( 1+foundX - w->leftOffset >= w->width-1 ||
	    foundX < w->leftOffset )
	{
	needRedraw = 1;
	/* Can we right the screen */
	if( foundX < w->width-1 )
	    w->leftOffset = 0;
	else
	    w->leftOffset = 2+foundX - w->width;
	w->cursorX = 1+foundX - w->leftOffset;
	}
    else
	w->cursorX = 1+foundX - w->leftOffset;
    if( foundLineNo != w->cursorLineNo )
	w->cursorLine = NULL;
    /* Do we need to adjust vertical ? */
    if( foundLineNo >= 1 && foundLineNo < w->height )
	{       /* Nope */
	w->cursorLineNo = foundLineNo;
	updateStatus();
	if( needRedraw )
	    redrawWindow(w);
	updateCursor(w);
	}
    else
	gotoline(w, w->topOffset + foundLineNo);
    return(1);
}

/* Assume caller allocated reference/replacement based on size of inString */
static int cutRplString(char *inString, char *reference, char *replace)
{
char delim;
int i, j;

    if( strlen(inString) < 3 )
	{
BAD:
	dingMsg("Bad /ref/replace/");
	return(0);
	}
    delim = inString[0];
    for(i=1, j=0; inString[i]; i++)
	{
	if( inString[i] == delim )
	    {
	    reference[j] = '\0';
	    break;
	    }
	reference[j++] = inString[i];
	}
    /* Must have hit delimiter and reference must be non-null */
    if( ! inString[i] || j == 0 )
	goto BAD;

    for(i++, j=0; inString[i]; i++)
	{
	if( inString[i] == delim )
	    {
	    replace[j] = '\0';
	    break;
	    }
	replace[j++] = inString[i];
	}
    /* Must have hit delimiter and must be end of string */
    if( ! inString[i] || inString[i+1] )
	goto BAD;
    return(1);
}

/* Replace the ref with the new */
/* If marks are set, limit to marked area */
/* If range is set, limit from cursor to end of range */
/* If no range, limit from cursor to end of file */
replaceCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *last = NULL;
int i, start, refLen, replLen, cnt=0;
char *reference, *replace, *line, *s, tmp[50];

    if( !(fs=w->fileInfo) )
	{
	emsg("Open a file before replace");
	return(0);
	}
    if( argc != 2 )
	{
	dingMsg("Specify one arg");
	return(0);
	}

    i = strlen(argv[1]);
    reference = (char *) alloca(i+1);
    replace = (char *) alloca(i+1);
    if( !cutRplString(argv[1], reference, replace) )
	return(0);
    refLen = strlen(reference);
    replLen = strlen(replace);

    getCursorLine(w, &ls);
    if( globalData.markSet )
	{
	if( w->leftOffset + w->cursorX != globalData.markRefX )
	    {
	    dingMsg("Can't replace in rect region");
	    return(0);
	    }
	if( w->topOffset + w->cursorLineNo < globalData.markRefY )
	    {
	    getLine(w, fs, globalData.markRefY-1, &last);
	    }
	else
	    {
	    last = ls;
	    getLine(w, fs, globalData.markRefY-1, &ls);
	    }
	}
    else if( w->rangeBotNum )
	getLine(w, fs, w->rangeBotNum-1, &last);

    if( !canModifyFile(fs) )
	return(0);

    /* We will need to unpack all the lines we are changing */
    for( ; ls ; ls=ls->next)
	{
	if( getChars(fs, ls, ls->numChars+1, &line) < refLen )
	    goto CHK;
	if( strstr(line, reference) )
	    {   /* Replacement needed, must unpack */
	    unpackDiskLine(fs, ls);
	    start = 0;
	    while( (s = strstr( &(ls->data.line[start]), reference)) )
		{
		if( refLen < replLen )
		    {   /* May need to re-alloc */
		    if( ls->numCharsAvail < replLen - refLen + ls->numChars )
			{   /* Re-alloc */
			ls->numCharsAvail = 20 + ls->numChars + replLen - refLen;
			ls->data.line = (char *) realloc(ls->data.line,
						ls->numCharsAvail+1);
			/* Refetch ptr to start of reference */
			s = strstr( &(ls->data.line[start]), reference);
			}
		    }
		if( refLen != replLen )
		    {
		    memmove( &(s[replLen]),
			     &(s[refLen]),
			     1+strlen(&(s[refLen]) ));
		    ls->numChars += replLen - refLen;
		    }
		/* Copy in the replacement chars */
		if( replLen )
		    memcpy(s, replace, replLen);
		cnt++;
		start = s - ls->data.line;
		}
	    }
CHK:
	if( last == ls )
	    break;
	}

    if( cnt )
	redrawWindow(w);

    sprintf(tmp,"Replaced %d", cnt);
    emsg(tmp);
    return(0);
}
