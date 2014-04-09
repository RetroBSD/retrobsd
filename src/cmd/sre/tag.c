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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#include "edit.h"
extern globalStruct globalData;

/* Can modify dirPath */
scanTags(char *funcName, char *dirPath, int ix, int *fcnt)
{
int i, j, bol, eol;
FILE *tagfp;
char bigLine[4096];
char *argv[2];

    argv[0] = "edit";
    strcpy(&dirPath[ix], "/tags");
    if( (tagfp=fopen(dirPath, "r")) == NULL )
	return(0);
    (*fcnt)++;
    while( fgets(bigLine, 4095, tagfp) )
	{
	for(i=0; funcName[i]; i++)
	    if( bigLine[i] != funcName[i] )
		break;
	if( funcName[i] != '\0' || bigLine[i] != '\t' )
	    continue;

	/* Matched next token is file name */
	dirPath[ix] = '/';
	for(i++, j=ix+1; bigLine[i]; i++, j++)
	    if( bigLine[i] == '\t')
		break;
	    else
		dirPath[j] = bigLine[i];
	dirPath[j] = '\0';
	argv[1] = dirPath;
	if( editFileCmd(2, argv) == 0)
	    {
	    fclose(tagfp);
	    return(0);
	    }
	fclose(tagfp);
	/* Next search for string in the file, editCmd has setup everything */
	i+=2;   /* skip over tab and leading slash */
	bol = eol = 0;
	if( bigLine[i] == '^' )
	    {
	    bol=1;
	    i++;
	    }
	for(j=i+1; bigLine[j]; j++)
	    ;
	if( bigLine[j-1] == '\n' )
	    j--;
	if( bigLine[j-1] == '/' )
	    j--;
	if( bigLine[j-1] == '$' )
	    {
	    eol = 1;
	    bigLine[j-1] = '\0';
	    }
	else
	    bigLine[j] = '\0';
	argv[1] = &bigLine[i];
	/* TODO, use bol/eol and make specific search command */
	searchDnCmd(2, argv);
	return(1);
	}
    return(0);
}

/* for tag command. Searches TAGPATH for all entries to find func */
tagCmd(int argc, char **argv)
{
windowStruct *w=globalData.activeWindow;
char *funcName, *tagPath, tagFile[4096];
struct stat buf;
DIR *dirp;
struct dirent *de;
int i, j, k, cnt=0;


    if( argc == 1 )
	{
	if( getWordUnderCursor(w, &funcName) == -1)
	    {
	    emsg("Need an argument to tag");
	    updateCursor(w);
	    return(0);
	    }
	}
    else
	funcName = argv[1];

    /* If none set, use current dir */
    if( !(tagPath = getenv("TAGPATH")) )
	tagPath = ".";

    for(i=j=0; 1; i++)
	{
	if( tagPath[i] != ':' && tagPath[i] != '\0' )
	    {
	    tagFile[j++] = tagPath[i];
	    continue;
	    }

	if( tagFile[j-1] == '*' )
	    {   /* Scan all subdirs of this directory for ctags files */
	    j--;
	    tagFile[j] = '\0';
	    if( (dirp = opendir(tagFile)) == NULL)
		{
		dingMsg("Bad Tag Path");
		continue;
		}
	    while( (de=readdir(dirp)) )
		{
		strcpy(&tagFile[j], de->d_name);
		k = strlen(tagFile);
		if( stat(tagFile, &buf) != 0)
		    continue;
		if( S_ISDIR(buf.st_mode) )
		    {
		    if( scanTags(funcName, tagFile, k, &cnt) )
			{
			closedir(dirp);
			goto FOUND;
			}
		    }
		}
	    }
	else
	    {
	    tagFile[j] = '\0';
	    if( scanTags(funcName, tagFile, j, &cnt) )
		goto FOUND;
	    }
	j=0;
	if( tagPath[i] == '\0' )
	    break;
	}
    if( cnt == 0 )
	emsg("No tag files; run ctags");
    else
	emsg("tag not found");
    updateCursor(w);
    return(0);
FOUND:
    return(1);
}
