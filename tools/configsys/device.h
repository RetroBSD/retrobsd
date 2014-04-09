#ifndef _DEVICE_H
#define _DEVICE_H

#include <map>
#include <string>
#include "cluster.h"

using namespace std;

class device {
    public:
        map <string,cluster> options;   // Option clusters
        cluster always;                 // Always cluster

    public:
        bool load(const char *);
};

#endif
