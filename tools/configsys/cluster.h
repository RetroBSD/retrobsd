#ifndef _CLUSTER_H
#define _CLUSTER_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>

using namespace std;

class cluster {
    public:
        map <string,string> defines;
        vector <string> files;
        vector <string> nofiles;
        map <string,string> sets;
        vector <string> requires;
        vector <string> targets;

    public:
        void read(ifstream&);
};

#endif
