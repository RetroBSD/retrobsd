#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <fcntl.h>
#include <sys/adc.h>
#include <unistd.h>


WINDOW *win;

struct channel {
    int fd;
    int value;
    char enabled;
};

int main(int argc, char **argv)
{
    struct channel channels[16];
    char temp[100];
    int i;
    long flags;
    int value;
    const char opts[] = ":d:";
    int opt;
    unsigned long delay = 100;

    for(i=0; i<16; i++)
        channels[i].enabled = 1;

    while((opt = getopt(argc, argv, opts)) != -1)
    {
        switch(opt)
        {
            case 'd':
                delay = atol(optarg);
                break;
        }
    }

    for(i=0; i<16; i++)
    {
        if(channels[i].enabled == 1)
        {
            sprintf(temp,"/dev/adc%d",i);
            channels[i].fd = open(temp,O_RDWR);
            if(!channels[i].fd)
            {
                printf("Error: unable to open %s\n",temp);
                exit(10);
            }
            fcntl(channels[i].fd,F_GETFD,&flags);
            flags |= O_NONBLOCK;
            fcntl(channels[i].fd,F_SETFD,&flags);
        }
    }

    win = initscr();

    clear();
    while(1)
    {
	clear();
        for(i=0; i<16; i++)
        {
            if(channels[i].enabled == 1)
            {
                if(read(channels[i].fd,temp,20))
                {
                    if(sscanf(temp,"%d",&value))
                    {
                        channels[i].value = value;
                    }
                }
                sprintf(temp,"adc%-2d %4d  ",i,channels[i].value);
                mvwaddstr(win,i,0,temp);
                for(value=0; value<channels[i].value>>4; value++)
                {
                    mvwaddch(win,i,value+11,'=');
                }
               // clrtoeol();
            }
        }
        refresh();
        usleep(delay * 1000);
    }
    
    for(i=0; i<16; i++)
    {
        close(channels[i].fd);
    }
    endwin();
}
