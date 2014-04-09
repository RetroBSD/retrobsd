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
#include <string.h>
#include <ctype.h>
#include <alloca.h>
#include <termios.h>
#include <unistd.h>

#include "edit.h"

extern globalStruct globalData;

/* Only "cmd" that does not take args */
int startArgCmd()
{
char tmp[512];

    if( globalData.argMode != 0 )
	return(0);
    globalData.argMode = 1;
    globalData.argPtr = 0;
    if( globalData.argBufferSize == 0 )
	{
	globalData.argBufferSize = 50;
        globalData.argBuffer = malloc(50);
	}
    /* Put on screen display in status area */
    memset(tmp, ' ', globalData.winWidth/2);
    tmp[globalData.winWidth/2] = '\0';
    printf("\033[%d;1H%s", globalData.winHeight, tmp);
    printf("\033[%d;1HCMD: ", globalData.winHeight);
}

redrawCmd()
{
    redrawScreen();
}

/* When lines are added or deleted, need to validate the alternate files */
/* other windows and ranges are not adversely affected */
/* firstLine is the beginning of the change. numLines is how many lines (+/-) */
/* that were added or deleted. ls is the first line of the change */
checkUpdateAlternate(windowStruct *chgWin, windowStruct *chkWin,
		long long firstLine, long long numLines, int delete)
{
long long cnt;
int i;
fileWinParams *fw;
fileStruct *fs;

    fs = chgWin->fileInfo;
    for(i=0; i<NUM_ALT_FILES; i++)
	{
	if( chkWin->curParamSet == i )
	    continue;
	fw = &(chkWin->editStack[i]);
	if( fw->fileInfo != fs )
	    continue;
	if( delete )
	    {   /* May need to fix up ranges */
	    if( firstLine <= fw->rangeTopNum )
		{
		if( firstLine + numLines > fw->rangeTopNum )
		    {
		    fw->rangeTopNum = 0;
		    }
		else
		    {
		    fw->rangeTopNum -= numLines;
		    }
		}
	    if( firstLine <= fw->rangeBotNum )
		{
		if( firstLine + numLines > fw->rangeBotNum )
		    {
		    fw->rangeBotNum = 0;
		    }
		else
		    {
		    fw->rangeBotNum -= numLines;
		    }
		}
	    }
	else
	    {   /* Adding lines */
	    if( fw->rangeTopNum >= firstLine )
		fw->rangeTopNum += numLines;
	    if( fw->rangeBotNum >= firstLine )
		fw->rangeBotNum += numLines;
	    }
	}
}

checkUpdate(long long firstLine, long long numLines, int delete)
{
windowStruct *cw, *w;
long long cnt;
int dirty=0;

    cw = globalData.activeWindow;
    for(w=globalData.windowList; w; w=w->next)
	{
	/* Do any of the alternate files need fixing up ? */
	checkUpdateAlternate(cw, w, firstLine, numLines, delete);
	/* Main window may need range fixup */
	if( w == cw )
	    {
	    if( delete )
		{   /* May need to fix up ranges */
		if( firstLine <= w->rangeTopNum )
		    {
		    if( firstLine + numLines > w->rangeTopNum )
			{
			w->rangeTopNum = 0;
			}
		    else
			{
			w->rangeTopNum -= numLines;
			}
		    }
		if( firstLine <= w->rangeBotNum )
		    {
		    if( firstLine + numLines > w->rangeBotNum )
			{
			w->rangeBotNum = 0;
			}
		    else
			{
			w->rangeBotNum -= numLines;
			}
		    }
		}
	    else
		{   /* Adding lines */
		if( w->rangeTopNum >= firstLine )
		    w->rangeTopNum += numLines;
		if( w->rangeBotNum >= firstLine )
		    w->rangeBotNum += numLines;
		}
	    }
	else if(w->fileInfo == cw->fileInfo )
	    {   /* May interact */
	    if( delete )
		{   /* Deleting, did we eat the top line, range etc */
		if( firstLine <= w->topOffset + 1 )
		    {
		    if( firstLine + numLines >= w->topOffset +1 )
			{  /* Delete includes top line, reset top line to start of delete */
			w->topOffset = firstLine-1;
			w->topLine = NULL;  /* Mark invalid */
			dirty = 1;
			}
		    else  /* Delete completely before our top, just subtract */
			w->topOffset -= numLines;
		    if( w->topOffset < 0 )
			w->topOffset = 0;
		    }
		if( firstLine <= w->rangeTopNum )
		    {
		    if( firstLine + numLines > w->rangeTopNum )
			{
			w->rangeTopNum = 0;
			}
		    else
			{
			w->rangeTopNum -= numLines;
			}
		    }
		if( firstLine <= w->rangeBotNum )
		    {
		    if( firstLine + numLines > w->rangeBotNum )
			{
			w->rangeBotNum = 0;
			}
		    else
			{
			w->rangeBotNum -= numLines;
			}
		    }
		}
	    else
		{   /* Adding lines */
		    /* Note topOffset is 0 based, ranges are 1 based */
		if( w->topOffset >= firstLine-1 )
		    w->topOffset += numLines;
		if( w->rangeTopNum >= firstLine )
		    w->rangeTopNum += numLines;
		if( w->rangeBotNum >= firstLine )
		    w->rangeBotNum += numLines;
		}
	    }
	}
    return(dirty);
}

/* They have specified a full command on the command line */
/* or just a number for a goto line command */
processCmd(char *execString)
{
char *cmdPtrs[20], *originalArgs;
int inArg, inQuote, numArgs, cmdIsInt=1, i, c, len;
cmdStruct *cmd;

    if( execString[0] == '\0' )
	{
	updateCursor(globalData.activeWindow);
	return(1);
	}
    inArg=numArgs=inQuote = 0;
    cmdPtrs[1] = "";
    len = strlen(execString);
    originalArgs = (char *) alloca(len);
    originalArgs[0] = '\0';
    for(i=0; i<len; i++)
	{
	c = execString[i];
	if( inQuote )
	    {
	    if( c == '"' )
		{
		inQuote = 0;
		i++;
	        execString[i] = '\0';
	        inArg = 0;
	        numArgs++;
	        continue;
		}
	    }
	if( inArg )
	    {
	    if( !isdigit(c) )
		cmdIsInt = 0;
	    if( isspace(c) )
		{
		execString[i] = '\0';
		inArg = 0;
		numArgs++;
		}
	    continue;
	    }
	if( !isspace(c) )
	    {
	    if( c == '"' )
		inQuote = 1;
	    cmdPtrs[numArgs] = &execString[i];
	    if( numArgs == 1 )
		strcpy(originalArgs, &execString[i]);
	    inArg = 1;
	    }
	}
    if( inArg )
	numArgs++;
    if( numArgs == 0 )
	return(1);
    if( numArgs == 1 && cmdIsInt )
	{
	char *argvx[3];

	argvx[0] = "goto";
	argvx[1] = cmdPtrs[0];
	argvx[2] = NULL;
	return(gotoCmd(2, argvx));
	}
    if(findCmd(cmdPtrs[0], &cmd, 0) > 0 )
	{
	if( globalData.markSet && cmd->flags & ILLEGAL_IN_MARK )
	   dingMsg("Can't do that with marks set");
	else if( cmd->flags & ORIGINAL_ARGS )
	    {
	    cmdPtrs[1] = originalArgs;
	    return((*cmd->func)(2, cmdPtrs));
	    }
	else
	    return((*cmd->func)(numArgs, cmdPtrs));
	}
    else
	{
	char tmp[30];

 	if( strlen(cmdPtrs[0]) > 15 )
	    cmdPtrs[0][15] = '\0';
	sprintf(tmp, "%s: not defined", cmdPtrs[0]);
	emsg(tmp);
	}
    return(0);
}

/* Goto cmd always expects just the one arg, skip the argc/argv option */
gotoCmd(int argc, char **argv)
{
char *arg;
long long lnum;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc != 2 || !(arg=argv[1]) )
	{
	emsg("Need a single argument to goto");
	updateCursor(w);
	return(1);
	}

    lnum = atoll(arg);
    return(gotoline(w, lnum));
}

/* "0" is the 1st line of the file, etc, to numLines-1 */
/* if current topLine is NULL, must use bottom or top only */
getLine(windowStruct *w, fileStruct *fs, long long lnum, lineStruct **lsp)
{
long long midline, midc, middiff, taildiff, i;
lineStruct *ls, *ns;
char tmp[20];

    if( w->topLine == NULL )
        {
        if( lnum >= fs->numLines )   /* Extending file */
            goto EXTEND;
        if( lnum < fs->numLines - lnum )
            goto HEAD;
        goto TAIL;
        }

    if( w->topOffset == lnum )
        {
        *lsp = w->topLine;
        return(1);
        }

    middiff = lnum - w->topOffset;
    if( middiff < 0 )
        midc = -middiff;
    else
        midc = middiff;
    if( midc <= lnum )   /* Cheaper to start at top offset */
        {
        if( lnum >= fs->numLines )   /* Extending file */
            {
EXTEND:
/*emsg("EXTEND");*/
            ls = fs->tail;
            for(i=fs->numLines; i<=lnum; i++)
                {
                ns = (lineStruct *) calloc(1, sizeof(lineStruct));
                ns->flags = 1;
                ls->next = ns;
                ns->prev = ls;
                ls = ns;
                fs->tail = ns;
                fs->numLines++;
                }
            }
        else if( fs->numLines - lnum < midc )
            {   /* Cheaper to start at EOF */
TAIL:
/*sprintf(tmp, "TAIL %lld", lnum);
emsg(tmp);*/
            ls = fs->tail;
            for(i=fs->numLines-1; i>lnum; i--)
                ls = ls->prev;
            }
        else if( middiff > 0 )
            {
/*emsg("mid-dn");*/
            ls = w->topLine;
            for(i=w->topOffset; i<lnum; i++)
                ls=ls->next;
            }
        else    /* Go backwards from top offset */
            {
/*emsg("mid-up");*/
            ls = w->topLine;
            for(i=w->topOffset; i>lnum; i--)
                ls = ls->prev;
            }
        }
    else        /* Cheaper to start in the beginning */
        {
HEAD:
/*emsg("HEAD");*/
        ls = fs->head;
        for(i=0; i<lnum; i++)
            ls = ls->next;
        }
    *lsp = ls;
    return(1);
}

/* get the pointer to the current line struct at the current cursor position */
/* If line does not exist, create */
getCursorLine(windowStruct *w, lineStruct **lsp)
{
fileStruct *fs;
lineStruct *ls, *ns;
int i;

    /* Do we have it already ? */
    if( (*lsp = w->cursorLine) )
	return(1);      /* Yes */

    for(ls=w->topLine, i=1; i<w->cursorLineNo; i++)
	{
	if( ls->next )
	    ls = ls->next;
	else
	    {
	    ns = (lineStruct *) calloc(1, sizeof(lineStruct));
	    ns->flags = 1;
	    ls->next = ns;
	    ns->prev = ls;
	    ls = ns;
	    fs = w->fileInfo;
	    fs->tail = ns;
	    fs->numLines++;
	    }
	}
    *lsp = ls;
    w->cursorLine = ls;     /* Also update so we can get it fast */
    return(1);
}

gotoline(windowStruct *w, long long lnum)
{
long long curLine, newTopLineOffset;
int i;
fileStruct *fs;
lineStruct *ls;

    if( (fs=w->fileInfo) == NULL )
	return(0);

    w->cursorLine = NULL;
    curLine = w->topOffset + w->cursorLineNo;
    if( lnum < 0 )
	{
	if( curLine + lnum < 1 )
	    {
	    w->topOffset = 0;
	    w->cursorLineNo = 1;
	    w->topLine = fs->head;
	    redrawWindow(w);
	    goto CHECKR;
	    }
	lnum = curLine + lnum;
	}
    else if( lnum == 0 )
	{
	updateCursor(w);
	return(1);
	}

    if( lnum == 1) /* Beginning of file */
	{
	w->topOffset = 0;
	w->cursorLineNo = 1;
	w->topLine = fs->head;
	redrawWindow(w);
	goto CHECKR;
	}

    if( lnum > w->topOffset && lnum < w->topOffset + w->height )
        {
        w->cursorLineNo = lnum - w->topOffset;
	updateStatus();
        updateCursor(w);
	goto CHECKR;
        }

    newTopLineOffset = lnum - (w->height -2)/2;
    if( newTopLineOffset < 0 )
	newTopLineOffset = 0;
    if( getLine(w, fs, newTopLineOffset, &ls) )
        {
        w->topLine = ls;
        w->topOffset = newTopLineOffset;
	w->cursorLineNo = lnum - newTopLineOffset;
        redrawWindow(w);
CHECKR:
	if( w->rangeTopNum &&
	    w->rangeTopNum > lnum )
	    dingMsg("Out of range");
	if( w->rangeBotNum &&
	    w->rangeBotNum < lnum )
	    dingMsg("Out of range");
        return(1);
        }
    return(0);
}

/* Cursor commands, move up, down, right left one or more spots */
/* The first set is the actual command, the 2n the I/F to the user */
cursorUp(int cnt)
{
int i;
long long lnum;
windowStruct *w=globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	return(0);

    w->cursorLine = NULL;
    if( w->rangeTopNum )
	{   /* Did we go out of bounds ? */
	if( w->rangeTopNum > w->topOffset + w->cursorLineNo - cnt )
	    {
	    dingMsg("Range limited");
	    return(1);
	    }
	}
    if( cnt == 0 )
	{
	if( w->rangeTopNum &&
		w->rangeTopNum > w->topOffset )
	    w->cursorLineNo = w->rangeTopNum - w->topOffset;
	else
	    w->cursorLineNo = 1;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    else if( cnt >= w->cursorLineNo )
	{
	i = w->cursorLineNo - 1;
	w->cursorLineNo -= i;
	cnt -= i;
	for(i=0; i<cnt && w->topOffset > 0; i++)
	    {
	    w->topLine = w->topLine->prev;
	    w->topOffset--;
	    }
	redrawWindow(w);
	}
    else
	{
	w->cursorLineNo -= cnt;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    return(1);
}

cursorDn(int cnt)
{
int i;
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns;

    if( !(fs=w->fileInfo) )
	return(0);

    w->cursorLine = NULL;
    if( w->rangeBotNum )
	{   /* Did we go out of bounds ? */
	if( w->rangeBotNum < w->topOffset + w->cursorLineNo + cnt )
	    {
	    dingMsg("Range limited");
	    return(1);
	    }
	}
    if( cnt == 0 )
	{
	if( w->rangeBotNum &&
		w->topOffset + w->height - 1 > w->rangeBotNum )
	    w->cursorLineNo = w->rangeBotNum - w->topOffset;
	else
	    w->cursorLineNo = w->height-1;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    else if( cnt >= w->height - w->cursorLineNo )
	{
	i = w->height - w->cursorLineNo - 1;
	w->cursorLineNo += i;
	cnt -= i;
	for(i=0, ls=w->topLine; i<cnt; i++)
	    {
	    w->topOffset++;
	    if( ls->next )
		ls = w->topLine = ls->next;
	    else
		{
		ns = (lineStruct *) calloc(1, sizeof(lineStruct));
		ns->flags = 1;
		ls->next = ns;
		ns->prev = ls;
		ls = w->topLine = ns;
		fs->tail = ns;
		fs->numLines++;
		}
	    }
	redrawWindow(w);
	}
    else
	{
	w->cursorLineNo += cnt;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    return(1);
}

cursorUpCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to cursor command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	{
	if( arg[0] )
	    cnt = atoi(arg);
	else
	    cnt = 0;
	}
    if( cnt >= 0 )
	cursorUp(cnt);
    else if( cnt < 0 )
	cursorDn(-cnt);
}
cursorDnCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to cursor command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	{
	if( arg[0] )
	    cnt = atoi(arg);
	else
	    cnt = 0;
	}
    if( cnt >= 0 )
	cursorDn(cnt);
    else if( cnt < 0 )
	cursorUp(-cnt);
}

/* Move at most cnt spaces leftward; Adjust leftOffset if needed */
cursorLeft(int cnt)
{
int i;
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns;

    if( !(fs=w->fileInfo) )
	return(0);

    if( cnt == 0 )
	{
	w->cursorX = 1;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    else if( cnt > w->cursorX - 1 )
	{
	if( w->leftOffset == 0 )
	    {
	    w->cursorX = 1;
	    updateStatus();
	    updateCursor(w);
	    }
	else
	    {
	    i = w->cursorX - 1;
	    w->cursorX -= i;
	    cnt -= i;
	    if( cnt > w->leftOffset )
	        w->leftOffset = 0;
	    else
	        w->leftOffset -= cnt;
	    redrawWindow(w);
	    }
	}
    else
	{
	w->cursorX -= cnt;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    return(1);
}

cursorRight(int cnt)
{
int i;
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns;
char *line;

    if( !(fs=w->fileInfo) )
	return(0);

    if( cnt == 0 )
	{
	getCursorLine(w, &ls);
        i = getChars(fs, ls, ls->numChars+1, &line);
        /* Backup to first non-blank */
        for( ; i>0; i--)
            if( line[i-1] != ' ')
                break;
        if( i < w->leftOffset + w->cursorX )
            w->cursorX = w->width - 1;
        else
            {
            if( i - w->leftOffset < w->width-1 )
                w->cursorX = 1 + i - w->leftOffset;
            else
                {
                w->leftOffset = i - w->width + 5;
                w->cursorX = 1 + i - w->leftOffset;
                redrawWindow(w);
                }
            }
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    else if( cnt >= w->width - w->cursorX )
	{
	i = w->width - w->cursorX - 1;
	w->cursorX += i;
	cnt -= i;
	w->leftOffset += cnt;
	redrawWindow(w);
	}
    else
	{
	w->cursorX += cnt;
	updateStatus();
	printf("\033[%d;%dH", w->y+w->cursorLineNo, w->x+w->cursorX);
	}
    return(1);
}

cursorLeftCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to cursor command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	{
	if( arg[0] )
	    cnt = atoi(arg);
	else
	    cnt = 0;
	}
    if( cnt >= 0 )
	cursorLeft(cnt);
    else if( cnt < 0 )
	cursorRight(-cnt);
}
cursorRightCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to cursor command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	{
	if( arg[0] )
	    cnt = atoi(arg);
	else
	    cnt = 0;
	}
    if( cnt >= 0 )
	cursorRight(cnt);
    else if( cnt < 0 )
	cursorLeft(-cnt);
}

tabRightCmd(int argc, char **argv)
{
char *arg;
int tcnt=1, cnt, pos;
windowStruct *w = globalData.activeWindow;

    if( argc > 2 )
	{
	emsg("At most one arg to tab command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	tcnt = atoi(arg);

    pos = w->leftOffset + w->cursorX - 1;
    if( tcnt > 0 )
	{
	cnt = tcnt*globalData.tabSpace - (pos%globalData.tabSpace);
	cursorRight(cnt);
	}
    else if( tcnt < 0 )
	{
	cnt = (pos%globalData.tabSpace);
	if( cnt == 0 )
	    cnt = 4;
	cnt += (-1-tcnt)*globalData.tabSpace;
	cursorLeft(cnt);
	}
}

tabLeftCmd(int argc, char **argv)
{
char *arg;
int tcnt=1, cnt, pos;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to tab command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	tcnt = atoi(arg);
    tcnt = -tcnt;
    pos = w->leftOffset + w->cursorX -1;
    if( tcnt > 0 )
	{
	cnt = tcnt*globalData.tabSpace - (pos%globalData.tabSpace);
	cursorRight(cnt);
	}
    else if( tcnt < 0 )
	{
	cnt = (pos%globalData.tabSpace);
	if( cnt == 0 )
	    cnt = 4;
	cnt += (-1-tcnt)*globalData.tabSpace;
	cursorLeft(cnt);
	}
}

returnCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w = globalData.activeWindow;

    if( argc > 2 )
	{
	emsg("At most one arg to goto command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    if( cnt <= 0 )
	return(0);
    cursorDn(cnt);
    cursorLeft(w->leftOffset+w->cursorX);
    return(1);
}

/* Move cnt lines up the file */
/* Move the cursor with it is unless we cannot go higher */
scrollUp(int cnt)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
int ii;
long long i, j;

    if( !(fs=w->fileInfo) )
	return(0);

    w->cursorLine = NULL;
    if( cnt == 0)
	{   /* Make this line the bottom line of the window */
	cnt = w->height-w->cursorLineNo-1;
	    /* unless we are limited by top of file */
	if( w->topOffset + w->cursorLineNo < w->height )
	    {   /* Yep we are limited to no more than moving w->topOffset */
	    i = w->topOffset + w->cursorLineNo;
	    w->topOffset = 0;
	    w->topLine = fs->head;
	    w->cursorLineNo = i;
	    redrawWindow(w);
	    return(1);
	    }
	}
    else if( w->rangeTopNum > w->topOffset + w->cursorLineNo - cnt )
	{
	if( w->rangeTopNum )
	    dingMsg("Range limited" );
	cnt = w->topOffset + w->cursorLineNo - w->rangeTopNum;
	if( cnt <= 0 )
	    return(1);
	}
    if(w->topOffset-cnt<0)
	{	/* Up to top **/
	i = cnt - w->topOffset;
	w->topOffset = 0;
	w->topLine = fs->head;
	j = w->cursorLineNo - i;
	if( j < 1 )
	    w->cursorLineNo = 1;
	else
	    w->cursorLineNo = j;
	}
    else
	{
	w->topOffset -= cnt;
	for(ii=0; ii<cnt; ii++)
	    w->topLine = w->topLine->prev;
	if( w->height > w->cursorLineNo + cnt )
	    w->cursorLineNo += cnt;
	}
    redrawWindow(w);
    return(1);
}

scrollDn(int cnt)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
int i;
lineStruct *ls, *ns;

    if( !(fs=w->fileInfo) )
	return(0);

    w->cursorLine = NULL;
    if( cnt == 0)
	{   /* Make this line the top line */
	cnt = w->cursorLineNo-1;
	}
    else if( w->rangeBotNum &&
	    w->rangeBotNum < w->topOffset + w->cursorLineNo + cnt )
	{
	cnt = w->rangeBotNum - (w->topOffset + w->cursorLineNo);
	if( cnt <= 0 )
	    {
	    dingMsg("Range limited");
	    return(1);
	    }
	}
    ls = w->topLine;
    for(i=0; i<cnt; i++)
	{
	if( ls->next )
	    ls = ls->next;
	else
	    {
	    ns = (lineStruct *) calloc(1, sizeof(lineStruct));
	    ns->flags = 1;
	    ns->prev = ls;
	    ls->next = ns;
	    fs->tail = ns;
	    ls = ns;
	    fs->numLines++;
	    }
	}
    w->topOffset += cnt;
    if( w->cursorLineNo > cnt )
	w->cursorLineNo -= cnt;
    w->topLine = ls;
    redrawWindow(w);
    return(1);
}

scrollUpCmd(int argc, char **argv)
{
char *arg;
windowStruct *w = globalData.activeWindow;
int cnt=0;

    if( argc > 2 )
	{
	emsg("At most one arg to scroll command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    else
	cnt = w->height/2;
    if( cnt >= 0)
	scrollUp(cnt);
    else if( cnt < 0 )
	scrollDn(-cnt);
}
scrollDnCmd(int argc, char **argv)
{
char *arg;
windowStruct *w = globalData.activeWindow;
int cnt=0;

    if( argc > 2 )
	{
	emsg("At most one arg to scroll command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    else
	cnt = w->height/2;
    if( cnt >= 0)
	scrollDn(cnt);
    else if( cnt < 0 )
	scrollUp(-cnt);
}

/* Move cnt whole pages upwards. A cnt of 0 implies goto to top of file */
pageUp(int cnt)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
long long lcnt, i, effTopLine;

    if( !(fs=w->fileInfo) )
	return(0);

    if( w->rangeTopNum )
	effTopLine = w->rangeTopNum;
    else
	effTopLine = 1;
    if( cnt == 0 )
	return(gotoline(w, effTopLine));

    w->cursorLine = NULL;
    lcnt = ((long long)cnt) * (w->height-1);
    if( w->topOffset + w->cursorLineNo - lcnt <= effTopLine )
	{
	if( w->topOffset == effTopLine )
	    {
	    w->cursorLineNo = 1;
	    updateStatus();
	    updateCursor(w);
	    }
	return(gotoline(w, effTopLine));
	}

    i = w->topOffset - lcnt;
    if( i <= w->cursorLineNo )
	return(gotoline(w, w->topOffset + w->cursorLineNo - lcnt));

    for(i=0; i<lcnt && w->topOffset>0; i++)
	{
	w->topOffset--;
	w->topLine = w->topLine->prev;
	}
    redrawWindow(w);
    return(1);
}

pageDn(int cnt)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns;
long long lcnt, numLines, topOffset, i, effBotLine;
int j;

    if( !(fs=w->fileInfo) )
	return(0);
    w->cursorLine = NULL;
    if( cnt == 0 )
	{	/* Goto EOF, harder than it seems b/c we may have added empties */
	if( w->rangeBotNum )
	    return(gotoline(w, w->rangeBotNum));
	ls = fs->tail;
	numLines = fs->numLines;
	/* Skip any fillers */
	while( (ls->flags & 1) && ls->prev )
	    {
	    numLines--;
	    ls = ls->prev;
	    }
	if( numLines < w->height )	/* Small file check, easier */
	    {
	    i= fs->numLines;
	    return(gotoline(w, i));
	    }
	/* Now skip back another 1/2 page, but we already have moved 1 more line */
	for(topOffset=numLines-1,j=2; j<w->height/2; j++)
	    {
	    ls=ls->prev;
	    topOffset--;
	    }
	w->topLine = ls;
	w->topOffset = topOffset;
	w->cursorLineNo = j;
	redrawWindow(w);
	return(1);
	}

    lcnt = ((long long)cnt) * (w->height-1);
    if( w->rangeBotNum )
	{
	if( w->rangeBotNum < w->topOffset + w->cursorLineNo + lcnt )
	    return(gotoline(w, w->rangeBotNum));
	}
    ls = w->topLine;
    for(i=0; i<lcnt; i++)
	{
	if( ls->next )
	    ls = ls->next;
	else
	    {
	    ns = (lineStruct *) calloc(sizeof(lineStruct), 1);
	    ns->flags = 1;
	    ns->prev = ls;
	    ls->next = ns;
	    ls = ns;
	    fs->tail = ls;
	    fs->numLines++;
	    }
	}
    w->topOffset+=lcnt;
    w->topLine = ls;
    redrawWindow(w);
    return(1);
}

pageUpCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to page command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    if( cnt >= 0 )
	pageUp(cnt);
    else if( cnt < 0 )
	pageDn(-cnt);
}
pageDnCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to page command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    if( cnt >= 0 )
	pageDn(cnt);
    else if( cnt < 0 )
	pageUp(-cnt);
}

pageLeft(int charCnt)
{
windowStruct *w = globalData.activeWindow;

    if( charCnt == 0 )
	w->leftOffset = 0;
    if( w->leftOffset < charCnt )
	w->leftOffset = 0;
    else
	w->leftOffset -= charCnt;
    redrawWindow(w);
    return(1);
}

pageRight(int charCnt)
{
windowStruct *w = globalData.activeWindow;

    w->leftOffset += charCnt;
    redrawWindow(w);
    return(1);
}

pageLeftCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to page command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    else
	cnt = 8;
    if( cnt >= 0 )
	pageLeft(cnt);
    else if( cnt < 0 )
	pageRight(-cnt);
}
pageRightCmd(int argc, char **argv)
{
char *arg;
int cnt=1;
windowStruct *w;

    w = globalData.activeWindow;
    if( argc > 2 )
	{
	emsg("At most one arg to page command");
	updateCursor(w);
	return(1);
	}
    if( argc == 2 && (arg=argv[1]) )
	cnt = atoi(arg);
    else
	cnt = 8;
    if( cnt > 0 )
	pageRight(cnt);
    else if( cnt < 0 )
	pageLeft(-cnt);
    else
	updateCursor(globalData.activeWindow);
}

statusCmd(int argc, char **argv)
{
windowStruct *w;
fileStruct *fs;
int i, j, k;

    /* Clear screen */
    printf("\033[2J\033[1;1H");
    for(w=globalData.windowList; w; w=w->next)
	{
	printf("Window %d: xy %d %d w/h %d %d\r\n", w->winNum,
                  w->x, w->y, w->width, w->height);
	printf("  left offset %d top offset %lld cursor offset %d down %d\n\r",
		w->leftOffset, w->topOffset, w->cursorX, w->cursorLineNo);
	if( w->rangeTopNum || w->rangeBotNum )
	    printf("  Range set from %lld to %lld\n",
	    w->rangeTopNum, w->rangeBotNum);
	if( (fs=w->fileInfo) )
	    {
	    for(j=0; j<NUM_ALT_FILES; j++)
	        {
		if( j == w->curParamSet )
		    {
		    fs = w->fileInfo;
		    printf("Main file(%d): %s\n\r", j, fs->fileName);
		    }
		else if( !(fs=w->editStack[j].fileInfo) )
		    continue;
		else
		    printf("Alternate file(%d): %s\n\r", j, fs->fileName);
		printf(" backed up? %d nosave? %d new? %d ro? %d modified? %d #lines %lld\n\r",
		    fs->backupWritten, fs->nosave, fs->created,
			fs->ro, fs->modified, fs->numLines);
		}
	    }
	}
    if( globalData.cutBuffer )
        {
        if( globalData.cutBuffer->mode == 0 )
            printf("Cut buffer has %d lines\n\r", globalData.cutBuffer->numLines);
        else
            printf("Cut buffer has %dx%d block\n\r", globalData.cutBuffer->numLines,
                     globalData.cutBuffer->width);
        }
    if( globalData.getBuffer )
        {
        if( globalData.getBuffer->mode == 0 )
            printf("Get buffer has %d lines\n\r", globalData.getBuffer->numLines);
        else
            printf("Get buffer has %dx%d block\n\r", globalData.getBuffer->numLines,
                     globalData.getBuffer->width);
        }
    /* Because now we are non-blocking, wait for a real char */
    while( getc(stdin) == EOF)
	;
    redrawScreen();
    return(1);
}

saveEditStack(windowStruct *w, int k)
{
    memcpy( &w->editStack[k], &w->topOffset, sizeof(fileWinParams));
    return(1);
}
/* With no args, switch to next file in window stack. With 1 arg, open file and
    push onto edit stack */
/* arg 2 is top offset, 3 is left offset 4 cursorY, 5, is cursorX */
editFileCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls;
long long lnum;
int j, k;

    w->cursorLine = NULL;
    if( argc == 1 )
	{       /* Switch files */
	k = w->curParamSet;
	for(j=(k+1)%NUM_ALT_FILES; j!=k; j = (j+1)%NUM_ALT_FILES)
	    if( w->editStack[j].fileInfo )
		break;
	if( j == k )
	    {
	    dingMsg("No alternate file");
	    updateCursor(w);
	    return(0);
	    }
	/* Copy the params to save and switch files */
	saveEditStack(w, k);
	/* Now, push the old file as current */
	w->curParamSet = j;
	w->leftOffset = w->editStack[j].leftOffset;
	if( w->editStack[j].cursorX < w->width-1 )
	    w->cursorX = w->editStack[j].cursorX;
	else
	    w->cursorX = w->width-1;
	if( w->editStack[j].cursorLineNo < w->height-1)
	    w->cursorLineNo = w->editStack[j].cursorLineNo;
	else
	    w->cursorLineNo = w->height - 1;
	fs = w->fileInfo = w->editStack[j].fileInfo;
	if( w->editStack[j].topOffset < fs->numLines )
	    w->topOffset = w->editStack[j].topOffset;
	else
	    w->topOffset = fs->numLines-4;
	w->fileInfo = w->editStack[j].fileInfo;
	w->rangeTopNum = w->editStack[j].rangeTopNum;
	w->rangeBotNum = w->editStack[j].rangeBotNum;
	w->topLine = NULL;  /* So getline knows it can't use topLine */
	/* Jump to top line */
	getLine(w, w->fileInfo, w->topOffset, &ls);
	w->topLine = ls;
	redrawWindow(w);
	return(1);
	}
    saveEditStack(w, w->curParamSet);
    if( openFile(argv[1], w) == 0 )
	return(0);
    fs = w->fileInfo;
    /* Update param set number */
    j = (w->curParamSet+1) % NUM_ALT_FILES;
    w->curParamSet = j;
    if( argc > 2 )
	{
	lnum = atoll(argv[2]);
	if( lnum >= fs->numLines-4 )
	    lnum = fs->numLines-4;
	if( lnum < 0 )
	    lnum = 0;
	getLine(w, fs, lnum, &ls);
	w->topOffset = lnum;
	w->topLine = ls;
	}
    if( argc > 3 )
	{
	j = atoi(argv[3]);
	if( j > 0 )
	    w->leftOffset = j;
	}
    if( argc > 4 )
	{
	j = atoi(argv[4]);
	if( j > 0 && j < w->height )
	    w->cursorLineNo = j;
	}
    if( argc > 5 )
	{
	j = atoi(argv[5]);
	if( j > 0 && j < w->width )
	    w->cursorX = j;
	}
    w->rangeTopNum = w->rangeBotNum = 0;
    redrawWindow(w);
    return(1);
}

/* Just unload the current file from the list of files for the window */
/* Takes no args */
uneditFileCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls;
long long lnum;
int j, k;

    if( argc != 1 )
	{
	emsg("-edit takes no args");
	updateCursor(w);
	return(0);
	}

    /* Make sure there is an alternate to switch to */
    k = w->curParamSet;
    for(j=(k+1)%NUM_ALT_FILES; j!=k; j = (j+1)%NUM_ALT_FILES)
	if( w->editStack[j].fileInfo )
	    break;
    if( j == k )
	{
	dingMsg("No alternate file");
	updateCursor(w);
	return(0);
	}
    w->cursorLine = NULL;
    /* Unload the file from the edit stack */
    w->editStack[k].fileInfo = NULL;

    /* Now, push the old file as current */
    w->curParamSet = j;
    w->leftOffset = w->editStack[j].leftOffset;
    if( w->editStack[j].cursorX < w->width-1 )
	w->cursorX = w->editStack[j].cursorX;
    else
	w->cursorX = w->width-1;
    if( w->editStack[j].cursorLineNo < w->height-1)
	w->cursorLineNo = w->editStack[j].cursorLineNo;
    else
	w->cursorLineNo = w->height - 1;
    fs = w->fileInfo = w->editStack[j].fileInfo;
    if( w->editStack[j].topOffset < fs->numLines )
	w->topOffset = w->editStack[j].topOffset;
    else
	w->topOffset = fs->numLines-4;
    w->fileInfo = w->editStack[j].fileInfo;
    w->rangeTopNum = w->editStack[j].rangeTopNum;
    w->rangeBotNum = w->editStack[j].rangeBotNum;
    w->topLine = NULL;  /* So getline knows it can't use topLine */
    /* Jump to top line */
    getLine(w, w->fileInfo, w->topOffset, &ls);
    w->topLine = ls;
    redrawWindow(w);
    return(1);
}

/* With no args, just split the window. With an arg, split and open new file */
addWindowCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls;
windowStruct *wnew;
int row, col;
char *fName = NULL;

    w->cursorLine = NULL;
    if( argc > 1 && argv[1] && argv[1][0] != '\0')
	{
	if( argv[1][0] == '"' && argv[1][1] == '"' )
	    ;	/* Special case of "" seen at startup */
	else
	    fName = argv[1];
	}
    if( argc > 3 )
	{   /* row col spec'ed */
	row = atoi(argv[2]);
	col = atoi(argv[3]);
	}
    else
	{
	row = w->cursorLineNo;
	col = w->cursorX;
	}
    fs = w->fileInfo;
    if( col == 1 )
	{
	if( row <= 1 || row >= w->height-1)
	    {
	    dingMsg("Can't put a window there");
	    updateCursor(w);
	    return(0);
	    }
	/* Add a horizontal window */
	wnew = (windowStruct *) malloc(sizeof(windowStruct));
	memcpy(wnew, w, sizeof(windowStruct));
	wnew->cutPosition = row;
	wnew->parent = w;
	wnew->splitVertical = 0;
	wnew->y = w->y + wnew->cutPosition;
	w->cursorLineNo = row-1;
	wnew->cursorLineNo = 1;
        /* Ensure the line exists */
        if( fs )
            {
            wnew->topOffset = w->topOffset + w->cursorLineNo+1;
            getLine(w, fs, wnew->topOffset, &ls);
            wnew->topLine = ls;
            }
	wnew->height = 1 + w->height - (1+wnew->cutPosition);
	w->height -= wnew->height;
	wnew->next = globalData.windowList;
	globalData.windowList = wnew;
	globalData.activeWindow = wnew;
	}
    else if( row != 1 )
	{
	dingMsg("Can't put a window there");
	updateCursor(w);
	return(0);
	}
    else
	{ /* Add a vertical window */
	if( col <= 1 || col >= w->width-1)
	    {
	    dingMsg("Can't put a window there");
	    updateCursor(w);
	    return(0);
	    }
	wnew = (windowStruct *) malloc(sizeof(windowStruct));
	memcpy(wnew, w, sizeof(windowStruct));
	wnew->cutPosition = col;
	wnew->parent = w;
	wnew->cursorX = 1;
	wnew->splitVertical = 1;
	wnew->x = w->x + wnew->cutPosition;
	w->cursorX = col - 1;
	wnew->width = 1 + w->width - (wnew->cutPosition+1);
	w->width -= wnew->width;
	wnew->next = globalData.windowList;
	globalData.windowList = wnew;
	globalData.activeWindow = wnew;
	}
    wnew->winNum = wnew->next->winNum+1;
    if( fName )
	{
	char *argvx[2];

	argvx[0] = "editFileCmd";
	argvx[1] = fName;
	editFileCmd(2, argvx);
	}
    redrawScreen();
    return(1);
}

/* Delete the last added window. No args */
delWindowCmd(int argc, char **argv)
{
windowStruct *w = globalData.windowList;
windowStruct *wp = w->parent;

    if( w->next == NULL )
	{
	dingMsg("Can't delete last window");
	updateCursor(w);
	return(0);
	}
    if( w->splitVertical )
	wp->width += w->width;
    else
        wp->height += w->height;
    if( globalData.activeWindow == w )
	globalData.activeWindow = wp;
    globalData.windowList = w->next;
    free(w);
    redrawScreen();
    return(1);
}

chgWindowCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
int wNum;

    /* With no args, just jump to the next window in line */
    if( argc < 2 )
	{
	if( w->next )
	    globalData.activeWindow = w->next;
	else
	    globalData.activeWindow = globalData.windowList;
	redrawScreen();
	updateCursor(globalData.activeWindow);
	return(1);
	}
    wNum = atoi(argv[1]);
    /* Change to this window number */
    for(w=globalData.windowList; w; w=w->next )
	if( w->winNum == wNum )
	    break;
    if( w )
	{
	globalData.activeWindow = w;
	redrawScreen();
	updateCursor(w);
	}
    return(0);
}

/* Line edit commands (addChar is in main.c) */
/* Delete the character under the cursor and move remaininf character left */
/* This is a destructive delete */
wipeCharCmd(int argc, char **argv)
{
int i, ix, ixx, cnt=1;
windowStruct *w = globalData.activeWindow;
fileStruct *fs = w->fileInfo;
lineStruct *ls;
char *line;

    if( fs == NULL )
	{
	dingMsg("No file open");
	return(0);
	}
    if( argc > 1  && argv[1][0] )
	cnt = atoi(argv[1]);
    if( cnt <= 0 )
	return(0);

    /* Get the correct line */
    if( !getCursorLine(w, &ls) )
	return(0);
    /* remove test for short lines. Because if tabs, we don't know how long */
    /* the line really is */
    if( ls->numCharsAvail == 0 )        /* Not in memory */
	unpackDiskLine(fs, ls);
    /* After line is unpacked, we know the real char count */
    if( ls->numChars < w->cursorX + w->leftOffset )
	return(0);      /* Nothing to do */

    if( !canModifyFile(fs) )
	return(0);

    fs->head->flags = fs->head->flags & ~1;
    ix = w->cursorX + w->leftOffset-1;
    ixx = ix + cnt;
    if( ixx >= ls->numChars )
	{
	ls->numChars = ix;
	ls->data.line[ix] = '\0';
	}
    else
	{
	memmove( &(ls->data.line[ix]),
		 &(ls->data.line[ixx]), 1+ls->numChars - ixx);
	ls->numChars -= cnt;
	}
    redrawLine(w, w->cursorLineNo, ls);
    updateCursor(w);
    return(1);
}

/* Delete the character before the cursor and move remaining character left */
/* if in insert mode, or if overwrite, just delete prev char and move */
/* This command does not accept a cnt, maybe it should? */
deleteCharCmd(int argc, char **argv)
{
int i, ix;
windowStruct *w = globalData.activeWindow;
fileStruct *fs = w->fileInfo;
lineStruct *ls;
char *line;

    if( fs == NULL )
	{
	dingMsg("No file open");
	return(0);
	}

    if( w->leftOffset + w->cursorX <= 1 )
	return(0);	/* Can't go any further left */
    if( !getCursorLine(w, &ls) )
	return(0);

    if( ls->numCharsAvail == 0 )	/* Not in memory */
	unpackDiskLine(fs, ls);
    if( ls->numChars+1 < w->cursorX + w->leftOffset )
	{
	return(cursorLeft(1));	/* Nothing to do but move */
	}

    if( !canModifyFile(fs) )
	return(0);
    fs->head->flags = fs->head->flags & ~1;
    ix = w->cursorX + w->leftOffset-1;

    if( globalData.insertMode )
	{
	memmove(&(ls->data.line[ix-1]), &ls->data.line[ix],
		1+ls->numChars-ix);
	ls->numChars--;
	}
    else
	ls->data.line[ix-1] = ' ';

    if( w->cursorX == 1)
	{	/* Need to redraw window from shift left 1 */
	w->leftOffset--;
	redrawWindow(w);
	}
    else
	{
	w->cursorX--;
        redrawLine(w, w->cursorLineNo, ls);
	}
    updateCursor(w);
    return(1);
}

/* If we are in mark, switch, if we have an arg to mark, cancel mark */
/* Mark is tightly integrated with cut, get, open commands below */
markCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
int dx, x, cx;
long long dy, y, cy;

    if( argc > 1 )   /* We ignore any value */
	globalData.markSet = 0;
    else if( globalData.markSet == 0 )
	{
	globalData.markSet = 1;
	globalData.markRefX = w->leftOffset + w->cursorX;
	globalData.markRefY = w->topOffset + w->cursorLineNo;
	}
   else
	{
	w->cursorLine = NULL;
        x = w->leftOffset + w->cursorX;
        y = w->topOffset + w->cursorLineNo;
        cx = globalData.markRefX;
        cy = globalData.markRefY;
        globalData.markRefX = x;
        globalData.markRefY = y;
        dx = cx - w->leftOffset;
        dy = cy - w->topOffset;
        if( dx > 0 && dx < w->width && dy > 0 && dy < w->height-1 )
            {
            w->cursorX = dx;
            w->cursorLineNo = dy;
            }
        else
            {
            if( dx <= 0 || dx >= w->width )
                {       /* Need to fix left offset */
                if( cx < w->width )
                    {   /* Set offset to 0 */
                    w->leftOffset = 0;
                    w->cursorX = cx;
                    }
                else /* Center it */
                    {   /* Try to offset by 8's */
                    w->leftOffset = cx - w->width/2;
                    w->cursorX = cx - w->leftOffset;
                    }
                }
            else        /* Don't adjust leftoffset */
                w->cursorX = dx;
            if( dy <= 0 || dy >= w->height )
                {       /* Need to goto line, it does a redraw */
                gotoline(w, cy);
                }
            else
                {
                w->cursorLineNo = dy;
                redrawWindow(w);
                }
            }
        }
    updateStatus();
    updateCursor(w);
    return(1);
}

/* Open up (insert) blank lines or a rectangular area if marks set */
openCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns, *head, *tail;
int i, cnt, startX, width, needRestoreCursor, dirty;
long long startLineNo, startCheckNo;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    if( !canModifyFile(fs) )
	return(0);

    needRestoreCursor = 0;
    if( globalData.markSet )
        {
        cnt = w->topOffset + w->cursorLineNo - globalData.markRefY;
        if( cnt < 0 )
            {
            startLineNo = w->topOffset + w->cursorLineNo;
            getLine(w, fs, startLineNo-1, &ls);
            cnt = 1 - cnt;
            }
        else
            {
            startLineNo = globalData.markRefY;
            getLine(w, fs, startLineNo-1, &ls);
            cnt = 1 + cnt;
            }
	startCheckNo = startLineNo;
        globalData.markSet = 0;
        needRestoreCursor = 1;
        clearArgDisplay();
        width = w->leftOffset + w->cursorX - globalData.markRefX;
        if( width == 0 )
            {   /* No dx */
            goto LINE_MODE;
            }
        if( width < 0 )
            {
            width = -width;
            startX = w->leftOffset + w->cursorX;
            }
        else
            {
            startX = globalData.markRefX;
            }
        /* insert width blanks starting at startX for cnt lines */
        /* May need to create and/or unpack the lines */
        startX--;   /* convert to 0 based indexing */
        for(i=0; i<cnt; i++, tail=ls, ls=ls->next)
            {
            if( !ls )
                {
                ls = (lineStruct *) calloc(sizeof(lineStruct), 1);
                ls->flags = 1;
                tail->next = ls;
                ls->prev = tail;
                fs->tail = ls;
                fs->numLines++;
                }
            if( ls->flags & 1 )
                {   /* Filler only line, create at least 80, more if needed */
                ls->numChars = startX + width;
                if( ls->numChars < 80 )
                    ls->numCharsAvail = 80;
                else
                    ls->numCharsAvail = ls->numChars + 20;
                ls->data.line = (char *) malloc(ls->numCharsAvail+1);

                memset(ls->data.line, ' ', ls->numChars);
                ls->data.line[ls->numChars] = '\0';
                }
            else
                {
                if( ls->numCharsAvail == 0 )
                    unpackDiskLine(fs, ls);
                if( ls->numChars < startX )
                    {   /* Really a do nothing as we don't add blanks with no reason at the EOL */
                    }
                else
                    {   /* Move the last chars of line past startX width to the right & blank fill */
                    if( ls->numCharsAvail < ls->numChars + width )
                        {
                        ls->numCharsAvail = ls->numChars + width + 20;
                        ls->data.line = (char *) realloc(ls->data.line,
                                        ls->numCharsAvail+1);
                        }
                    memmove(&(ls->data.line[startX+width]),
                            &(ls->data.line[startX]),
                            ls->numChars - startX);
                    memset(&(ls->data.line[startX]), ' ', width);
                    ls->numChars += width;
                    ls->data.line[ls->numChars] = '\0';
                    }
                }
            }
        restoreToMark(w);
	w->cursorLine = NULL;
        redrawWindow(w);
        return(0);
        }
    else
	startCheckNo = w->topOffset + w->cursorLineNo;

    if( argc > 1 && argv[1][0] )
	{
	cnt = atoi(argv[1]);
	if( cnt <= 0 )
	    {
	    emsg("Invalid number of lines to open");
	    updateCursor(w);
	    return(0);
	    }
	}
    else
	cnt = 1;

    for(i=1,ls=w->topLine; i<w->cursorLineNo; i++)
        {
        if( ls->next )
            ls = ls->next;
        else
            {
            ns = (lineStruct *) calloc(1, sizeof(lineStruct));
            ns->flags = 1;
            ls->next = ns;
            ns->prev = ls;
            ls = ns;
            fs->tail = ns;
            fs->numLines++;
            }
        }

LINE_MODE:
    dirty = checkUpdate(startCheckNo, (long long) cnt, 0);

    fs->head->flags = fs->head->flags & ~1;
    /* Create a new group of cnt lines */
    for(i=0; i<cnt; i++)
	{
	ns = (lineStruct *)calloc(1, sizeof(lineStruct));
	ns->numCharsAvail = 80;
	ns->data.line = (char *) malloc(81);
	ns->data.line[0] = '\0';
	if( i == 0 )
	    head = ns;
	else
	    {
	    ns->prev = tail;
	    tail->next = ns;
	    }
	tail = ns;
	}
    fs->numLines += cnt;

    /* Add if necessary to fill down to the current cursor pos */
    if( ls == w->topLine )
	{	/* The linked list starts at top line now */
	w->topLine = head;
	}
    if( ls == fs->head )	/* We are at top of file */
	fs->head = head;
    else
	{
	ls->prev->next = head;
	head->prev = ls->prev;
	}
    tail->next = ls;
    ls->prev = tail;
    w->cursorLine = NULL;   /* Reset current line */

    /* fs tail is never updated */
    globalData.markSet = 0;
    if( dirty )
	redrawScreen();
    else
	redrawWindow(w);
    return(1);
}

/* Frees the buffer if something in there, then converts lines to buffer */
/* In cut mode, stuff is freed, in get mode, stuff is not freed */
/* May need to pad if cnt is more than lines. Can happen with a EOF */
convertToLineBuffer(fileStruct *fs, lineStruct *head, int cnt,
                bufferStruct **bufPtrP, int freeLines)
{
bufferStruct *bufPtr;
lineStruct *ls, *xs;
int i;

    if( (bufPtr = *bufPtrP) )
        {       /* Must free old */
        for(i=0; i<bufPtr->numLines; i++)
            free(bufPtr->lines[i]);
        free(bufPtr);
        *bufPtrP = NULL;
        }
    *bufPtrP = bufPtr = (bufferStruct *) malloc(sizeof(bufferStruct) +
                        (cnt-1) * sizeof(char *));
    bufPtr->mode = 0;
    bufPtr->numLines = cnt;
    for(i=0, ls=head; i<cnt; i++)
        {
        if( ls )
            {
            if( ls->numCharsAvail == 0 )
                unpackDiskLine(fs, ls);
            if( freeLines )
                bufPtr->lines[i] = ls->data.line;
            else
                {
                bufPtr->lines[i] = (char *) malloc(ls->numChars+1);
                strcpy(bufPtr->lines[i], ls->data.line);
                }
            xs = ls;
            ls =ls->next;
            if( freeLines )
                free(xs);
            }
        else
            {
            bufPtr->lines[i] = (char *) malloc(1);
            bufPtr->lines[i][0] = '\0';
            }
        }
    return(1);
}

/* Frees the buffer if something in there, then converts lines to rect buffer */
/* Buffer begins in start col (1 index) and countains width chars */
/* May need to pad if cnt is more than lines. Can happen with a EOF */
convertToRectBuffer(fileStruct *fs, lineStruct *head, int cnt,
                int startX, int width, int createLines,
                bufferStruct **bufPtrP)
{
bufferStruct *bufPtr;
lineStruct *ls, *tail;
int i, col, j;

    if( (bufPtr = *bufPtrP) )
        {       /* Must free old */
        for(i=0; i<bufPtr->numLines; i++)
            free(bufPtr->lines[i]);
        free(bufPtr);
        *bufPtrP = NULL;
        }
    *bufPtrP = bufPtr = (bufferStruct *) malloc(sizeof(bufferStruct) +
                        (cnt-1) * sizeof(char *));
    bufPtr->mode = 1;
    bufPtr->numLines = cnt;
    bufPtr->width = width;
    for(i=0, ls=head; i<cnt; i++)
        {
        bufPtr->lines[i] = (char *) malloc(width+1);
        if( ls )
            {
            if( ls->numCharsAvail == 0 )
                unpackDiskLine(fs, ls);
            for(col=startX-1,j=0; j<width; col++, j++)
                {
                if( col < ls->numChars )
                    bufPtr->lines[i][j] = ls->data.line[col];
                else
                    bufPtr->lines[i][j] = ' ';
                }
            bufPtr->lines[i][j] = '\0';
            tail = ls;
            ls =ls->next;
            }
        else
            {
            memset(bufPtr->lines[i], ' ', width);
            bufPtr->lines[i][width] = '\0';
            if( createLines )
                {   /* For cut version, need the line to exist */
                ls = (lineStruct *) calloc(sizeof(lineStruct), 1);
                ls->numCharsAvail = 80;
                ls->data.line = (char *) malloc(81);
                ls->prev = tail;
                tail->next = ls;
                tail = fs->tail = ls;
                fs->numLines++;
                ls = NULL;
                }
            }
        }
    return(1);
}

/* Move the cursor back to the marked location */
/* Try to keep it on the same page */
restoreToMark(windowStruct *w)
{
int dx, x, cx;
long long dy, y, cy;

        cx = globalData.markRefX;
        cy = globalData.markRefY;
        dx = cx - w->leftOffset;
        dy = cy - w->topOffset;
        if( dx > 0 && dx < w->width && dy > 0 && dy < w->height-1 )
            {
            w->cursorX = dx;
            w->cursorLineNo = dy;
            }
        else
            {
            if( dx <= 0 || dx >= w->width )
                {       /* Need to fix left offset */
                if( cx < w->width )
                    {   /* Set offset to 0 */
                    w->leftOffset = 0;
                    w->cursorX = cx;
                    }
                else /* Center it */
                    {   /* Try to offset by 8's */
                    w->leftOffset = cx - w->width/2;
                    w->cursorX = cx - w->leftOffset;
                    }
                }
            else        /* Don't adjust leftoffset */
                w->cursorX = dx;
            if( dy <= 0 || dy >= w->height )
                {       /* Need to goto line, it does a redraw */
                gotoline(w, cy);
                }
            else
                {
                w->cursorLineNo = dy;
                }
            }
}


/* Cut line(s) or a rectangular area */
cutCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *head, *tail;
long long startLineNo, startCheckNo;
int i, cnt, dcnt, startX, width, dirty;
int needRestoreCursor=0;
char tmp[20];

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    if( !canModifyFile(fs) )
	return(0);

    w->cursorLine = NULL;
    if( globalData.markSet )
        {
        cnt = w->topOffset + w->cursorLineNo - globalData.markRefY;
        if( cnt < 0 )
            {
            startLineNo = w->topOffset + w->cursorLineNo;
            getLine(w, fs, startLineNo-1, &ls);
            cnt = 1 - cnt;
            }
        else
            {
            startLineNo = globalData.markRefY;
            getLine(w, fs, startLineNo-1, &ls);
            cnt = 1 + cnt;
            }
	startCheckNo = startLineNo;
        globalData.markSet = 0;
        needRestoreCursor = 1;
        clearArgDisplay();
        width = w->leftOffset + w->cursorX - globalData.markRefX;
        if( width == 0 )
            {   /* No dx */
            goto LINE_MODE;
            }
        if( width < 0 )
            {
            width = -width;
            startX = w->leftOffset + w->cursorX;
            }
        else
            {
            startX = globalData.markRefX;
            }
        /* Conversion also forces the lines to go into memory */
        convertToRectBuffer(w->fileInfo, ls, cnt, startX, width,
                        1, &(globalData.cutBuffer));
        /* Delete the characters in the region */
        startX--;   /* convert to 0 based indexing */
        for(i=0; i<cnt; i++, ls=ls->next)
            {
            if( startX+width >= ls->numChars )
                {   /* Easy we are trucating the end of the line */
		if( ls->numChars > startX )
		    {
		    ls->numChars = startX;
		    ls->data.line[startX] = '\0';
		    }
                }
            else
                {
                memmove( &(ls->data.line[startX]),
                         &(ls->data.line[startX+width]),
                         1 + ls->numChars -(startX+width));
                ls->numChars -= width;
                }
            }
        restoreToMark(w);
        redrawWindow(w);
        sprintf(tmp, "del/got %dx%d", cnt, width);
        emsg(tmp);
        return(0);
        }
    else
	startCheckNo = w->topOffset + w->cursorLineNo;
    if( argc > 1 )
	{
	if( argv[1][0] == '\0' )
	    {	/* uncut */
	    return(uncutCmd(1, NULL));
	    }
	cnt = atoi(argv[1]);
	if( cnt <= 0 )
	    {
	    emsg("Invalid number of lines to cut");
	    updateCursor(w);
	    return(0);
	    }
	}
    else
	cnt = 1;

    fs->head->flags = fs->head->flags & ~1;

    /* skip down to current cursor position */
    for(i=1, ls=w->topLine; i<w->cursorLineNo; i++)
        {
        if( ls->next == NULL )
            {   /* Nothing to delete really, already at EOF */
            head = NULL;
            dcnt = 0;
            goto BUFFER;
            }
        ls = ls->next;
        }
    startLineNo = -1;	/* B/c in cnt mode we don't need to adj line */
    /* Delete from line ls to ls + cnt-1 lines */
LINE_MODE:
    head = ls;
    for(i=0; i<cnt; i++)
	{
	tail = ls;
	if( ls->next )
	    ls = ls->next;
	else
	    break;
	}
    fs->numLines -= i;	/* May be less than cnt */
    dcnt = i;
    if( i != cnt )	/* We were short */
	{		/* We must update file tail */
	if( fs->head == head )
	    {		/* all lines are now deleted, put in a fake one */
	    fs->head = fs->tail = (lineStruct *) calloc(1, sizeof(lineStruct));
	    w->topLine = fs->head;
	    w->topLine->flags = 1;
	    fs->numLines = 1;
	    w->topOffset = 0;
	    w->cursorLineNo = 1;
	    goto BUFFER;
	    }
/* Ok, we could make the tail of the file safely be the line above the head */
/* but that causes a line shift if the start of the delete is the top line */
/* of the window, so add a fake line to the bottom of the file */
	ls = fs->tail = (lineStruct *) calloc(1, sizeof(lineStruct));
	ls->flags = 1;
	}
    if( w->topLine == head )
	w->topLine = ls;
    if( fs->head == head )
	{
	ls->prev = NULL;
	fs->head = ls;
	}
    else
	head->prev->next = ls;
    ls->prev = head->prev;
    /* Now convert lines to buffer format */
    tail->next = NULL;
BUFFER:
    dirty = checkUpdate(startCheckNo, (long long) cnt, 1);
    convertToLineBuffer(w->fileInfo, head, cnt, &(globalData.cutBuffer), 1);
    sprintf(tmp, "del %d got %d", dcnt, cnt);
    emsg(tmp);
    if( startLineNo >= 0 )
	{
	if( startLineNo > w->topOffset && startLineNo < w->topOffset + w->height )
	    w->cursorLineNo = startLineNo - w->topOffset;
	else
	    {   /* goto will do a redraw since we are out of cur window */
            gotoline(w, startLineNo);
            emsg(tmp);
	    return(1);
            }
	}
    if( dirty )
	redrawScreen();
    else
	redrawWindow(w);
    return(1);
}

/* A side effect of a get is that lines are put in memory instead of
  pointing to disk offsets */
getCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *head, *tail;
int i, cnt, startX, width;
int needRestoreCursor;
char tmp[20];

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    needRestoreCursor = 0;
    if( globalData.markSet )
        {
	w->cursorLine = NULL;
        cnt = w->topOffset + w->cursorLineNo - globalData.markRefY;
        if( cnt < 0 )
            {
            getLine(w, fs, w->topOffset + w->cursorLineNo-1, &head);
            cnt = 1 - cnt;
            }
        else
            {
            getLine(w, fs, globalData.markRefY-1, &head);
            cnt = 1 + cnt;
            }
        globalData.markSet = 0;
        needRestoreCursor = 1;
        clearArgDisplay();
        width = w->leftOffset + w->cursorX - globalData.markRefX;
        if( width == 0 )
            {   /* No dx */
            goto BUFFER;
	    }
        /* Rectangular get */
        if( width < 0 )
            {
            width = -width;
            startX = w->leftOffset + w->cursorX;
            }
        else
            {
            startX = globalData.markRefX;
            }
        convertToRectBuffer(w->fileInfo, head, cnt, startX, width,
                        0, &(globalData.getBuffer));
        sprintf(tmp, "Got %dx%d rect", cnt, width);
        emsg(tmp);
        /* Restore cursor to original mark location */
        restoreToMark(w);
        updateCursor(w);
        return(1);
        }

    if( argc > 1 )
	{
	if( argv[1][0] == '\0' )
	    {	/* put */
	    return(putCmd(1, NULL));
	    }
	cnt = atoi(argv[1]);
	if( cnt <= 0 )
	    {
            emsg("Invalid number of lines to get");
	    updateCursor(w);
	    return(0);
	    }
	}
    else
	cnt = 1;

    /* skip down to current cursor position */
    for(i=1, head=ls=w->topLine; i<w->cursorLineNo; i++)
	{
	if( ls->next == NULL )
	    {	/* At EOF, just update buffer */
	    head = NULL;
	    goto BUFFER;
	    }
	ls = ls->next;
	}
    head = ls;
    /* get from line ls to ls + cnt-1 lines */
BUFFER:
    convertToLineBuffer(w->fileInfo, head, cnt, &(globalData.getBuffer), 0);
    sprintf(tmp, "Got %d lines", cnt);
    emsg(tmp);
    if( needRestoreCursor )
        restoreToMark(w);
    updateCursor(w);
    return(1);
}

uncutCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *head, *tail;
int i, cnt;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    if( globalData.cutBuffer == NULL || globalData.cutBuffer->numLines == 0)
	{
	dingMsg("Cut buffer is empty");
	updateCursor(w);
	return(0);
	}
    w->cursorLine = NULL;
    if( globalData.cutBuffer->mode == 1 )
        return(bufferRectPut(w, fs, globalData.cutBuffer));
    else
        return(bufferPut(w, fs, globalData.cutBuffer));
}

putCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    if( globalData.getBuffer == NULL || globalData.getBuffer->numLines == 0)
	{
	dingMsg("Get buffer is empty");
	updateCursor(w);
	return(0);
	}
    if( globalData.getBuffer->mode == 1 )
        return(bufferRectPut(w, fs, globalData.getBuffer));
    else
        return(bufferPut(w, fs, globalData.getBuffer));
}

/* Put the specified buffer into the current window */
bufferPut(windowStruct *w, fileStruct *fs, bufferStruct *buffer)
{
lineStruct *ls, *ns, *head, *tail;
int i, cnt, len, alen, dirty;

    if( !canModifyFile(fs) )
	return(0);
    dirty = checkUpdate(w->topOffset + w->cursorLineNo,
	     (long long)buffer->numLines , 0);
    fs->head->flags = fs->head->flags & ~1;
    /* Create a new group of lines */
    for(i=0; i<buffer->numLines; i++)
	{
	ns = (lineStruct *)calloc(1, sizeof(lineStruct));
	len = strlen(buffer->lines[i]);
	if( len < 80 )
	    alen = 80;
	else
	    alen = len + 20;
	ns->numCharsAvail = alen;
	ns->numChars = len;
	ns->data.line = (char *) malloc(alen+1);
	strcpy(ns->data.line, buffer->lines[i]);
	if( i == 0 )
	    head = ns;
	else
	    {
	    ns->prev = tail;
	    tail->next = ns;
	    }
	tail = ns;
	}
    fs->numLines += buffer->numLines;

    /* Add if necessary to fill down to the current cursor pos */
    for(i=1,ls=w->topLine; i<w->cursorLineNo; i++)
	{
	if( ls->next )
	    ls = ls->next;
	else
	    {	/* These are real unallocated lines */
	    ns = (lineStruct *) calloc(1, sizeof(lineStruct));
	    ls->next = ns;
	    ns->prev = ls;
	    ls = ns;
	    fs->tail = ns;
	    fs->numLines++;
	    }
	}
    if( ls == w->topLine )
	{	/* The linked list starts at top line now */
	w->topLine = head;
	}
    if( ls == fs->head )	/* We are at top of file */
	fs->head = head;
    else
	{
	ls->prev->next = head;
	head->prev = ls->prev;
	}
    tail->next = ls;
    ls->prev = tail;

    /* fs tail is never updated */
    if( dirty )
	redrawScreen();
    else
	redrawWindow(w);
    return(1);
}

/* Put the specified rectangular buffer into the current window */
/* May need to create lines if buffer goes past EOF */
bufferRectPut(windowStruct *w, fileStruct *fs, bufferStruct *buffer)
{
lineStruct *ls, *ns, *tail;
int i, len, allocLen, offsetX, wReq, width;
long long curLineNo;

    if( !canModifyFile(fs) )
	return(0);

    fs->head->flags = fs->head->flags & ~1;
    curLineNo = w->topOffset + w->cursorLineNo;
    getLine(w, fs, curLineNo-1, &ls);
    offsetX = w->leftOffset + w->cursorX - 1;
    width = buffer->width;
    wReq = offsetX + width;
    for(i=0; i<buffer->numLines; i++, tail=ls, ls=ls->next)
        {
        if( !ls )   /* Need to create a line */
            {
            ls = (lineStruct *) calloc(1, sizeof(lineStruct));
            fs->numLines++;
            fs->tail = ls;
            ls->prev = tail;
            tail->next = ls;
            ls->flags = 1;
            }

        if( ls->flags & 1 )
            {   /* But it is a filler, easy allocate enough space for the buffer */
            if( (allocLen=wReq) < 80 )
                allocLen = 80;
            ls->numCharsAvail = allocLen;
            ls->data.line = (char *) malloc(allocLen+1);
            ls->flags = ls->flags & ~1;
            if( offsetX )
                memset(ls->data.line, ' ', offsetX);
            /* Pick up trailing NULL */
            memcpy(&ls->data.line[offsetX], buffer->lines[i], 1+width);
            ls->numChars = offsetX + width;
            }
        else
            {
            if( ls->numCharsAvail == 0 )
                {   /* On disk still, pull it in */
                unpackDiskLine(fs, ls);
                }
            /* We are always inserting, so we need at laeast width more chars */
            if( ls->numChars + width > wReq )
                allocLen = ls->numChars + width;
            else
                allocLen = wReq;
            if( allocLen > ls->numCharsAvail )
                {
                if( allocLen < 80 )
                    allocLen = 80;
                else
                    allocLen += 20;
                ls->data.line = (char *) realloc(ls->data.line, allocLen+1);
                ls->numCharsAvail = allocLen;
                }
            if( offsetX >= ls->numChars )
                {   /* We are adding to the right */
                if( offsetX > ls->numChars )
                    memset(&(ls->data.line[ls->numChars]),
                        ' ', offsetX - ls->numChars);
                ls->numChars = offsetX;
                }
            else
                memmove(&(ls->data.line[offsetX+width]),
                        &(ls->data.line[offsetX]),
                        ls->numChars - offsetX);
            memcpy(&(ls->data.line[offsetX]),
                    buffer->lines[i], width);
            ls->numChars += width;
            ls->data.line[ls->numChars] = '\0';
            }
if( ls->numChars > ls->numCharsAvail )
emsg("OVERRUN");
        }
    redrawWindow(w);
    return(1);
}

checkCmd()
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *prev;
int cnt=0;

    if( !(fs=w->fileInfo) )
	return(0);

    if( fs->head->prev )
	printf("Head ptr has a prev!\n\r");
    if( fs->tail->next )
	printf("Tail ptr has a next!\n\r");
    for(ls=fs->head; ls; ls=ls->next, cnt++)
	{
	if( cnt != 0 )
	    if( ls->prev != prev )
		printf("At line %d, line ptr prev invalid is %p\n\r",
			cnt, ls->prev);
	prev = ls;
	}
    if( cnt != fs->numLines )
	printf("\n\r line count wrong; expected %lld got %d\n\r",
		fs->numLines, cnt);
    return(0);
}

/* Split the line. The end of the current line is just before the cursor */
splitCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns, *tail;
int len, dirty;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}

    if( !canModifyFile(fs) )
	return(0);

    /* Need this minor tweak so that a split of the top of range keeps */
    /* The top of range at beginning of split */
    dirty = checkUpdate(w->topOffset + w->cursorLineNo +1,
		(long long)1, 0);
    getCursorLine(w, &ls);
    if( ls->numCharsAvail == 0 )        /* Not in memory */
	unpackDiskLine(fs, ls);
    /* Allocate a new line */
    ns = (lineStruct *)calloc(sizeof(lineStruct), 1);
    tail = ns->next = ls->next;
    ls->next = ns;
    ns->prev = ls;
    if( tail )
	tail->prev = ns;
    else
	fs->tail = ns;
    fs->numLines++;

    /* And cut up the line */
    len = strlen(ls->data.line) - (w->leftOffset + w->cursorX);
    if( len < 80 )
	ns->numCharsAvail = 80;
    else
	ns->numCharsAvail = len + 20;
    ns->data.line = (char *) malloc(ns->numCharsAvail+1);
    strcpy(ns->data.line, &(ls->data.line[w->leftOffset+w->cursorX-1]));
    ns->numChars = strlen(ns->data.line);
    ls->data.line[w->leftOffset+w->cursorX-1] = '\0';
    ls->numChars = strlen(ls->data.line);
    redrawWindow(w);
    return(1);
}

joinCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;
lineStruct *ls, *ns, *ps;
int len, dirty;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    if( !canModifyFile(fs) )
	return(0);

    dirty = checkUpdate(w->topOffset + w->cursorLineNo +1,
		(long long)1, 1);
    getCursorLine(w, &ls);
    getLine(w, fs, w->topOffset + w->cursorLineNo, &ns);
    if( ls->flags & 1 || ns->flags & 1 || ls->numChars == 0 || ns->numChars == 0 )
	{
	if( ns->flags & 1 || ns->numChars == 0 )     /* keep ls */
	    {
	    if( ns->numCharsAvail )
		free(ns->data.line);
	    ls->next = ns->next;
	    if( ns->next )
		ns->next->prev = ls;
	    else
		fs->tail = ls;
	    free(ns);
	    }
	else
	    {
	    if( ls->numCharsAvail )
		free(ls->data.line);
	    ns->prev = ls->prev;
	    if( ls->prev )
		ls->prev->next = ns;
	    else
		fs->head = ns;
	    /* Do we need to update topLine ? */
	    if( w->topLine == ls )
		w->topLine = ns;    /* But the line no did not change, since we deleted current */
	    if( w->cursorLine == ls )   /* And what about cursorLine */
		w->cursorLine = ns;
	    free(ls);
	    }
	}
    else
	{   /* both have at least 1 char so we must join */
	if( ls->numCharsAvail ==0 )
	    unpackDiskLine(fs, ls);
	if( ns->numCharsAvail ==0 )
	    unpackDiskLine(fs, ns);
	len = ls->numChars + ns->numChars;
	if( len > ls->numCharsAvail )
	    {
	    ls->numCharsAvail = len + 20;
	    ls->data.line = (char *) realloc(ls->data.line,
				    ls->numCharsAvail+1);
	    }
	strcpy( &(ls->data.line[ls->numChars]), ns->data.line);
	ls->numChars += ns->numChars;
	ls->next = ns->next;
	if( ns->next )
	    ns->next->prev = ls;
	else
	    fs->tail = ls;
	free(ns->data.line);
	free(ns);
	}
    fs->numLines--;
    if( dirty )
	redrawScreen();
    else
	redrawWindow(w);
    return(1);
}

/* Disable range command */
mrangeCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    w->rangeBotNum = w->rangeTopNum = 0;
    redrawScreen();
    return(1);
}

rangeCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	{
	dingMsg("No file open;Use ^space edit <file> to open");
	return(0);
	}
    if( !globalData.markSet )
	{
	dingMsg("Mark area before command");
	return(0);
	}

    if( globalData.markRefY < w->topOffset + w->cursorLineNo )
	{
	w->rangeTopNum = globalData.markRefY;
	w->rangeBotNum = w->topOffset + w->cursorLineNo;
	}
    else
	{
	w->rangeBotNum = globalData.markRefY;
	w->rangeTopNum = w->topOffset + w->cursorLineNo;
	}
    globalData.markSet = 0;
    redrawScreen();
    emsg("Range set");
    return(1);
}
