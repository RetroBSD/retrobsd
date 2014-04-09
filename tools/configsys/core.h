#ifndef _CORE_H
#define _CORE_H

#include <map>
#include <string>
#include "cluster.h"
#include "gstore.h"

using namespace std;

class core {
    public:
        map <string,cluster> options;   // Option clusters
        cluster always;                 // Always cluster
        gstore data;                    // data store

    public:
        bool load(const char *);
};

#endif
