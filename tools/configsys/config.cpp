#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "util.h"
#include "mapping.h"
#include "device.h"
#include "instance.h"
#include "config.h"

using namespace std;

extern map <string,device> devices;
extern map <string,device> cores;
extern map <string,mapping> mappings;

config::config(string path)
{
    this->configpath = path;
}

string get_parsed_line(ifstream &f)
{
    stringstream line;
    char in[1024];
    string token;
    stringstream s;

    s.str("");

    if(f.getline(in,1023))
    {
        line.str(in);
        while(line >> token)
        {
            if(token == "\\")
            {
                f.getline(in,1023);
                line.str(in);
                continue;
            }
            s << token << " ";
        }
    }
    return s.str();
}

bool config::load(const char *filename)
{
    ifstream f;
    string temp;
    size_t pos;
    string devid = "";

    map <string,device>::iterator it;

    f.open(filename,ios::in);
    if(!f.is_open())
    {
        cout << "Unable to open " << filename << endl;
        return false;
    }
    while(!f.eof())
    {
        string command;

        temp = get_parsed_line(f);
        stringstream in(temp);

        in >> command;
        uc(command);

        if(command.substr(0,1)=="#")
        {
            continue;
        }

        if(command == "MAPPING")
        {
            in >> this->pinmapping;
            uc(this->pinmapping);
            continue;
        }

        if(command == "DEVICE")
        {
            string devname;
            int devnum;
            string devid;
            in >> devid;
            uc(devid);
            pos = devid.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUFWXYZ");
            if(pos == devid.npos)
            {
                devname = devid;
                devnum = 0;
            } else {
                devname = devid.substr(0,pos);
                devnum = atoi(devid.substr(pos,devid.size()).c_str());
            }

            this->instances[devid].unit = devnum;
            this->instances[devid].device = devname;

            while(in >> temp)
            {
                pos = temp.find("=");
                if(pos>0)
                {
                    string l,r;
                    l = temp.substr(0,pos);
                    uc(l);
                    r = temp.substr(pos+1,temp.size());
                    this->instances[devid].settings[l] = r;
                }
            }
            continue;
        }

        if(command == "CORE")
        {
            string devname;
            in >> devname;
            uc(devname);

            if(cores.find(devname) == cores.end())
            {
                cout << "Unknown core: " << devname << endl;
                f.close();
                return false;
            }

            this->core.device = devname;
            this->core.unit = 0;

            while(in >> temp)
            {
                pos = temp.find("=");
                if(pos>0)
                {
                    this->core.settings[temp.substr(0,pos)] = temp.substr(pos,temp.size());
                }
            }
            continue;
        }

        if(command == "LINKER")
        {
            in >> this->linker;
            continue;
        }

        if(command == "OPTION")
        {
            in >> temp;
            pos = temp.find("=");
            if(pos>0)
            {
                string l,r;
                l = temp.substr(0,pos);
                uc(l);
                r = temp.substr(pos+1,temp.size());
                this->instances["GLOBAL"].settings[l] = r;
                this->instances["GLOBAL"].device="GLOBAL";
                this->instances["GLOBAL"].unit=0;
            }
            continue;
        }
    }
    f.close();
    return true;
}

