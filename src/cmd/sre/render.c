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
#include <termios.h>
#include <unistd.h>
#include <alloca.h>

#include "edit.h"

extern globalStruct globalData;

refreshScreen()
{
int i;

    /* Clear screen */
    printf("\033[2J");
    for(i=0; i<globalData.winWidth; i++)
	putchar('-');
    printf("\n\r");
    for(i=1; i<globalData.winHeight-2; i++)
	printf("|\033[%dC|\n\r", globalData.winWidth-2);
    for(i=0; i<globalData.winWidth; i++)
	putchar('-');
    printf("\n\r");
    if( globalData.activeWindow->fileInfo)
	{
	updateStatus();
	updateCursor(globalData.activeWindow);
	}
    else
        {
        printf(" No file open;Use ^space edit <file> to open");
        printf("\033[%d;%dH", 4, 4);
        printf("sre  Copyright (C) 2011 Mike Stabenfeldt\n\r");
        printf("\033[%d;%dH", 5, 4);
        printf("This program comes with ABSOLUTELY NO WARRANTY;\n\r");
        printf("\033[%d;%dH", 7, 4);
        printf("This is free software, and you are welcome\n\r");
        printf("\033[%d;%dH", 8, 4);
        printf("to redistribute it under certain conditions;\n\r");
        printf("\033[%d;%dH", 9, 4);
        printf("goto http://www.gnu.org/licenses for details.\n\r");

        printf("\033[%d;%dH", 12, 4);
        printf("Exit with <ctrl>X\n\r");
        }


}

redrawWindow(windowStruct *w)
{
int i, j, k, l, active, six;
lineStruct *ls;
char *s, *screen;

    if( !w->fileInfo )
        return(0);
    active = 0;
    i = w->width + 5;
    screen = (char *) alloca(i);
    if( w == globalData.activeWindow )
	active = 1;
    if( !w->topLine )   /* Ah, dirty from another window delete */
	{
	if( w->fileInfo->numLines < w->topOffset-2 )
	    w->topOffset = w->fileInfo->numLines-2;
	if( w->topOffset < 0 )
	    w->topOffset = 0;
	getLine(w, w->fileInfo, w->topOffset, &w->topLine);
	}
    ls = w->topLine;
    /* Top decoration */
    printf("\033[%d;%dH", w->y, w->x);
    if( active )
	{
	i=0;
	if( globalData.insertMode )
	    screen[i++] = 'I';
	else
	    screen[i++] = 'O';
        for(; i<w->width; i++)
	    {
	    if( i != 1 && (i-1 + w->leftOffset) % globalData.tabSpace == 0 )
		screen[i] = 'T';
	    else if( i == 1 && ( w->rangeTopNum || w->rangeBotNum) )
		screen[i] = 'R';
	    else
		screen[i] = '-';
	    }
	}
    else
	{
        for(i=0; i<w->width; i++)
	    screen[i] = '.';
	}
    screen[i] = '\0';
    printf("%s", screen);
    for(i=0; i<w->height-1; i++)
	{	/* set to start & put out decoration */
	six=0;
        printf("\033[%d;%dH", i+w->y+1, w->x);
	if( active )
	    {
	    if( ls )
		{
	        if( w->leftOffset == 0 )
		    {
		    if( ls->flags & 1) /* Internal Filler line */
		        screen[six++] = ':';
		    else if( w->rangeTopNum || w->rangeBotNum )
			{
			screen[six] = '*';
			if( w->rangeTopNum == 0 )
			    {
			    if( w->topOffset + i < w->rangeBotNum )
				screen[six] = '|';
			    }
			else if( w->rangeBotNum == 0 )
			    {
			    if( w->topOffset + i + 1 >= w->rangeTopNum )
				screen[six] = '|';
			    }
			else
			    {
			    if( w->topOffset + i < w->rangeBotNum &&
				w->topOffset + i + 1 >= w->rangeTopNum )
				screen[six] = '|';
			    }
			six++;
			}
		    else
			screen[six++] = '|';
		    }
	        else
		    screen[six++] = '<';
		}
	    else
		screen[six++] = ';';
	    }
	else
	    screen[six++] = '.';
	if( ls )
	    {
	    getChars(w->fileInfo, ls, w->leftOffset + w->width+1, &s);
	    for(j=k=0; j<w->leftOffset+w->width-1; )
	        {
	        if( s[k] == '\0')
		    {
		    if( j >= w->leftOffset )
			screen[six++] = ' ';
		    j++;
		    }
	        else if( s[k] == '\t' )
		    {
		    k++;
		    for(l=0; j < w->width-1 && l < 8; j++, l++)
			if( j >= w->leftOffset )
			    screen[six++] = ' ';
		    }
		else
		    {
		    if( j >= w->leftOffset )
			screen[six++] = s[k];
		    k++;
		    j++;
		    }
	        }
	    if( active )
		{
	        if( s[k] != '\0' )
		    screen[six++] = '>';
		else if(active)
		    screen[six++] = '|';
		else
		    screen[six++] = ';';
		}
	    else
		screen[six++] = '.';

	    ls = ls->next;
	    }
	else
	    {
	    for(j=0; j<w->width-1; j++)
		screen[six++] = ' ';
	    if( active )
		screen[six++] = ';';
	    else
		screen[six++] = '.';
	    }
	screen[six] = '\0';
	printf("%s", screen);
	}
    /* Bottom decoration and status display update */
    if( active )
	{
	if( globalData.searchMode == 0 )
	    screen[0] = 'L';
	else if( globalData.searchMode == 1 )
	    screen[0] = 'C';
	else
	    screen[0] = 'R';
        for(i=1; i<w->width; i++)
	    screen[i] = '-';
	if( globalData.activeWindow->fileInfo &&
	    globalData.activeWindow->fileInfo->modified )
	    screen[i++] = '*';
	else
	    screen[i++] = '+';
	screen[i] = '\0';
        printf("\033[%d;%dH%s", w->y+w->height, w->x, screen);
	updateStatus();
	updateCursor(w);
	}
    else
	{
        for(i=0; i<w->width; i++)
	    screen[i] = '.';
	screen[i] = '\0';
        printf("\033[%d;%dH%s", w->y+w->height, w->x, screen);
	}
    return(1);
}


redrawLine(windowStruct *w, int offset, lineStruct *ls)
{
int i, j, k, l, active, six;
char *s, *screen;

    active = 0;
    i = w->width + 5;
    screen = (char *) alloca(i);
    if( w == globalData.activeWindow )
	active = 1;

	six=0;
        printf("\033[%d;%dH", offset+w->y, w->x);
	if( active )
	    {
	    if( ls )
		{
		if( w->leftOffset == 0)
		    {
		    if( ls->flags & 1 )
		        screen[six++] = ':';
		    else
			screen[six++] = '|';
		    }
	        else
		    screen[six++] = '<';
		}
	    else
		screen[six++] = ';';
	    }
	else
	    screen[six++] = '.';
	if( ls )
	    {
	    getChars(w->fileInfo, ls, w->leftOffset + w->width+1, &s);
	    for(j=k=0; j<w->leftOffset+w->width-1; )
	        {
	        if( s[k] == '\0')
		    {
		    if( j >= w->leftOffset )
			screen[six++] = ' ';
		    j++;
		    }
	        else if( s[k] == '\t' )
		    {
		    k++;
		    for(l=0; j < w->width-1 && l < 8; j++, l++)
			if( j >= w->leftOffset )
			    screen[six++] = ' ';
		    }
		else
		    {
		    if( j >= w->leftOffset )
			screen[six++] = s[k];
		    k++;
		    j++;
		    }
	        }
	    if( active )
		{
	        if( s[k] != '\0' )
		    screen[six++] = '>';
		else if(active)
		    screen[six++] = '|';
		else
		    screen[six++] = ';';
		}
	    else
		screen[six++] = '.';
	    }
	else
	    {
	    for(j=0; j<w->width-1; j++)
		screen[six++] = ' ';
	    if( active )
		screen[six++] = ';';
	    else
		screen[six++] = '.';
	    }
	screen[six] = '\0';
	printf("%s", screen);
}

/* Redraw everything */
redrawScreen()
{
windowStruct *w;

    refreshScreen();
    for(w=globalData.windowList; w; w=w->next)
	if( w != globalData.activeWindow )
	    redrawWindow(w);
    redrawWindow(globalData.activeWindow);
}

/* Update the current cursor position on the screen */
updateCursor(windowStruct *w)
{
	printf("\033[%d;%dH", 
		w->y+w->cursorLineNo,
		w->x+w->cursorX);
}

updateStatus()
{
windowStruct *w=globalData.activeWindow;
char statusLine[256];
int len, flen, avail, cnt;

    if( w->fileInfo)
	{
	/* Set cursor pos and erase to right */
	/* May not have enough room for full file name */
	printf("\033[%d;%dH\033[0K",
		globalData.winHeight,
		globalData.winWidth/2);
	avail = globalData.winWidth/2;
	len = sprintf(statusLine, " %-4lldx%d in ",
		w->cursorLineNo +
		w->topOffset, 
		w->cursorX +
		w->leftOffset);
	if( len+3 > avail )
	    statusLine[avail] = '\0';
	else
	    {   /* Otherwise no room at all */
	    flen = strlen(w->fileInfo->fileName);
	    if( flen + len < globalData.winWidth/2 )
		strcpy(&statusLine[len], w->fileInfo->fileName);
	    else
		{
		statusLine[len++] = '.';
		statusLine[len++] = '.';
		cnt = avail - len;
		strcpy( &statusLine[len],
			  &(w->fileInfo->fileName[flen - cnt]));
		}
	    }
	fputs(statusLine, stdout);
	}
    if( globalData.markSet )
	{
	int dx, len;
	long long dy;
	char tmp[512];

	dx = w->leftOffset + w->cursorX - globalData.markRefX;
	dy = w->topOffset + w->cursorLineNo - globalData.markRefY;
	if( dx == 0 )
	   len = sprintf(tmp, "MARK: %lld", dy);
	else
	   len = sprintf(tmp, "MARK: %lldx%d", dy, dx);
	for(; len<globalData.winWidth/2; len++)
	    tmp[len] = ' ';
	tmp[len] = '\0';
	printf("\033[%d;1H%s", globalData.winHeight, tmp);
	}
}

clearArgDisplay()
{
char tmp[512];
int i;

    for(i=0; i<globalData.winWidth/2; i++)
	tmp[i] = ' ';
    tmp[i] = '\0';
    printf("\033[%d;1H%s", globalData.winHeight, tmp);
}
