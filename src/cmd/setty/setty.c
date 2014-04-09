#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    char *ttname = ttyname(0);
    char *tn;
    FILE *ttys;
    char buffer[100];
    char found = 0;
    int j,k;
    char delim;
    char *parts[30];
    int part;

    if(!ttname)
        return 10;
    if(strlen(ttname)<5)
        return 10;

    tn = ttname+5;

    ttys = fopen("/etc/ttys","r");
    if(!ttys)
        return 10;
    while(fgets(buffer,100,ttys))
    {
        if(!strncmp(tn,buffer,strlen(tn)))
        {
            found = 1;
            break;
        }
    }
    fclose(ttys);
    if(!found)
        return 10;

    // replace all tabs with spaces and remove CRLF
    for(j=0; j<strlen(buffer); j++)
    {
        if(buffer[j] == 9)
            buffer[j] = 32;
        if(buffer[j] == 10)
            buffer[j] = 0;
        if(buffer[j] == 13)
            buffer[j] = 0;
    }

    // reduce multiple spaces to single spaces
    for(j=0; j<strlen(buffer); j++)
    {
        if(buffer[j]==32)
        {
            while(buffer[j+1]==32)
            {
                for(k=j+1; k<strlen(buffer); k++)
                {
                    buffer[k] = buffer[k+1];
                }
            }
        }
    }

    // split string by delimiters
    delim = ' ';
    parts[0] = buffer;
    part=0;

    k = strlen(buffer);

    for(j=0; j<k; j++)
    {
        if(buffer[j] == delim)
        {
            part++;
            buffer[j] = 0;
            if(delim!=' ')
            {
                j++;
                buffer[j] = 0;
            }
            delim = ' ';
            if(buffer[j+1] == '"')
            {
                delim = '"';
                j++;
                buffer[j] = 0;
            }
            if(buffer[j+1] == '\'')
            {
                delim = '\'';
                j++;    
                buffer[j] = 0;
            }
            parts[part] = &(buffer[j+1]);
        }
    }

    printf("%s\n",parts[2]);
    return 0;
    
}
