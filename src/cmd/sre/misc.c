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
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#include "edit.h"
extern globalStruct globalData;

updateCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	{
	emsg("No file open;Use ^space edit <file> to open");
	updateCursor(w);
	return(0);
	}
    emsg("INFO: Changes to file will be saved");
    updateCursor(w);
    fs->nosave = 0;
    return(1);
}
   
noupdateCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	{
	emsg("No file open;Use ^space edit <file> to open");
	updateCursor(w);
	return(0);
	}
    emsg("INFO: Changes to file will NOT be saved");
    updateCursor(w);
    fs->nosave = 1;
    return(1);
}

insertModeCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;

    if( !(fs=w->fileInfo) )
	{
	emsg("No file open;Use ^space edit <file> to open");
	updateCursor(w);
	return(0);
	}
    globalData.insertMode = 1 - globalData.insertMode;
    redrawWindow(w);
    return(1);
}

/* Set number of spaces between tabs */
/* RE was more versatile, it was like a typewriter and */
/* tab could be set randomly. the tabs in RE just set each tabstop */
tabsCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
int cnt;

    if( argc != 2 )
	{
	emsg("Need an argument to tabs");
	updateCursor(w);
	return(0);
	}
    cnt = atoi(argv[1]);
    if( cnt <= 1 )
	{
	emsg("tabs arg must be > 1");
	updateCursor(w);
	return(0);
	}
    globalData.tabSpace = cnt;
    redrawWindow(w);
    return(1);
}

setCmd(int argc, char **argv)
{
int i;
windowStruct *w = globalData.activeWindow;
lineStruct *ls;

    if( argc != 3 )
	{
	emsg("set var value");
	updateCursor(globalData.activeWindow);
	return(0);
	}

    if( strcasecmp(argv[1], "insertMode") == 0)
	{
	if( atoi(argv[2]) == 0 )
	    globalData.insertMode = 0;
	else
	    globalData.insertMode = 1;
	}
    else if( strcasecmp(argv[1], "bell") == 0 )
	{
	if( atoi(argv[2]) == 0 )
	    globalData.ringBellOnErr = 0;
	else
	    globalData.ringBellOnErr = 1;
	}
    else if(strcasecmp(argv[1], "searchKey") == 0)
	{
	if( (i=(strlen(argv[2])+1)) >= globalData.srchBufferSize )
	    {
	    if( globalData.srchBuffer )
		free(globalData.srchBuffer);
	    if( i < 128 )
		i = 128;
	    globalData.srchBufferSize = i;
	    globalData.srchBuffer = (char *) malloc(i);
	    }
	/* set demands a quoted string, strip start/end quotes */
	strcpy(globalData.srchBuffer, &argv[2][1]);
	globalData.srchBuffer[strlen(globalData.srchBuffer)-1] = '\0';
	}
    else if( strcasecmp(argv[1], "rangetop") == 0 )
	{
	if( w )
	    {
	    w->rangeTopNum = atoi(argv[2]);
	    }
	}
    else if( strcasecmp(argv[1], "rangebot") == 0 )
	{
	if( w )
	    {
	    w->rangeBotNum = atoi(argv[2]);
	    }
	}
    else
	emsg("Unknown var");

    updateCursor(globalData.activeWindow);
    return(0);
}
