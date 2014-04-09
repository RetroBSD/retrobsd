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

#define NUM_ALT_FILES 4

typedef struct lineStruct {
	unsigned short numCharsAvail; /* 0 if it is an index into the file */
	unsigned short numChars;
	unsigned char  flags;	      /* 1 implies filler */
	union {
		char *line;
		unsigned long long offset;
		} data;
	struct lineStruct *prev;
	struct lineStruct *next;
	} lineStruct;

/* Can be in more than one window */
typedef struct fileStruct {
	unsigned int created:1;		/* New file */
	unsigned int modified:1;	/* Since last save */
	unsigned int nosave:1;
	unsigned int backupWritten:1;	/* So we only write 1 comma file */
	unsigned int ro:1;
	unsigned int accessMode;
	long long numLines;
	char *fileName;
	int devNo;
	long long inode;
	FILE *fp;
	lineStruct *head;
	lineStruct *tail;
	struct fileStruct *next;
	} fileStruct;

/* We do not store topLine as another window or stack could have changed it */
/* Even the cursorX/cursorLineNo may have to be adjusted if windows were */
/* opened or closed */
typedef struct fileWinParams {
	long long topOffset;
	unsigned short leftOffset;
	unsigned short cursorX;
	unsigned short cursorLineNo;
	long long rangeTopNum;           /* If non-z, top of range */
	long long rangeBotNum;           /* If non-z, bottom of range */
	fileStruct *fileInfo;
	} fileWinParams;
	
typedef struct windowStruct {
	unsigned char  curParamSet;
	unsigned char  splitVertical;
	unsigned char  rangeMode;       /* If set, cmd's are limited to range */
	unsigned char  winNum;  /* 0 is root */
	unsigned short x;	/* Beginning of window including decoration */
	unsigned short y;
	unsigned short width;	/* Size includes decoration */
	unsigned short height;
	unsigned short cutPosition;     /* How we came about for startup */
	lineStruct *topLine;            /* In window */
	lineStruct *cursorLine;         /* For faster access. Gets reset to NULL alot */
	/* Start same as fileWinParams */
	long long topOffset;            /* On screen offset to top line */
					/* 0 means top line is 1st line of file */
	unsigned short leftOffset;
	unsigned short cursorX;         /* On screen offset, does not include left offset */
	unsigned short cursorLineNo;            /* On screen offset from topLine */
	long long rangeTopNum;           /* If non-z, top of range */
	long long rangeBotNum;           /* If non-z, bottom of range */
	fileStruct *fileInfo;
	/* End same as fileWinParams */
	fileWinParams editStack[NUM_ALT_FILES];     /* Allow up to N files on stack */
					/* Original was only 2 */
	struct windowStruct *parent;	/* Who we came from */
	struct windowStruct *next;
	} windowStruct;

typedef struct bufferStruct {
	unsigned char mode;	/* 0 is lines, 1 is rectangle */
	unsigned short width;	/* only meaningful in rect mode */
	int numLines;
	char *lines[1];
	} bufferStruct;

typedef struct cmdStruct {
	char *funcName;
	int (*func)();
	unsigned int flags;
	} cmdStruct;

/* A "user" defined one. Really just a predefined with args */
/* Note the entire struct is alloc'ed as one chunk for easy free */
typedef struct ucmdStruct {
	char *funcName; /* NULL as a marker for user defined */
	int (*func)();  /* Func to call */
	unsigned int flags;
	int argc;       /* How many args, func counts as 1 */
	char *argv[1];  /* The argument vector for the callee */
	} ucmdStruct;

/* For keys that send escape sequences */
typedef struct multiCharKeyDef {
	char *charSeq;
	cmdStruct *cmd;
	struct multiCharKeyDef *next;
	} multiCharKeyDef;

/* Keeps track of all top level info and is global */
typedef struct globalStruct {
	unsigned char tabSpace;		/* Number of spaces between tabs */
	unsigned char mode;		/* 0 is not setup yet */
	unsigned char argMode;
	unsigned char insertMode;	/* 1 if insert, 0 for overwrite */
	unsigned char searchMode;	/* 1 if pattern, 0 for literal */
	unsigned char markSet;		/* 1 if we have a mark set */
	unsigned char ringBellOnErr;
	unsigned short winWidth;	/* In chars */
	unsigned short winHeight;	/* In lines */
	unsigned short argPtr;
	unsigned short srchBufferSize;
	unsigned short argBufferSize;
	unsigned short lineBufferSize;
	unsigned short wordBufferSize;
	unsigned short markRefX;	/* absolute pos 1 is 1st char */
	long long      markRefY;	/* absolute pos, 1 is 1st line */
	windowStruct *windowList;	/* Deleted in reverse order */
	windowStruct *activeWindow;
	fileStruct *fileList;
	bufferStruct *cutBuffer;
	bufferStruct *getBuffer;
	char *srchBuffer;
	char *argBuffer;
	char *lineBuffer;
	char *wordBuffer;
	cmdStruct *keyMap[256];	/* Simple single char mappings */
	multiCharKeyDef *multiList;
	} globalStruct;
	
/* For cmdStruct flags */
#define ILLEGAL_IN_MARK 1	/* Set when the cmd is an error with marks set */
#define IGNORE_IN_ARG   2	/* Set when a kbd sequence should be skipped in argmode */
#define ORIGINAL_ARGS   4       /* Command parses args itself */
