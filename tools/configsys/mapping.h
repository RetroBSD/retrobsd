#ifndef _MAPPING_H
#define _MAPPING_H

#include <map>

using namespace std;

class mapping {
    public:
        map<string,string> ports;
        map<string,int>  pins;
   
    public:
        bool load(const char *);
};

#endif
