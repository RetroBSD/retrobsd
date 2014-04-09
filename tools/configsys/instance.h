#ifndef _INSTANCE_H
#define _INSTANCE_H

#include <map>
#include <string>
#include "cluster.h"

using namespace std;

class instance {
    public:
        map <string,string> settings;   // Settings from config
        int unit;
        string device;
};

#endif
