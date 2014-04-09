#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ioctl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glcd.h>
#include <string.h>

int glcd;

void error(char *message)
{
    fprintf(stderr,"Error: %s: %d\n",message,errno);
}

int openDevice()
{
    struct glcd_command com;
    glcd = open("/dev/glcd0",O_RDWR);
    if(!glcd)
    {
        error("Could not open /dev/glcd0");
        return 0;
    }
    ioctl(glcd,GLCD_RESET,&com);
    return 1;
}

void cls()
{
    struct glcd_command c;
    ioctl(glcd,GLCD_CLS,&c);
}

void random_dots(int num)
{
    struct glcd_command c;
    unsigned char rfsh=0;;

    cls();

    while(num>0)
    {
        c.x1 = random()%128;
        c.y1 = random()%64;
        ioctl(glcd,GLCD_SET_PIXEL,&c);
        c.x1 = random()%128;
        c.y1 = random()%64;
        ioctl(glcd,GLCD_CLEAR_PIXEL,&c);
        rfsh++;
        if(rfsh==0)
            ioctl(glcd,GLCD_UPDATE,&c);
        num--;
    }
}

#define LNUM 20

void bouncy_lines(int num)
{
    struct glcd_command c;
    struct glcd_command history[LNUM];
    int l;
    int x1,x2,y1,y2;
    char dx1,dx2,dy1,dy2;

    dx1=1;
    dy1=1;
    dx2=-1;
    dy2=-1;

    x1 = random() % 128;
    x2 = random() % 128;
    y1 = random() % 64;
    y2 = random() % 64;

    while(num>0)
    {
        for(l=0; l<LNUM-1; l++)
        {
            history[l] = history[l+1];
        }
        history[LNUM-1].x1 = x1;
        history[LNUM-1].x2 = x2;
        history[LNUM-1].y1 = y1;
        history[LNUM-1].y2 = y2;
        history[LNUM-1].ink = 1;

        cls();
        for(l=0; l<LNUM; l++)
        {
            ioctl(glcd,GLCD_LINE,&(history[l]));
        }

        x1+=dx1;
        y1+=dy1;
        x2+=dx2;
        y2+=dy2;

        if(x1>=128)
        {
            x1=127;
            dx1=0-dx1;
        }

        if(x1<0)
        {
            x1=0;
            dx1=0-dx1;
        }

        if(x2>=128)
        {
            x2=127;
            dx2=0-dx2;
        }

        if(x2<0)
        {
            x2=0;
            dx2=0-dx2;
        }

        if(y1>=64)
        {
            y1=63;
            dy1=0-dy1;
        }

        if(y1<0)
        {
            y1=0;
            dy1=0-dy1;
        }

        if(y2>=63)
        {
            y2=63;
            dy2=0-dy2;
        }

        if(y2<0)
        {
            y2=0;
            dy2=0-dy2;
        }

        ioctl(glcd,GLCD_UPDATE,&c);
        num--;
    }
}

void stretchy_box(int num)
{
    struct glcd_command c;
    int l;
    int x1,x2,y1,y2;
    char dx1,dx2,dy1,dy2;

    dx1=1;
    dy1=1;
    dx2=-1;
    dy2=-1;

    x1 = random() % 128;
    x2 = random() % 128;
    y1 = random() % 64;
    y2 = random() % 64;

    while(num>0)
    {
        c.x1 = x1;
        c.x2 = x2;
        c.y1 = y1;
        c.y2 = y2;
        c.ink = 1;

        cls();
        ioctl(glcd,GLCD_FILLED_BOX,&c);

        x1+=dx1;
        y1+=dy1;
        x2+=dx2;
        y2+=dy2;

        if(x1>=128)
        {
            x1=127;
            dx1=0-dx1;
        }

        if(x1<0)
        {
            x1=0;
            dx1=0-dx1;
        }

        if(x2>=128)
        {
            x2=127;
            dx2=0-dx2;
        }

        if(x2<0)
        {
            x2=0;
            dx2=0-dx2;
        }

        if(y1>=64)
        {
            y1=63;
            dy1=0-dy1;
        }

        if(y1<0)
        {
            y1=0;
            dy1=0-dy1;
        }

        if(y2>=63)
        {
            y2=63;
            dy2=0-dy2;
        }

        if(y2<0)
        {
            y2=0;
            dy2=0-dy2;
        }

        ioctl(glcd,GLCD_UPDATE,&c);
        usleep(1000);
        num--;
    }
}

void banner(char *message)
{
    struct glcd_command c;
    int len = strlen(message);
    c.x1 = 64 - ((len / 2) * 6);
    c.y1 = 3;
    c.ink = 1;
    ioctl(glcd,GLCD_GOTO_XY,&c);
    write(glcd,message,len);
}

int main()
{
    int fifo;
    struct glcd_command c;
    if(!openDevice())
        return 10;

    cls();
    banner("Random Dots");
    sleep(1);
    random_dots(30000);
    sleep(1);

    cls();
    banner("Bouncy Lines");
    sleep(1);
    bouncy_lines(500);
    sleep(1); 

    cls();
    banner("Stretchy Box");
    sleep(1);
    stretchy_box(500);
    sleep(1);

    cls();
    banner("Fin.");
    sleep(3);
    cls();
    ioctl(glcd,GLCD_UPDATE,&c);

    return 0;
}
