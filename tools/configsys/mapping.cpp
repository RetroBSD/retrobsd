#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include "mapping.h"

using namespace std;

bool mapping::load(const char *filename)
{
    ifstream f;
    string in;
    char line[1024];

    f.open(filename);
    if(!f.is_open())
    {
        cout << "Unable to open mapping " << filename << endl;
        return false;
    }

    while(f.getline(line,1023))
    {
        stringstream temp(line);
        string silk;

        temp << line;
        temp >> silk;
        if(silk.substr(0,1)=="#")
            continue;

        temp >> this->ports[silk];
        temp >> this->pins[silk];

    }
    f.close();
    return true;
}
