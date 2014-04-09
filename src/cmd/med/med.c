/* Majenko Editor - a very very light-weight visual editor for
 * RetroBSD.  (c) 2012 Majenko Technologies.  Use at your own risk!
 * This is designed to use such a small footprint that it works entirely
 * from disk as much as is practical.
 */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <vmf.h>
#include <string.h>
#include <strings.h>
#include <signal.h>

WINDOW *win;

FILE *file;

int offset_line;
int cursor_line;
int cursor_column;
int screen_width;
int screen_height;
int file_lines;
char *filename;
char modified;

struct vspace space;

#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 10
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26

#define KEY_UP 256
#define KEY_DOWN 257
#define KEY_LEFT 258
#define KEY_RIGHT 259
#define KEY_F1 260
#define KEY_F2 261
#define KEY_F3 262
#define KEY_F4 263
#define KEY_F5 264
#define KEY_F6 265
#define KEY_F7 266
#define KEY_F8 267
#define KEY_F9 268
#define KEY_F10 269

void sigint()
{
    clear();
    refresh();
    endwin();
    vmclose(&space);
    printf("Aborted\n");
    exit(0);
}

int input_value(char *prompt)
{
    char temp[100];
    mvwaddstr(win,screen_height,0,"                                                                               ");
    mvwaddstr(win,screen_height,0,prompt);
    refresh();
    nocbreak();
    echo();
    wgetstr(win,temp);
    noecho();
    cbreak();
    return atoi(temp);
}

void refresh_screen()
{
    char c;
    struct vseg *seg;
    char temp[100];
    int coff = 0;
    int cpos;
    unsigned char t;

    if(cursor_column<0)
        cursor_column=0;

    cpos = cursor_column;

    if(cpos>79)
    {
        coff = cpos-79;
        cpos = 79;
    }
    for(c=0; c<screen_height; c++)
    {
        seg = vmmapseg(&space,offset_line+c);
        if(offset_line+c>=file_lines)
        {
            mvwaddstr(win,c,0,"~");
            clrtoeol();
        } else {
            if(c == cursor_line-offset_line)
            {
		memcpy(temp,seg->s_cinfo+coff,80);
		temp[79]=0;
		temp[80]=0;
		for(t=0; t<strlen(temp); t++)
			if(temp[t]=='\t')
				temp[t] = ' ';
		if(strlen(seg->s_cinfo+coff)>79)
		{
			temp[79]='>';
		}
		if(coff>0)
		{
			temp[0]='<';
		}
                mvwaddstr(win,c,0,temp);
            } else {
		memcpy(temp,seg->s_cinfo,80);
		temp[79]=0;
		temp[80]=0;
		for(t=0; t<strlen(temp); t++)
			if(temp[t]=='\t')
				temp[t] = ' ';
		if(strlen(seg->s_cinfo)>79)
		{
			temp[79] = '>';
		}
                mvwaddstr(win,c,0,temp);
            }
            clrtoeol();
        }
    }

    standout();
    mvwaddstr(win,screen_height,0,"                                                                               ");
    sprintf(temp,"Line: %d/%d %s %c",
        offset_line+cursor_line+1, file_lines, filename,
        modified==1 ? '*' : ' ');

    mvwaddstr(win,screen_height,0,temp);
    standend();
    
    mvwinch(win,cursor_line,cpos);
    refresh();
}

void delete_char_to_left()
{
    struct vseg *seg1;
    struct vseg *seg2;
    int i;
    int np;
    if(cursor_column==0)
    {
        // We are joining lines
        if(cursor_line+offset_line>0)
        {
            seg1 = vmmapseg(&space,cursor_line+offset_line);
            seg2 = vmmapseg(&space,cursor_line+offset_line-1);
            np = strlen(seg2->s_cinfo);
            strncpy(&(seg2->s_cinfo[strlen(seg2->s_cinfo)]),seg1->s_cinfo,1024-strlen(seg2->s_cinfo));
            vmmodify(seg2);
            for(i = cursor_line+offset_line; i<file_lines; i++)
            {
                seg1 = vmmapseg(&space,i);
                seg2 = vmmapseg(&space,i+1);
                strcpy(seg1->s_cinfo,seg2->s_cinfo);
                vmmodify(seg1);
            }
            file_lines--;
            cursor_column=np;
            if(cursor_line>0)
            {
                cursor_line--;
            } else {
                offset_line--;
            }
        }
        return;
    }

    seg1 = vmmapseg(&space,cursor_line+offset_line);
    for(i=cursor_column-1; i<1024; i++)
    {
        seg1->s_cinfo[i] = seg1->s_cinfo[i+1];
    }
    vmmodify(seg1);
    cursor_column--;
}

void insert_char_at_cursor(int c)
{
    int line;
    int len;
    int i;
    struct vseg *seg;

    line = offset_line + cursor_line;
    seg = vmmapseg(&space,line);
    len = strlen(seg->s_cinfo);
    for(i=len; i>cursor_column; i--)
    {
        seg->s_cinfo[i] = seg->s_cinfo[i-1];
    }
    seg->s_cinfo[len+1]=0;
    seg->s_cinfo[cursor_column] = c;
    cursor_column++;
    vmmodify(seg);
}

void save_file(char *fn)
{
    FILE *file;
    int i;
    struct vseg *seg;

    file = fopen(fn,"w");
    if(!file)
        return; 
    for(i=0; i<file_lines; i++)
    {
        seg = vmmapseg(&space,i);
        fputs(seg->s_cinfo,file);
        fputc('\n',file);
    }
    fclose(file);
}

int get_key()
{
    char escape[4];
    int key;
    int c;
    c = getch();
    if(c == '\e')
    {

        // Start an escape sequence
        escape[0] = 0;
        escape[1] = 0;
        escape[2] = 0;
        escape[3] = 0;

        escape[0] = '\e';
        escape[1] = getch();
        escape[2] = getch(); 

        if(!strcmp(escape,"\e[A"))
            return KEY_UP;

        if(!strcmp(escape,"\e[B"))
            return KEY_DOWN;

        if(!strcmp(escape,"\e[D"))
            return KEY_LEFT;

        if(!strcmp(escape,"\e[C"))
            return KEY_RIGHT;

        if(!strcmp(escape,"\eOS"))
            return KEY_F4;

    } else {
        return c;
    }
}

int main(int argc, char **argv)
{
    int status;
    int d;
    unsigned long lineno;
    struct vseg *seg;
    struct vseg *seg1;
    int i;

    signal(SIGINT,&sigint);

    cursor_column = 0;
    cursor_line = 0;
    offset_line = 0;

    screen_width = 80;
    screen_height = 21;

    if(argc!=2)
    {
        printf("Usage: med <filename>\n");
        printf("There are no options.\n");
        exit(10);
    }

    status = vminit(20);
    if(status == -1)
    {
        printf("Error: unable to lock memory\n");
        exit(10);
    }

    status = vmopen(&space,NULL);
    if(status == -1)
    {
        printf("Error: unable to lock scratch file\n");
        exit(10);
    }

    modified = 0;

    filename = argv[1];
    file = fopen(argv[1],"r");

    // If the file exists, load it into the VM system line by line - one line to a page.
    if(file)
    {
        lineno = 0;

        seg = vmmapseg(&space,lineno);
        bzero(seg->s_cinfo,1024);
        while(fgets(seg->s_cinfo,1024,file))
        {
            while(seg->s_cinfo[strlen(seg->s_cinfo)-1]=='\n')
                seg->s_cinfo[strlen(seg->s_cinfo)-1] = 0;
            vmmodify(seg);
            lineno++;
            file_lines++;
            seg = vmmapseg(&space,lineno);
            bzero(seg->s_cinfo,1024);
        }

        fclose(file);
    } else {    
        file_lines = 1;
    }

    win = initscr();
    cbreak();
    noecho();

    refresh_screen();

    while(1)
    {
        d = get_key();
        switch(d)
        {
            case CTRL_G:
                i = input_value("Goto: ");
                if(i>0)
                {
                    cursor_line=0;
                    offset_line = i-1;
                    if(offset_line>=file_lines)
                        offset_line = file_lines-1;
                    if(offset_line > screen_height/2)
                    {
                        offset_line = offset_line - (screen_height/2);
                        cursor_line = screen_height/2;
                    }
                }
                refresh_screen();
                break;
            case CTRL_H:
                delete_char_to_left();
                modified=1;
                refresh_screen();
                break;
            case KEY_UP:
                if(cursor_line>0)
                {
                    cursor_line--;
                } else {
                    if(offset_line>0)
                    {
                        offset_line--;
                    } else {
                        break;
                    }
                }
                seg = vmmapseg(&space,offset_line+cursor_line);
                if(cursor_column >= strlen(seg->s_cinfo))
                    cursor_column = strlen(seg->s_cinfo);
                refresh_screen();
                break;    
            case KEY_DOWN:
                if(cursor_line+offset_line<file_lines-1)
                {
                    if(cursor_line<screen_height-1)
                    {
                        cursor_line++;
                    } else {
                        offset_line++;
                    }
                    seg = vmmapseg(&space,offset_line+cursor_line);
                    if(cursor_column >= strlen(seg->s_cinfo))
                        cursor_column = strlen(seg->s_cinfo);
                    refresh_screen();
                }
                break;    
            case KEY_RIGHT:
                cursor_column++;
                seg = vmmapseg(&space,offset_line+cursor_line);
                if(cursor_column > strlen(seg->s_cinfo))
                {
                    if(cursor_line+offset_line<file_lines-1)
                    {
                        if(cursor_line<screen_height-1)
                        {
                            cursor_line++;
                        } else {
                            offset_line++;
                        }
                        cursor_column = 0;
                    } else {
                        cursor_column = strlen(seg->s_cinfo);
                    }
                }
                refresh_screen();
                break;
            case KEY_LEFT:
                if(cursor_column>0)
                {
                    cursor_column--;
                    refresh_screen();
                } else {
                    if(cursor_line>0)
                    {
                        cursor_line--;
                        seg = vmmapseg(&space,offset_line+cursor_line);
                        cursor_column = strlen(seg->s_cinfo);
                        refresh_screen();
                    } else {
                        if(offset_line>0)
                        {
                            offset_line--;
                            seg = vmmapseg(&space,offset_line+cursor_line);
                            cursor_column = strlen(seg->s_cinfo);
                            refresh_screen();
                        }
                    }
                }
                break;
            case CTRL_V:
            case CTRL_X:
                save_file(filename);
                modified=0;
                refresh_screen();
                break;
            case CTRL_L:
                clear();
                refresh();
                refresh_screen();
                break;
            case '\r':
            case '\n':
                // This is a tricky one.  We need to split the current line
                // at the cursor and insert the rightmost portion as a new
                // line in between the current one and the next one.  That means
                // shuffling all the lines below this one down one.

                for(i=file_lines; i>offset_line+cursor_line; i--)
                {
                    seg = vmmapseg(&space,i);
                    seg1 = vmmapseg(&space,i-1);
                    strcpy(seg->s_cinfo,seg1->s_cinfo);
                    vmmodify(seg);
                    vmmodify(seg1);
                }

                seg = vmmapseg(&space,offset_line+cursor_line);
                seg1 = vmmapseg(&space,offset_line+cursor_line+1);
                strcpy(seg1->s_cinfo,&(seg->s_cinfo[cursor_column]));
                seg->s_cinfo[cursor_column]=0;
                vmmodify(seg);
                vmmodify(seg1);
                if(cursor_line<screen_height-1)
                {
                    cursor_line++;
                } else {
                    offset_line++;
                }
                cursor_column=0;
                file_lines++;
                refresh_screen();
                break;
            case CTRL_I:
                insert_char_at_cursor(' ');
		while(cursor_column%8>0)
			insert_char_at_cursor(' ');
                modified=1;
                refresh_screen();
                break;
            default:
                if(d>=32 && d<=127)
                {
                    insert_char_at_cursor(d);
                    modified=1;
                    refresh_screen();
                }
        }
        if(d==CTRL_X) 
            break;
    }

    clear();
    refresh();
    endwin();

    vmclose(&space);

    return 0;
}
