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
#include <ctype.h>
#include <alloca.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "edit.h"

globalStruct globalData;

extern int startArgCmd();
extern int redrawCmd();

extern int cursorLeftCmd();
extern int cursorRightCmd();
extern int cursorUpCmd();
extern int cursorDnCmd();
extern int tabRightCmd();
extern int tabLeftCmd();
extern int returnCmd();

extern int pageUpCmd();
extern int pageDnCmd();
extern int scrollUpCmd();
extern int scrollDnCmd();
extern int pageLeftCmd();
extern int pageRightCmd();
extern int gotoCmd();
extern int rangeCmd();
extern int mrangeCmd();

extern int searchUpCmd();
extern int searchDnCmd();
extern int matchCmd();
extern int replaceCmd();

extern int wipeCharCmd();
extern int deleteCharCmd();

extern int splitCmd();
extern int joinCmd();

extern int markCmd();
extern int openCmd();	/* Open up line(s), or rectangular area */
extern int cutCmd();	/* Cut line(s), or rectangular area */
extern int getCmd();	/* Copy line(s), or rectangular area */
extern int uncutCmd();	/* Uncut line(s), or rectangular area */
extern int putCmd();	/* Put gotten line(s), or rectangular area */

extern int editFileCmd();
extern int uneditFileCmd(); /* Unload the file from the window edit list */
extern int addWindowCmd();
extern int chgWindowCmd();
extern int delWindowCmd();

extern int statusCmd();
extern int saveCmd();
extern int exitCmd();

extern int tabsCmd();
extern int updateCmd();
extern int noupdateCmd();
extern int insertModeCmd();
extern int defineKeyCmd();
extern int tagCmd();
extern int setCmd();

extern int checkCmd();
cmdStruct globalCmd[] = {
    {"arg", startArgCmd, 0},
    {"redraw", redrawCmd, 0},
    {"cursorLeft", cursorLeftCmd, 0},
    {"cursorRight", cursorRightCmd, 0},
    {"cursorUp", cursorUpCmd, 0},
    {"cursorDn", cursorDnCmd, 0},
    {"tabRight", tabRightCmd, IGNORE_IN_ARG},
    {"tabLeft", tabLeftCmd, IGNORE_IN_ARG},
    {"return", returnCmd, 0},

    {"scrollUp", scrollUpCmd, 0},	/* The scroll cmds do 1/2 height */
    {"scrollDn", scrollDnCmd, 0},	/* if arg specified, that number */
    {"pageUp", pageUpCmd, 0},
    {"pageDn", pageDnCmd, 0},
    {"pageLeft", pageLeftCmd, 0},	/* An 8 char shift by default */
    {"pageRight", pageRightCmd, 0},
    {"goto",     gotoCmd, 0},
    {"range",     rangeCmd, 0},
    {"-range",     mrangeCmd, 0},

    {"searchUp", searchUpCmd, 0},
    {"searchDn", searchDnCmd, 0},
    {"match", matchCmd, 0},
    {"replace", replaceCmd, ORIGINAL_ARGS},

    {"join", joinCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"split", splitCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},

    {"wipeChar", wipeCharCmd, IGNORE_IN_ARG},
    {"deleteChar", deleteCharCmd, IGNORE_IN_ARG},

    {"mark", markCmd, 0},	/* An empty arg is used to cancel the mark */
    {"open", openCmd, 0},	/* An empty arg is one line */
    {"cut", cutCmd, 0},		/* An empty arg is an uncut */
    {"get", getCmd, 0},		/* An empty arg is a put */
    /* The paste commands. 2, one for cut, one for get buffer */
    {"uncut", uncutCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"put", putCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},

    {"edit", editFileCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"-edit", uneditFileCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"e", editFileCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},	/* Duplicate shorthand */
    {"win", addWindowCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"-win", delWindowCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"cwin", chgWindowCmd, ILLEGAL_IN_MARK},
    {"status", statusCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},
    {"save", saveCmd, IGNORE_IN_ARG},
    {"exit", exitCmd, ILLEGAL_IN_MARK|IGNORE_IN_ARG},

    {"tabs", tabsCmd, IGNORE_IN_ARG},
    {"-update", noupdateCmd, IGNORE_IN_ARG},
    {"update", updateCmd, IGNORE_IN_ARG},
    {"insertMode", insertModeCmd, 0},
    {"defineKey", defineKeyCmd, 0},
    {"tag", tagCmd, ILLEGAL_IN_MARK},
    {"check", checkCmd, 0},
    {"set", setCmd, IGNORE_IN_ARG},
    {NULL, NULL}};

findCmd(char *cmdName, cmdStruct **cmdPtr, int startup)
{
int i, best=-1, l1;

    l1 = strlen(cmdName);
    for(i=0; globalCmd[i].funcName; i++)
	{
	if( strncmp(cmdName, globalCmd[i].funcName, l1) == 0 )
	    {	/* Check exact match */
	    if( strlen(globalCmd[i].funcName) == l1)
		{
		*cmdPtr = &globalCmd[i];
    		return(i);
		}
		
	    if( best == -1 )
		best = i;
	    else if(startup == 0)
		{
		emsg("ambiguous command");
		return(-1);
		}
	    }
	}
    if( best != -1 )
	*cmdPtr = &globalCmd[best];
    return(best);
}
		
main(int argc, char *argv[])
{
char *startFile[NUM_ALT_FILES];
char escapeString[12];
multiCharKeyDef *mk;
cmdStruct *cptr;
ucmdStruct *ucptr;
char *emptyArgSet[2], *oneArgSet[3];
int i, j, c;

    oneArgSet[2] = emptyArgSet[1] = NULL;
    globalData.tabSpace = 4;
    globalData.ringBellOnErr = 1;
    memset(startFile, 0, sizeof(char *) * NUM_ALT_FILES );
    for(j=0,i=1; i<argc; i++)
	{
	if( argv[i][0] == '-' && argv[i][1] == '-' )
	    continue;
	else if( j < NUM_ALT_FILES )
	    startFile[j++] = argv[i];
	}

    setupCmdMap();
    readRC();

    setupTerm(); 
    createInitialWindow(1, 1, globalData.winWidth-1, globalData.winHeight-2);
    globalData.mode = 1;
    
    if( startFile[0] )
	{
	openFile(startFile[0], globalData.activeWindow);
	for(i=1; startFile[i] && i<NUM_ALT_FILES; i++)
	    {
	    saveEditStack(globalData.activeWindow, globalData.activeWindow->curParamSet);
	    globalData.activeWindow->curParamSet = i;
	    openFile(startFile[i], globalData.activeWindow);
	    }
	}
    else
        readStartup();

    redrawScreen();
    while( 1)
        {
	c = getc(stdin);
	if( c == -1)
	    continue;
        cptr = NULL;
        if( c == '\033' )
            {       /* Escape sequence; next char should be a [ */
                    /* followed by either a single non-digit or */
                    /* one or more digits terminated by a ~ */
            if( (c=getc(stdin)) == EOF )
		{
BAD_ESC:
		dingMsg("Illegal char escape");
                continue;	/* Data should be ready immediately */
		}

            j = 0;
            escapeString[j++] = c;
            if( (c=getc(stdin)) == EOF )
                goto BAD_ESC;
            escapeString[j++] = c;
            if( isdigit(c) )
                { /* Collect to trailing ~ */
		while( j<NUM_ALT_FILES && (c=getc(stdin)) != EOF )
                    {
                    escapeString[j++] = c;
                    if( c == '~' )
                        break;
                    }
                }
            escapeString[j] = '\0';
            for(mk=globalData.multiList; mk; mk=mk->next)
                if( strcmp(mk->charSeq, escapeString) == 0)
                    break;
            if( !mk )
                {
		char tmpe[512];
		sprintf(tmpe, "Unknown escape %s", escapeString);
		dingMsg(tmpe);
		continue;
		}
            cptr = mk->cmd;
            c = 256; /* Make it out there */
            }
           
        if( globalData.argMode )
            {
            if( c == '\r' )
                {
                globalData.argBuffer[globalData.argPtr] = '\0';
                clearArgDisplay();
                processCmd(globalData.argBuffer);
                globalData.argPtr = 0;
                globalData.argMode = 0;
                continue;
                }
            else if( c == 127 )     /* Backspace 1 */
                {
                if( globalData.argPtr )
                    {
                    globalData.argPtr--;
                    printf("\033[D \033[D");
                    }
                continue;
                }
            else if( cptr || (cptr=globalData.keyMap[c]) )
                {   /* It should be an arg to the keymap's func */
                if( cptr->flags & IGNORE_IN_ARG )
                    continue;       /* Quietly ignore */
                globalData.argBuffer[globalData.argPtr] = '\0';
                clearArgDisplay();
		oneArgSet[0] = cptr->funcName;
		oneArgSet[1] = globalData.argBuffer;
		(cptr->func)(2, oneArgSet);
                globalData.argPtr = 0;
                globalData.argMode = 0;
                continue;
                }
            else if( globalData.argPtr +1 >= globalData.argBufferSize )
                {
                globalData.argBuffer = realloc(globalData.argBuffer, 
                            globalData.argBufferSize + 50);
                globalData.argBufferSize += 50;
                }
            globalData.argBuffer[globalData.argPtr++] = c;
            putchar(c);
            }
        else if( cptr || (cptr=globalData.keyMap[c]) )
            {
            if( globalData.markSet && cptr->flags & ILLEGAL_IN_MARK )
                {
		dingMsg("Can't do that with marks set");
                updateCursor(globalData.activeWindow);
                }
	    else if( cptr->funcName == NULL )
		{   /* pre-packaged */
		ucptr = (ucmdStruct *)cptr;
		(*ucptr->func)(ucptr->argc, ucptr->argv);
		continue;
		}
            else
		{
		emptyArgSet[0] = cptr->funcName;
		(*cptr->func)(1, emptyArgSet);
		}
            }
        else if( isprint(c) )
            addChar(c);
        else
            {
            char tmpe[20];
            sprintf(tmpe, "key %d undefined", c);
	    dingMsg(tmpe);
            }
        }
    exit(0);
}

/* Simple create for 1st window which is mandatory */
createInitialWindow(int x, int y, int width, int height)
{
windowStruct *w;

    w = (windowStruct *) calloc(1, sizeof(windowStruct));
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->next = globalData.windowList;
    globalData.windowList = w;
    globalData.activeWindow = w;
    return(1);
}

/* Add a character at the current cursor position. May need to */
/* Convert the line from disk to memory or extend it. Also, must pay */
/* attention if insert mode is on or off */
addChar(int c)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs = w->fileInfo;
lineStruct *ls, *ns;
char *line;
int i, cnt, lc, edge, col;

    if( fs == NULL )
	{
	emsg("No file open");
	return(0);
	}

    /* Get correct line */
    if( !getCursorLine(w, &ls) )
	return(0);
    if( !canModifyFile(fs) )
	return(0);
    fs->head->flags = fs->head->flags & ~1;
    if( ls->numCharsAvail == 0)	/* Not In memory */
	unpackDiskLine(fs, ls);

    if( ls->numChars < w->leftOffset + w->cursorX )
	{	/* We are end of the line */
	if( isspace(c) )
	    return(cursorRight(1));	/* Do nothing but move the cursor */
	/* We are right, so insert mode makes no diff */
	/* But we could need ALOT more space */
	if( ls->numCharsAvail < w->leftOffset + w->cursorX )
	    {
	    cnt = w->leftOffset + w->cursorX + 40;
	    ls->data.line = (char *) realloc(ls->data.line,
			cnt+1);
	    ls->numCharsAvail = cnt;
	    }
	for(i=ls->numChars; i<w->leftOffset + w->cursorX-1; i++)
	    ls->data.line[i] = ' ';
	ls->data.line[i] = c;
	ls->data.line[i+1] = '\0';
	ls->numChars = i+1;
        if( w->cursorX + 1 >= w->width )
	    return(cursorRight(1));
	w->cursorX++;
	putchar(c);
	return(1);
	}

    if( globalData.insertMode )
	{	/* Adds a char at this position */
	if( ls->numCharsAvail < ls->numChars+1 )
	    {       /* Need to reallocate */
	    ls->data.line = (char *)realloc(ls->data.line,
			    ls->numCharsAvail+41);
	    ls->numCharsAvail += 40;
	    }
	i=w->leftOffset+w->cursorX-1;
	col = w->cursorX;
	if( col + 1 >= w->width )
	    edge = 1;
	else
	    edge = 0;
	for(; ls->data.line[i]; i++)
	    {
	    lc = ls->data.line[i];
	    ls->data.line[i] = c;
	    if( !edge )
		{
		if( col < w->width )
	    	    putchar(c);
		else if( col == w->width )
		    putchar('>');
		col++;
		}
	    c = lc;
	    }
	ls->data.line[i++] = c;
	ls->data.line[i] = '\0';
	ls->numChars++;
	if( !edge )
	    {
	    if( col < w->width )
	    	putchar(c);
	    else if( col == w->width )
		putchar('>');
	    w->cursorX++;
	    updateCursor(w);
	    }
	else
	    return(cursorRight(1));
	}
    else	/* Overstrike, never adds a char */
	{
	ls->data.line[w->leftOffset+w->cursorX-1] = c;

        /* Check if we are at the right edge */
        if( w->cursorX + 1 >= w->width )
	    return(cursorRight(1));
        /* Nope, do it cheap */
        w->cursorX++;
        putchar(c);
	}
    return(1);
}

unpackDiskLine(fileStruct *fs, lineStruct *ls)
{
char *line;

    ls->flags = ls->flags & ~1;	/* A real line now */
    if( ls->numChars == 0 )
	{	/* Nothing or new area */
	ls->numCharsAvail = 80;
        ls->data.line = (char *) malloc(81);
	ls->data.line[0] = '\0';
	return(1);
	}
	
    /* Get the line */
    ls->numChars = getChars(fs, ls, ls->numChars+1, &line);
    if( ls->numChars < 80 )
	ls->numCharsAvail = 80;
    else
	ls->numCharsAvail = ls->numChars + 20;
    ls->data.line = (char *) malloc(ls->numCharsAvail+1);
    strcpy(ls->data.line, line);
    return(1);
}
    
/* Get at least numchars and return a ptr to them. */
/* Returns number of chars */
getChars(fileStruct *fs, lineStruct *ls, int numChars, char **s)
{
int i, j, k, ix, tcnt;
char *line;

    if( ls->numCharsAvail )
	{
	*s = ls->data.line;
/* For debug */
#if 0
	if( strlen(ls->data.line) != ls->numChars )
		{
		char tmpe[12];
		sprintf(tmpe, "%d != %d", strlen(ls->data.line), ls->numChars);
		emsg(tmpe);
		ls->numChars = strlen(ls->data.line);
		}
#endif
	return(ls->numChars);
	}
    else if( ls->numChars == 0 )
	{
	*s = "";
	return(0);
	}
    fseek(fs->fp, ls->data.offset, SEEK_SET);
    if( numChars < ls->numChars )
	i = numChars;
    else
        i = ls->numChars;
    line = (char *) alloca(i+1);
    fread(line, 1, i, fs->fp);
    line[i] = '\0';

    /* How many tab expansion chars ? */
   for(i=0, ix=0; line[i]; i++)
        {
        if( line[i] == '\t' )
            ix += 8 - (ix % 8);
        else
            ix++;
        }

    if( ix+1 > globalData.lineBufferSize )
	{
	if( globalData.lineBufferSize )
	    free(globalData.lineBuffer);
	globalData.lineBufferSize = ix + 128;
	if( globalData.lineBufferSize < 1024 )
	    globalData.lineBufferSize = 1024;
	globalData.lineBuffer = (char *) malloc(globalData.lineBufferSize);
	}

    /* Put the data in final blank expanded format */
    for(i=j=0, ix=0; line[i]; i++)
        {
        if( line[i] == '\t' )
            {
            tcnt = 8 - (ix % 8);
            for(k=0; k<tcnt; k++)
                globalData.lineBuffer[j++] = ' ';
            ix += tcnt;
            }
        else
            {
            globalData.lineBuffer[j++] = line[i];
            ix++;
            }
        }
    globalData.lineBuffer[j] = '\0';
    *s = globalData.lineBuffer;
    return(j);
}

canModifyFile(fileStruct *fs)
{

    if( fs->ro )
	{
	dingMsg("Read Only");
	return(0);
	}
    fs->modified = 1;
    return(1);
}
