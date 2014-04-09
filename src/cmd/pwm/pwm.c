#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ioctl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <oc.h>

int oc;

void error(char *message)
{
    fprintf(stderr,"Error: %s: %d\n",message,errno);
}

int openDevice(int dev)
{
    char temp[20];
    int mode=OC_MODE_PWM;

    sprintf(temp,"/dev/oc%d",dev);
    oc = open(temp,O_RDWR);
    if(!oc)
    {
        error("Could not open PWM device");
        return 0;
    }
    if(ioctl(oc,OC_SET_MODE,&mode)==-1)
    {   
        error("Could not switch channel to PWM mode");
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int unit;
    unsigned int duty;
    long tduty;

    if(argc!=3)
    {
        error("Specify a channel and a duty cycle");
        exit(10);
    }

    unit = atoi(argv[1]);
    tduty = atol(argv[2]);
    duty = (unsigned long)tduty;

    if((unit<0) || (unit>4))
    {   
        error("Invalid channel specified");
        exit(10);
    }


    if(openDevice(unit)==0)
        exit(10);


    if(ioctl(oc,OC_PWM_DUTY,&duty)==-1)
    {
        error("Could not set duty");
        exit(10);
    }

    return 0;
}
