#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "util.h"
#include "gstore.h"

using namespace std;

void gstore::read(ifstream& f)
{
    char line[1024];
    string in;

    while(f.getline(line,1023))
    {
        stringstream temp(line);
        temp >> in;
        uc(in);
        if(in.substr(0,1) == "#")
            continue;
        if(in == "END")
        {
            return;
        }

        temp >> this->values[in];
    }
    return;
}

