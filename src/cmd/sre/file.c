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

exitCmd(int argc, char **argv)
{
char *arg;
fileStruct *fs;
int mode=0;

    if( argc > 2 )
	goto INVALID;
    arg = argv[1];
    if( argc > 1 )
	{	/* Precheck arg */
	if( strncasecmp(arg, "nosave", strlen(arg)) == 0 )
            ;   /* Don;t save any files */
        else if( strcasecmp(arg, "abort") == 0 )
            ;   /* Don't save files or write init */
        else if( strcasecmp(arg, "stop") == 0 )
	    ;   /* Suspend */
	else
	    {	/* Invalid */
INVALID:
	    dingMsg("exit [nosave|abort|stop]");
	    updateCursor(globalData.activeWindow);
	    return(0);
	    }
	}
    /* Go thru all files that have been touched and save as necessary */
    printf("\033[2J\033[1;1H\n\r");
    restoreTerm();
    globalData.mode = 0;	/* So emsg just prints */

    if( argc > 1 )
	{
        if( strncasecmp(arg, "nosave", strlen(arg)) == 0 )
            mode = 1;
        else if( strncasecmp(arg, "abort", strlen(arg)) == 0 )
            goto SKIP;
	else if( strcasecmp(arg, "stop") == 0 )
	    {
	    /* Can't seem to get timing right to restore the screen */
	    restoreTerm();
	    printf("\n");   /* Force a flush for restore screen */
	    sleep(1);
	    kill(getpid(), SIGSTOP);
	    sleep(1);
	    setupTerm();
	    globalData.mode = 1;
	    redrawScreen();
	    return(0);
	    }
	}
    writeStartup();
    if( mode == 1)
        goto SKIP;
    for(fs=globalData.fileList; fs; fs=fs->next)
	{
	if( fs->ro )
	    printf("Read only %s\n", fs->fileName);
	else if( fs->modified && !fs->nosave )
	    {
	    printf("Saving %s\n", fs->fileName);
	    if( fs->backupWritten == 0 && fs->created == 0)
		writeBackup(fs);
	    writeFile(fs, fs->fileName, 0);
	    }
	}
SKIP:
    exit(0);
}

saveCmd(int argc, char **argv)
{
windowStruct *w = globalData.activeWindow;
fileStruct *fs;
char tmp[512];
char *fName, *s;
int i, wrotePrimary=0, isNew=0;

    if( !(fs=w->fileInfo) )
	{
	emsg("No file in window to save");
	updateCursor(w);
	return(0);
	}
    if( argc == 1 )
	{	/* Save to existing name */
PRIMARY:
	if( fs->ro )
	    {
	    dingMsg("Read Only");
	    updateCursor(w);
	    return(0);
	    }
	fName = fs->fileName;
	if( fs->modified == 0 )
	    {
	    dingMsg("Nothing to save");
	    updateCursor(w);
	    return(0);
	    }
	if( fs->backupWritten == 0 && fs->created == 0 )
	    writeBackup(fs);
	wrotePrimary = 1;
	}
    else if(argc != 2 )
	{
	dingMsg("save command has 0 or 1 arguments");
	updateCursor(w);
	return(0);
	}
    else
	{
	if( argv[1][0] == '\0' )
	    goto PRIMARY;
	fName = argv[1];
	for(s=argv[1]; *s; s++)
	    if( iscntrl(*s) || isspace(*s) )
		{
		emsg("Filename has control or spaces; save skipped");
		updateCursor(w);
		return(0);
		}
	isNew = 1;
	}
    strcpy(tmp, "SAVE: ");
    for(s=fName, i=6; *s && i<globalData.winWidth/2; i++, s++)
	tmp[i] = *s;
    tmp[i] = '\0';
    emsg(tmp);
    if(writeFile(fs, fName, isNew))
	{
	if( wrotePrimary )
	    fs->modified = 0;
        strcpy(tmp, "SAVED");
        tmp[5] = ' ';
	emsg(tmp);
	return(1);
	}
    return(0);
}
	
openFile(char *fileName, windowStruct *w)
{
FILE *fp;
fileStruct *fs;
lineStruct *cur, *prev;
int isNew = 0, c, newNeeded; 
unsigned long long offset;
struct stat statbuf;

AGAIN:
    if( stat(fileName, &statbuf) != 0 )
	{
	if( errno == ENOENT )
	    {
	    emsg("File does not exist; create?");
	    while( (c=getc(stdin)) )
		{
		if( c == 'y' || c == 'Y' )
		    {
		    if( (fp=fopen(fileName, "w")) == NULL )
			{
			dingMsg("File create failed; sorry");
			return(0);
			}
		    fclose(fp);
		    emsg("Create file succeeded ......");
		    stat(fileName, &statbuf);
		    isNew = 1;
		    goto AGAIN;
		    }
		else if( c != EOF )
		    {
		    emsg("Canceled edit");
		    return(0);
		    }
		}
	    }
	emsg("File could not be opened");
	return(0);
	}

    for(fs=globalData.fileList; fs; fs=fs->next)
	if( fs->inode == statbuf.st_ino && fs->devNo == statbuf.st_dev)
	    break;
    if( fs )
	goto WIN_ATTACH;

    if( (fp=fopen(fileName, "r")) == NULL )
	{
	emsg("Could not open file");
	return(0);
	}
    fs = (fileStruct *) calloc(1, sizeof(fileStruct));
    fs->devNo = statbuf.st_dev;
    fs->inode = statbuf.st_ino;
    fs->created = isNew;
    fs->accessMode = statbuf.st_mode;
    if( statbuf.st_uid == getuid() && (statbuf.st_mode & S_IWUSR) )
	fs->ro = 0;
    else if( statbuf.st_gid == getgid() && (statbuf.st_mode & S_IWGRP) )
	fs->ro = 0;
    else if( statbuf.st_mode & S_IWOTH )
	fs->ro = 0;
    else
	{
	fs->ro = 1;
	dingMsg("Read only");
	}
    fs->fileName = strdup(fileName);
    fs->next = globalData.fileList;
    globalData.fileList = fs;

    /* Read in the file into the buffer */
    newNeeded = 1;
    offset = 0;
    prev = NULL;
    while( (c=getc(fp)) != EOF )
	{
	if( newNeeded )
	    {
	    cur = (lineStruct *) calloc(sizeof(lineStruct), 1);
	    cur->data.offset = offset;
	    newNeeded = 0;
	    cur->prev = prev;
	    if( prev )
		prev->next = cur;
	    else
		fs->head = cur;
	    prev = cur;
	    }
	if( c == '\n' )
	    {
	    fs->numLines++;
	    newNeeded = 1;
	    }
	else
	    cur->numChars++;
	offset++;
	}
    if( !newNeeded )	/* File did not end w/newline */
	fs->numLines++;
    if( fs->numLines == 0 )	/* Empty file, need something */
	{
	w->topLine = fs->head = fs->tail = 
	    (lineStruct *) calloc(sizeof(lineStruct), 1);
	w->topLine->flags = 1;
	fs->numLines = 1;
	}
    else
        fs->tail = cur;
    fs->fp = fp;

WIN_ATTACH:
    w->fileInfo = fs;
    w->cursorX = w->cursorLineNo = 1;
    w->topLine = fs->head;
    w->topOffset = 0;
    w->cursorX = w->cursorLineNo = 1;
    w->leftOffset = 0;

    return(1);
}

/* Really just a rename */
writeBackup(fileStruct *fs)
{
char backupFileName[2048], *p, *src;
int i, j;

    if(fs->backupWritten )
	return(0);
    if( strlen(fs->fileName) >= 2040 )
	return(0);
    fs->backupWritten = 1;
    if( (p=rindex(fs->fileName, '/')) )
	{	/* Has path */
	for(i=0, src=fs->fileName; src != p; i++, src++)
	    backupFileName[i] = *src;
	backupFileName[i++] = '/';
	backupFileName[i++] = ',';
	src++;
	for(; i<2048 && *src; i++, src++)
	    backupFileName[i] = *src;
	if( i >= 2040 )
	    return(0);
	backupFileName[i] = '\0';
	}
    else
	{
	backupFileName[0] = ',';
	strcpy(&backupFileName[1], fs->fileName);
	}
    return(1+rename(fs->fileName, backupFileName));
}

/* Never bother to free as files are never really closed once opened */
/* Convert leading blanks to tabs if possible and */
/* remove trailing blanks on lines in memory. Disk */
/* lines are written unmodified */
writeFile(fileStruct *fs, char *fName, int isNew)
{    
FILE *fp;
lineStruct *ls, *ps;
int i, c, j, tabCnt, blankCnt, hadNonblank;
struct stat statbuf;

    if( fs->ro && !isNew )
	{
	dingMsg("File is Read-Only");
	return(0);
	}
    fp = fopen(fName, "w");
    if( fp == NULL )
	{
	dingMsg("Failed to open file");
	return(0);
	}
    fchmod(fileno(fp), fs->accessMode);
    ps = NULL;
    for(ls=fs->head; ls; ls=ls->next)
	{
	if( ps )	/* Was it a filler fake ? */
	    {
	    if( !(ls->flags & 1))
		{	/* It was interim filler, put in the blank lines */
		for( ; ps != ls; ps=ps->next)
		    putc('\n', fp);
		ps = NULL;
		}
	    }
	if( ls->numCharsAvail == 0 )
	    {	/* On disk still, just copy from disk */
	    if( ls->flags & 1)	/* A filler line */
		{
		if( !ps )
		    ps = ls;
		}
	    else
		{
		fseek(fs->fp, ls->data.offset, SEEK_SET);
		for(i=0; i<ls->numChars; i++)
		    if( (c=getc(fs->fp)) == EOF )
			{
			emsg("Unexpected EOF on read");
			break;
			}
		    else
			putc(c, fp);
		putc('\n', fp);
		}
	    }
	else
	    {
            hadNonblank = blankCnt = 0;
	    for(i=0; i<ls->numChars; i++)
                {
                if( ls->data.line[i] == ' ' )
                    blankCnt++;
                else
                    {
                    if( blankCnt )
                        {
                        if( hadNonblank == 0 )
                            {   /* no chars yet, convert as many as possible to tabs */
                            tabCnt = blankCnt / 8;
                            for(j = 0; j<tabCnt; j++)
                                putc('\t', fp); 
                            blankCnt = blankCnt - tabCnt * 8;
                            }
                        for(j=0; j<blankCnt; j++)
                            putc(' ', fp);
			blankCnt = 0;
                        }
                    hadNonblank = 1;
                    putc(ls->data.line[i], fp);
                    }
                }
	    putc('\n', fp);
	    }
	}
    /* Every time we write, need to update the file info's inode info so */
    /* a subsequent open does not create a new file entry */
    if( fstat(fileno(fp), &statbuf) == 0 )
	{
	fs->devNo = statbuf.st_dev;
	fs->inode = statbuf.st_ino;
	}
    fclose(fp);
    return(1);
}


