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

/* key is ^X etc for a control char */
/* ^A is one, ^space is 0 */

defineKeyCmd(int argc, char **argv)
{
int i, c, len;
char escapeSequence[10];
cmdStruct *cmd, *cmdx;
ucmdStruct *ucmd;

    if( argc < 3 )
	{
	emsg("defineKey: key cmd [args]");
	return(0);
	}
    /* parse the key part */
    if( argv[1][0] == '^' )
        {
	if( strlen(argv[1]) == 2 )
	    {
	    c = argv[1][1];
	    if( c >= 64 && c < 96 )
		c = c-64;
	    else if( c >= 97 && c <= 122 )
		c = c - 96; /* Lower case a-z */
	    else
		{
BADCTL:
		printf("Unknown control character %c\n\r", c);
		printf("Value specified is in decimal\n\r");
		printf("Must be a single character or backspace.\n");
		return(0);
		}
	    }
	else if( strcasecmp(&argv[1][1], "bs") == 0 ||
		 strcasecmp(&argv[1][1], "backspace") == 0 )
	    c = 127;
	else
	    goto BADCTL;
        }
    else
	{   /* An escape sequence, escape is assumed */
	if( strlen(argv[1]) > 6 )
	    {
	    printf("Escape sequence should never be more than 6 characters\n");
	    return(0);
	    }
	strcpy(escapeSequence, argv[1]);
	c = -1;
	}

    /* If there are more than 2 args, then we need to convert it to a string type command */
    /* In any event, argv[2], must be a known command */
    if( findCmd(argv[2], &cmd, 0) == -1 )
	{
	printf("No command named %s; ignore binding\n", argv[2]);
	return(0);
	}
    if( argc == 3 ) /* Simple atomic command */
	cmdx = cmd;
    else
	{   /* Need to create a command string that will get executed by processCmd */
	ucmd = (ucmdStruct *) calloc(1, sizeof(ucmdStruct) + sizeof(char *)*(argc-2));
	ucmd->func = cmd->func;
	ucmd->flags = ILLEGAL_IN_MARK | IGNORE_IN_ARG;
	ucmd->argc = argc-2;
	for(i=2; i<argc; i++)
	    {
	    ucmd->argv[i-2] = (char *)malloc(strlen(argv[i])+1);
	    strcpy(ucmd->argv[i-2], argv[i]);
	    }
	cmdx = (cmdStruct *) ucmd;
	}
    if( c == -1 )
	{   /* Multi key definition */
	defineMultiCharKey(escapeSequence, cmdx);
	}
    else
	globalData.keyMap[c] = cmdx;
    return(1);
}

defineMultiCharKey(char *keySeq, cmdStruct *cptr)
{
multiCharKeyDef *mk;

    mk = (multiCharKeyDef *)calloc(1, sizeof(multiCharKeyDef));
    mk->charSeq = strdup(keySeq);
    mk->cmd = cptr;
    mk->next = globalData.multiList;
    globalData.multiList = mk;
    return(1);
}
    
setupCmdMap()
{
cmdStruct *cptr;

    findCmd("arg", &globalData.keyMap[0], 0);
    findCmd("pageLeft", &globalData.keyMap[1], 0);	/* A */
    findCmd("edit", &globalData.keyMap[2], 0);		/* B */
    findCmd("open", &globalData.keyMap[4], 0);		/* D */
    findCmd("searchUp", &globalData.keyMap[5], 0);	/* E */
    findCmd("cut", &globalData.keyMap[6], 0);		/* F */
	/* Note that an empty arg with a cut is an uncut */
    findCmd("mark", &globalData.keyMap[7], 0);		/* G */
    findCmd("cursorLeft", &globalData.keyMap[8], 0);	/* H */
    findCmd("tabRight", &globalData.keyMap[9], 0);	/* I */
    findCmd("cursorDn", &globalData.keyMap[10], 0);	/* J */
    findCmd("cursorRight", &globalData.keyMap[11], 0);	/* K */
    findCmd("get", &globalData.keyMap[12], 0);		/* L */
	/* Note that an empty arg with a get is a put */
    findCmd("return", &globalData.keyMap[13], 0);	/* M */
    findCmd("cursorUp", &globalData.keyMap[14], 0);	/* N */
    findCmd("insertMode", &globalData.keyMap[15], 0);	/* O */
    findCmd("pageUp", &globalData.keyMap[16], 0);	/* P */
    findCmd("searchDn", &globalData.keyMap[18], 0);	/* R */
    findCmd("save", &globalData.keyMap[19], 0);         /* S */
    findCmd("scrollDn", &globalData.keyMap[20], 0);	/* T */
    findCmd("wipeChar", &globalData.keyMap[21], 0);	/* U */
    findCmd("pageRight", &globalData.keyMap[22], 0);    /* V */
    findCmd("scrollUp", &globalData.keyMap[23], 0);	/* W */
    findCmd("exit", &globalData.keyMap[24], 0);		/* X */
    findCmd("pageDn", &globalData.keyMap[25], 0);	/* Y */
    findCmd("cwin", &globalData.keyMap[26], 0);		/* Z */
    findCmd("tabLeft", &globalData.keyMap[29], 0);	/* ] */
    findCmd("deleteChar", &globalData.keyMap[127], 0);	/* del/bs */

    /* Default multi-char keys */
    findCmd("cursorDn", &cptr, 0);	/* arrow keys */
    defineMultiCharKey("[B", cptr);
    findCmd("cursorUp", &cptr, 0);	
    defineMultiCharKey("[A", cptr);
    findCmd("cursorLeft", &cptr, 0);
    defineMultiCharKey("[D", cptr);
    findCmd("cursorRight", &cptr, 0);
    defineMultiCharKey("[C", cptr);
    
    findCmd("match", &cptr, 0);
    defineMultiCharKey("[E", cptr);

    findCmd("pageUp", &cptr, 0);
    defineMultiCharKey("[5~", cptr);
    findCmd("pageDn", &cptr, 0);
    defineMultiCharKey("[6~", cptr);


    findCmd("deleteChar", &cptr, 0);
    defineMultiCharKey("[3~", cptr);
}

struct termios tioOrigIn;
struct termios tioOrigOut;

restoreTerm()
{
    /* Restore main screen */
    printf("\033[?47l");
    printf("\033[%d;1H", globalData.winHeight);
    tcsetattr(fileno(stdin), TCSANOW, &tioOrigIn);
    tcsetattr(fileno(stdout), TCSANOW, &tioOrigOut);
}

setupTerm()
{
int c, w, h, i;
char tmp[20];
struct termios tio;

    /* These appear to be tied together, as a req for stdout is modified */
    /* by the stdin request */
    tcgetattr(fileno(stdout), &tioOrigOut);
    tcgetattr(fileno(stdin), &tio);
    memcpy( &tioOrigIn, &tio, sizeof(struct termios));
    cfmakeraw(&tio);
    tcsetattr(fileno(stdin), TCSAFLUSH, &tio);
    tcgetattr(fileno(stdout), &tio);
    cfmakeraw(&tio);
    /* Make the terminal non-blocking and wait 1/10th of a second for a char */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1;
    tcsetattr(fileno(stdout), TCSADRAIN, &tio);
    tcgetattr(fileno(stdout), &tio);
    /* Get terminal height/width */
    printf("\033[18t");
    i = 0;
    while((c=getc(stdin)) != EOF && i < 20 )
	{
	if( c == 't' )
	    {
	    tmp[i] = '\0';
	    break;
	    }
	tmp[i++] = c;
	}
    if( i == 20 )
	{
	globalData.winHeight = 24;
	globalData.winWidth = 80;
	emsg("Failed to get terminal width/height\n");
	}
    else
	{
	sscanf(&tmp[4], "%d;%d", &h, &w);
	globalData.winHeight = h;
	globalData.winWidth = w;
	/* switch to alternate screen */
	printf("\033[?47h");
	}
}

emsg(char *s)
{
int i, j;

    if( globalData.mode == 0 )
	printf("%s\n", s);
    else
	{
	printf("\033[%d;%dH",
		globalData.winHeight, 1);
	for(i=j=0; i<globalData.winWidth/2; i++)
	    {
	    if( s[j] )
		putchar(s[j++]);
	    else
		putchar(' ');
	    }
	updateCursor(globalData.activeWindow);
	}
}

dingMsg(char *s)
{

    if( globalData.ringBellOnErr )
	{
	printf("\a");
	fflush(stdout);
	}
    emsg(s);
}
