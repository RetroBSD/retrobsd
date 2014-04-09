#ifndef _GSTORE_H
#define _GSTORE_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>

using namespace std;

class gstore {
    public:
        map <string,string> values;

    public:
        void read(ifstream&);
};

#endif
