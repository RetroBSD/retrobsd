#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "util.h"
#include "cluster.h"

using namespace std;

void cluster::read(ifstream& f)
{
    char line[1024];
    string in;

    while (f.getline(line,1023)) {
        stringstream temp(line);
        temp >> in;
        uc(in);
        if (in.substr(0,1) == "#")
            continue;
        if (in == "END") {
            return;
        }
        if (in == "DEFINE") {
            temp >> in;
            temp >> this->defines[in];
        }
        if (in == "SET") {
            temp >> in;
            temp >> this->sets[in];
        }
        if (in == "REQUIRE") {
            temp >> in;
            uc(in);
            this->requires.push_back(in);
        }
        if (in == "FILE") {
            temp >> in;
            this->files.push_back(in);
        }
        if (in == "NOFILE") {
            temp >> in;
            this->nofiles.push_back(in);
        }
        if (in == "TARGET") {
            temp >> in;
            this->targets.push_back(in);
        }
    }
    return;
}

