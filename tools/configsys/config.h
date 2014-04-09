#ifndef _CONFIG_H
#define _CONFIG_H

#include <map>
#include <string>
#include "mapping.h"
#include "instance.h"

using namespace std;

class config {
    public:
        string pinmapping;
        instance core;
        map <string,instance> instances;
        string configpath;
        string linker;
        map <string,string> options;

    public:
        bool load(const char *);
        config(string);
};

#endif
