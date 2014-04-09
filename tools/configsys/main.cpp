#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>


#include "util.h"
#include "config.h"
#include "device.h"
#include "core.h"

using namespace std;

map <string,device> devices;
map <string,core> cores;
map <string,mapping> mappings;

void substitute(string &out, string source, string param, int devnum, string pinmap)
{
    size_t pos;
    stringstream dnum;
    dnum << devnum;

    out = source;

    replace_all(out, "%0", dnum.str());
    replace_all(out, "%1", param);

    pos = out.find("$TRIS(");
    while(pos != out.npos)
    {
        int end;
        string content;
        end = out.find(")",pos);
        content = out.substr(pos+6,(end-pos)-6);
        out.replace(pos,pos-end,"TRIS"+mappings[pinmap].ports[content]);
        pos = out.find("$TRIS(");
    }

    pos = out.find("$PIN(");
    while(pos != out.npos)
    {
        int end;
        string content;
        stringstream pno;
        end = out.find(")",pos);
        content = out.substr(pos+5,(end-pos)-5);
        pno << mappings[pinmap].pins[content];
        out.replace(pos,pos-end,pno.str());
        pos = out.find("$PIN(");
    }
}

int main(int argc, char *argv[])
{
    string path(argv[0]);
    string root,machine,cfg;
    string corep;
    DIR *d;
    struct dirent *dent;
    map <string,string>::iterator dit;
    map <string,string>::iterator sit;
    map <string,cluster>::iterator cit;
    int q __attribute__((unused));

    if(argc!=2)
    {
        cout << "Usage: " << argv[0] << " <config>" << endl;
        return 10;
    }

    path = path.substr(0,path.rfind("/"));
    root = path + "/../..";
    corep = path + "/cores";

    d = opendir(corep.c_str());
    if(!d)
    {
        cout << "Unable to locate cores directory!" << endl;
        return 10;
    }

    while((dent = readdir(d)))
    {
        string filepath(corep + "/" + dent->d_name);
        string filename(dent->d_name);

        if(filepath.substr(filepath.size()-4,4) == ".cor")
        {
            string devname = filename.substr(0,filename.size()-4);
            uc(devname);
            if(!cores[devname].load(filepath.c_str()))
            {
                cout << "Unable to parse core " << devname << endl;
                return 10;
            }
        }
    }
    closedir(d);


    config config(cfg);

    if(!config.load(argv[1]))
    {
        cout << "Config load failed" << endl;
        return 10;
    }

    machine = root + "/sys/" + cores[config.core.device].data.values["ROOT"];
    cfg = machine + "/cfg";

    cout << "Machine: " << machine << endl;
    cout << "Config:  " << cfg << endl;

    d = opendir(cfg.c_str());
    if(!d)
    {
        cout << "Unable to locate config directory!" << endl;
        return 10;
    }

    while((dent = readdir(d)))
    {
        string filepath(cfg + "/" + dent->d_name);
        string filename(dent->d_name);

        if(filepath.substr(filepath.size()-4,4) == ".dev")
        {
            string devname = filename.substr(0,filename.size()-4);
            uc(devname);
            if(!devices[devname].load(filepath.c_str()))
            {
                cout << "Unable to parse device " << devname << endl;
                return 10;
            }
        }

        if(filepath.substr(filepath.size()-4,4) == ".map")
        {
            string devname = filename.substr(0,filename.size()-4);
            uc(devname);
            if(!mappings[devname].load(filepath.c_str()))
            {
                cout << "Unable to parse mapping " << devname << endl;
                return 10;
            }
        }
    }
    closedir(d);


    // At this point we should now have the configuration loaded in the config object, and all the
    // device descriptors etc loaded into their respective maps.
    // Now to link the two together to create an output.

    // First we will iterate the device instances and collate any requred sub-devices.
    // From that we will add new device instances, and repeat until there are no more changes.

    bool repeat = true;
    map <string,instance>::iterator it;
    vector <string>::iterator rit;

    if(cores.find(config.core.device)==cores.end())
    {
        cout << "Unknown core: " << config.core.device << endl;
        return 10;
    }

    for(rit = cores[config.core.device].always.requires.begin();
        rit != cores[config.core.device].always.requires.end();
        rit++)
    {
        map <string,instance>::iterator iit;
        bool exist = false;
        for(iit = config.instances.begin(); iit != config.instances.end(); iit++)
        {
            if((*iit).second.device == *rit)
            {
                exist = true;
            }
        }
        if(!exist)
        {
            config.instances[*rit].device=*rit;
            config.instances[*rit].unit=0;
        }
    }

    while(repeat)
    {
        repeat = false;
        for(it = config.instances.begin(); it != config.instances.end(); it++)
        {
            for(
                rit = devices[(*it).second.device].always.requires.begin();
                rit != devices[(*it).second.device].always.requires.end();
                rit++)
            {
                map <string,instance>::iterator iit;
                bool exist = false;
                for(iit = config.instances.begin(); iit != config.instances.end(); iit++)
                {
                    if((*iit).second.device == *rit)
                    {
                        exist = true;
                    }
                }
                if(!exist)
                {
                    config.instances[*rit].device=*rit;
                    config.instances[*rit].unit=0;
                    repeat=true;
                }
            }
        }
    }

    // Ok, we now have instances for all devices, and all their dependencies, and all
    // their dependencies dependencies ... etc ...

    // Now we need to generate our lists.
    // Let's start with files.  Work through the list of instances, and get the files
    // from their devices.  Compile them all into one big vector in
    // alphabetical order.

    vector <string> files;

    // First the core:

    vector <string>::iterator fit;
    vector <string>::iterator eit;
    vector <string>::iterator vit;

    for(
        fit = cores[config.core.device].always.files.begin();
        fit != cores[config.core.device].always.files.end();
        fit++
    )
    {
        bool exist = false;
        for(eit = files.begin(); eit != files.end(); eit++)
        {
            if(*eit == *fit)
                exist = true;
        }
        if(!exist)
        {
            files.push_back(*fit);
        }
    }


    // Then the instances:

    for(it = config.instances.begin(); it != config.instances.end(); it++)
    {
        for(
            fit = devices[(*it).second.device].always.files.begin();
            fit != devices[(*it).second.device].always.files.end();
            fit++
        )
        {
            bool exist = false;
            for(eit = files.begin(); eit != files.end(); eit++)
            {
                if(*eit == *fit)
                    exist = true;
            }
            if(!exist)
            {
                files.push_back(*fit);
            }
        }
    }

    vector <string> nofiles;

    // First the core:


    for(
        fit = cores[config.core.device].always.nofiles.begin();
        fit != cores[config.core.device].always.nofiles.end();
        fit++
    )
    {
        bool exist = false;
        for(eit = nofiles.begin(); eit != nofiles.end(); eit++)
        {
            if(*eit == *fit)
                exist = true;
        }
        if(!exist)
        {
            nofiles.push_back(*fit);
        }
    }


    // Then the instances:

    for(it = config.instances.begin(); it != config.instances.end(); it++)
    {
        for(
            fit = devices[(*it).second.device].always.nofiles.begin();
            fit != devices[(*it).second.device].always.nofiles.end();
            fit++
        )
        {
            bool exist = false;
            for(eit = nofiles.begin(); eit != nofiles.end(); eit++)
            {
                if(*eit == *fit)
                    exist = true;
            }
            if(!exist)
            {
                nofiles.push_back(*fit);
            }
        }
    }

    // We have the files, now to sort them alphabetically.  A good old
    // bubble sort is what we will do.  We could have done this while
    // adding the files to the vector, but this is simpler to manage.

    unsigned int outer,inner;

    for(outer = 0; outer < files.size(); outer++)
    {
        for(inner = outer; inner < files.size(); inner++)
        {
            if(files[outer] > files[inner])
            {
                string t = files[outer];
                files[outer] = files[inner];
                files[inner] = t;
            }
        }
    }

    // Now we should filter out the files to remove any nofiles.  Again, this
    // could have been integrated with the previous stages, but meh...

    for (fit = files.begin(); fit != files.end(); fit++) {
        bool exist = false;
        for (eit = nofiles.begin(); eit != nofiles.end(); eit++) {
            if(*eit == *fit)
                exist = true;
        }
        if (exist) {
            files.erase(fit);
        }
    }

    // Now to collate the extra targets that should be built.

    vector <string> targets;

    // First the core:

    for (
        fit = cores[config.core.device].always.targets.begin();
        fit != cores[config.core.device].always.targets.end();
        fit++
    ) {
        bool exist = false;
        for (eit = targets.begin(); eit != targets.end(); eit++) {
            if(*eit == *fit)
                exist = true;
        }
        if (!exist) {
            targets.push_back(*fit);
        }
    }

    // Then the instances:

    for (it = config.instances.begin(); it != config.instances.end(); it++) {
        for (
            fit = devices[(*it).second.device].always.targets.begin();
            fit != devices[(*it).second.device].always.targets.end();
            fit++
        ) {
            bool exist = false;
            for (eit = targets.begin(); eit != targets.end(); eit++) {
                if(*eit == *fit)
                    exist = true;
            }
            if (!exist) {
                targets.push_back(*fit);
            }
        }
    }

    // Next let's do the defines.  We need to do this for the "always"
    // and also for every instance.  We also need to do string replacements
    // using the data from the instances.

    map <string,string> defines;

    // Again, core first:

    for(
        dit = cores[config.core.device].always.defines.begin();
        dit != cores[config.core.device].always.defines.end();
        dit++
    )
    {
        string f,s;
        substitute(f, (*dit).first, "", 0, config.pinmapping);
        substitute(s, (*dit).second, "", 0, config.pinmapping);
        defines[f] = s;
    }
    for(
        sit = config.core.settings.begin();
        sit != config.core.settings.end();
        sit++
    )
    {
        string testopt = (*sit).first + "=" + (*sit).second;
        uc(testopt);
        if(cores[config.core.device].options.find(testopt) == cores[config.core.device].options.end())
        {
            testopt = (*sit).first;
            uc(testopt);
            if(cores[config.core.device].options.find(testopt) == cores[config.core.device].options.end())
            {
                cout << "Unknown option: " << testopt << endl;
                return 10;
            }
        }
        for(
            dit = cores[config.core.device].options[testopt].defines.begin();
            dit != cores[config.core.device].options[testopt].defines.end();
            dit++
        )
        {
            string f,s;
            substitute(f, (*dit).first, (*sit).second, 0, config.pinmapping);
            substitute(s, (*dit).second, (*sit).second, 0, config.pinmapping);
            defines[f] = s;
        }
    }

    // followed by the instances:

    for(it = config.instances.begin(); it != config.instances.end(); it++)
    {
        for(
            dit = devices[(*it).second.device].always.defines.begin();
            dit != devices[(*it).second.device].always.defines.end();
            dit++
        )
        {
            string f,s;
            substitute(f, (*dit).first, "", (*it).second.unit, config.pinmapping);
            substitute(s, (*dit).second, "", (*it).second.unit, config.pinmapping);
            defines[f] = s;
        }
        for(
            sit = (*it).second.settings.begin();
            sit != (*it).second.settings.end();
            sit++
        )
        {
            string testopt = (*sit).first + "=" + (*sit).second;
            uc(testopt);
            if(devices[(*it).second.device].options.find(testopt) == devices[(*it).second.device].options.end())
            {
                testopt = (*sit).first;
                uc(testopt);
                if(devices[(*it).second.device].options.find(testopt) == devices[(*it).second.device].options.end())
                {
                    cout << "Unknown option: " << testopt << "=" << (*sit).second<< endl;
                    return 10;
                }
            }
            for(
                dit = devices[(*it).second.device].options[testopt].defines.begin();
                dit != devices[(*it).second.device].options[testopt].defines.end();
                dit++
            )
            {
                string f,s;
                substitute(f, (*dit).first, (*sit).second, (*it).second.unit, config.pinmapping);
                substitute(s, (*dit).second, (*sit).second, (*it).second.unit, config.pinmapping);
                defines[f] = s;
            }
        }
    }

    // Wow... that was fun.  Now we need to do it all again for the SETs.

    map <string,string> sets;

    for(
        dit = cores[config.core.device].always.sets.begin();
        dit != cores[config.core.device].always.sets.end();
        dit++
    )
    {
        string f,s;
        substitute(f, (*dit).first, "", 0, config.pinmapping);
        substitute(s, (*dit).second, "", 0, config.pinmapping);
        sets[f] = s;
    }
    for(
        sit = config.core.settings.begin();
        sit != config.core.settings.end();
        sit++
    )
    {
        if(cores[config.core.device].options.find((*sit).first) == cores[config.core.device].options.end())
        {
            cout << "Unknown option: " << (*sit).first << endl;
            return 10;
        }
        for(
            dit = cores[config.core.device].options[(*sit).first].sets.begin();
            dit != cores[config.core.device].options[(*sit).first].sets.end();
            dit++
        )
        {
            string f,s;
            substitute(f, (*dit).first, (*sit).second, 0, config.pinmapping);
            substitute(s, (*dit).second, (*sit).second, 0, config.pinmapping);
            sets[f] = s;
        }
    }


    for(it = config.instances.begin(); it != config.instances.end(); it++)
    {
        for(
            dit = devices[(*it).second.device].always.sets.begin();
            dit != devices[(*it).second.device].always.sets.end();
            dit++
        )
        {
            string f,s;
            substitute(f, (*dit).first, "", (*it).second.unit, config.pinmapping);
            substitute(s, (*dit).second, "", (*it).second.unit, config.pinmapping);
            sets[f] = s;
        }
        for(
            sit = (*it).second.settings.begin();
            sit != (*it).second.settings.end();
            sit++
        )
        {
            string testopt = (*sit).first + "=" + (*sit).second;
            uc(testopt);
            if(devices[(*it).second.device].options.find(testopt) == devices[(*it).second.device].options.end())
            {
                testopt = (*sit).first;
                uc(testopt);
                if(devices[(*it).second.device].options.find(testopt) == devices[(*it).second.device].options.end())
                {
                    cout << "Unknown option " << testopt << endl;
                    return 10;
                }
            }
            for(
                dit = devices[(*it).second.device].options[testopt].sets.begin();
                dit != devices[(*it).second.device].options[testopt].sets.end();
                dit++
            )
            {
                string f,s;
                substitute(f, (*dit).first, (*sit).second, (*it).second.unit, config.pinmapping);
                substitute(s, (*dit).second, (*sit).second, (*it).second.unit, config.pinmapping);
                sets[f] = s;
            }
        }
    }

    // Right, we might have it all ready for outputting now.  Let's give it a go...

    ofstream out;

    out.open("Makefile",ios::out);

    out << "BUILDPATH = " << machine << endl;
    out << "H         = " << root << "/sys/include" << endl;
    out << "M         = " << machine << endl;
    out << "S         = " << root << "/sys/kernel" << endl;
    out << endl;
    out << "vpath %.c $(M):$(S)" << endl;
    out << "vpath %.S $(M):$(S)" << endl;
    out << endl;
    out << "KERNOBJ += ";


    for(vit = files.begin(); vit != files.end(); vit++)
    {
        out << (*vit) << " ";
    }
    out << endl;
    out << "EXTRA_TARGETS = ";

    for(vit = targets.begin(); vit != targets.end(); vit++)
    {
        out << (*vit) << " ";
    }

    out << endl;
    out << endl;

    for(sit = defines.begin(); sit != defines.end(); sit++)
    {
        if((*sit).second != "")
        {
            out << "DEFS += -D" << (*sit).first << "=" << (*sit).second << endl;
        } else {
            out << "DEFS += -D" << (*sit).first << endl;
        }
    }

    out << endl;

    for(sit = sets.begin(); sit != sets.end(); sit++)
    {
        out << (*sit).first << " = " << (*sit).second << endl;
    }

    out << endl;

    out << "LDSCRIPT = " << machine << "/cfg/"<< config.linker << ".ld" << endl;

    out << endl;
    out << "CONFIG = " << argv[1] << endl;
    out << "CONFIGPATH = " << path << endl;

    out << endl;

    out << "include " << machine << "/kernel-post.mk" << endl;
    out.close();

    q = system("make clean");

    return 0;
}
