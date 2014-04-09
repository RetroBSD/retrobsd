#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

using namespace std;

void replace_all(string &haystack, string from, string to)
{
    size_t pos;
    pos = haystack.find(from);
    while(pos != haystack.npos)
    {
        haystack.replace(pos, from.size(), to);
        pos = haystack.find(from);
    }
}

void uc(string &s)
{
    for(unsigned int l = 0; l < s.length(); l++)
    {
        s[l] = toupper(s[l]);
    }
}
