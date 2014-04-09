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
/* Startup related stuff */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <alloca.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "edit.h"
extern globalStruct globalData;

/* Read the .srerc file if it exists */
readRC()
{
char *s;
char line[1024];
FILE *fp;

    if( (s=getenv("HOME")) == NULL )
	return(0);
    sprintf(line, "%s/.srerc", s);
    if( !(fp=fopen(line, "r")) )
	return(0);
    while(fgets(line, 1000, fp) != NULL)
	if( processCmd(line) == 0)
	    {
	    printf("Bad .srerc init file; correct and restart\n");
	    exit(0);
	    }
    fclose(fp);
    return(1);
}

readStartup()
{
FILE *fp;
char line[1024];

    if( !(fp=fopen(".sre_init", "r")) )
        return(0);

    while(fgets(line, 1000, fp) != NULL)
        processCmd(line);
    fclose(fp);
}

writeStartup()
{
FILE *fp;
windowStruct *w;

    if( !(fp=fopen(".sre_init", "w")) )
        return(0);

    writeWindowStartup(fp);
    if( globalData.insertMode )
	fprintf(fp, "set insertMode 1\n");
    if( globalData.srchBuffer )
	fprintf(fp, "set searchKey \"%s\"\n", globalData.srchBuffer);
    fclose(fp);
}

writeWindowStartup(FILE *fp)
{
fileStruct *fs;
fileWinParams *fw;
int i, j, wcnt;
windowStruct **warray, *ww, *w;

    /* create a list of the windows */
    for(wcnt=0, ww=globalData.windowList; ww; ww=ww->next)
	wcnt++;
    warray = (windowStruct **) alloca(sizeof(windowStruct *) * wcnt);
    for(i=1, ww=globalData.windowList; ww; ww=ww->next, i++)
	warray[wcnt-i] = ww;

    for(j=0; j<wcnt; j++)
	{
	w = warray[j];
	if( j != 0 )   /* top window does not need a open command */
	    {
	    fprintf(fp, "cwin %d\n", w->parent->winNum);
	    if( w->splitVertical == 0 )
		fprintf(fp, "win \"\" %d 1\n", w->cutPosition);
	    else
		fprintf(fp, "win \"\" 1 %d\n", w->cutPosition);
	    }
	/* Write out the commands to open the existing files in the window */
	for(i=0; i<NUM_ALT_FILES; i++)
	    {
	    if( w->curParamSet == i )
		continue;
	    fw=&w->editStack[i];
	    if( (fs=fw->fileInfo) )
		{
		fprintf(fp, "edit %s %lld %d %d %d\n",
		    fs->fileName,
		    fw->topOffset, fw->leftOffset, fw->cursorLineNo, fw->cursorX);
		fprintf(fp, "set rangeTop %lld\n", w->editStack[i].rangeTopNum);
		fprintf(fp, "set rangeBot %lld\n", w->editStack[i].rangeBotNum);
		}
	    }
	/* Write the primary last so it is current */
	if( (fs = w->fileInfo) )
	    {
	    fprintf(fp, "edit %s %lld %d %d %d\n",
		    fs->fileName,
		    w->topOffset, w->leftOffset, w->cursorLineNo, w->cursorX);
	    fprintf(fp, "set rangeTop %lld\n", w->rangeTopNum);
	    fprintf(fp, "set rangeBot %lld\n", w->rangeBotNum);
	    }
	}
    fprintf(fp, "cwin %d\n", globalData.activeWindow->winNum);
    return(1);
}
