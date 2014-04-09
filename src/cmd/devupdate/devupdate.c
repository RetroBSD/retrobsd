#include <stdio.h>
#include <fcntl.h>
#include <nlist.h>
#include <string.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kmem.h>

struct device {
    char *name;
    int major;
    int minor;
    char class;
    struct device *next;
};

struct nlist nl[] = { {"_bdevsw"}, {"_cdevsw"}, {""}};

struct device *devices = NULL;

struct bdevsw bdevsw;
struct cdevsw cdevsw;

struct devspec devspec;

void remove_device(struct device *entry)
{
    struct device *dev;
    struct device *last = NULL;
    struct device *next = NULL;

    if (!entry)
        return;

    next = entry->next;

    if (entry == devices) {
        free(devices);
        devices = next;
        return;
    }

    for (dev = devices; dev; dev = dev->next) {
        if (dev->next == entry) {
            last = dev;
            break;
        }
    }

    // This should never happen - we have an entry, it's not the head,
    // so there must be a previous entry.
    if (!last)
        return;

    free(entry);
    last->next = next;
}

void add_device(char *name, int major, int minor, char class)
{
    struct device *dev;

    if(!devices)
    {
        devices = malloc(sizeof(struct device));
        dev = devices;
    } else {
        for (dev=devices; dev->next; dev = dev->next);
        dev->next = malloc(sizeof(struct device));
        dev = dev->next;
    }
    dev->name = strdup(name);
    dev->major = major;
    dev->minor = minor;
    dev->class = class;
    dev->next = NULL;
}

int getstring(int fd, unsigned int offset, char *buffer)
{
    char *p = buffer;
    lseek(fd,offset,0);

    read(fd, p, 1);
    while (*p != 0) {
        p++;
        read(fd, p, 1);
    }
    return (p-buffer);
}

int getdata(int fd, unsigned int offset, void *buffer, int size)
{
    lseek(fd, offset, 0);
    return read(fd, buffer, size);
}

int getbdev(int fd, int num)
{
    getdata(fd, nl[0].n_value + (sizeof(struct bdevsw) * num), &bdevsw, sizeof(struct bdevsw));
    if (bdevsw.d_open == 0) {
        return 0;
    }
    return 1;
}

int getcdev(int fd, int num)
{
    getdata(fd, nl[1].n_value + (sizeof(struct cdevsw) * num), &cdevsw, sizeof(struct cdevsw));
    if (cdevsw.d_open == 0) {
        return 0;
    }
    return 1;
}

int getdevspec(int fd, unsigned int offset, unsigned int num)
{
    getdata(fd, offset + (sizeof(struct devspec) * num), &devspec, sizeof(struct devspec));
    if (devspec.devname == 0) {
        return 0;
    } 
    return 1;

}

struct device *get_device(char *name)
{
    struct device *dev;

    for (dev = devices; dev; dev = dev->next) {
        if (!strcmp(dev->name, name)) {
            return dev;
        }
    }
    return NULL;
}

void delete_device(char *filename)
{
    //printf("unlink(%s)\n",filename);
    unlink(filename);
}

void create_device(struct device *dev)
{
    char buffer[20];
    dev_t ds;
    mode_t mode;
    sprintf(buffer,"/dev/%s", dev->name);
    
    ds = makedev(dev->major, dev->minor);
    if (dev->class == 'c')
        mode = 0666 | S_IFCHR;
    else
        mode = 0666 | S_IFBLK;

    //printf("mknod(%s, %o, %04X)\n", buffer, mode, ds);
    mknod(buffer, mode, ds);
}

void create_kmem()
{
    dev_t kmd = kmemdev();
    struct device dev;
    dev.major = major(kmd);
    dev.minor = minor(kmd);
    dev.name = "kmem";
    dev.class = 'c';
    create_device(&dev);
}

int main()
{
    int kmem;
    char buffer[20];
    int bdevnum = 0;
    int cdevnum = 0;
    int specno = 0;
    int dnnum = 0;
    int i;
    DIR *dp;
    struct direct *file;
    struct device *dev;
    struct stat sb;
    int isgood;
    int um;
    dev_t kmd = kmemdev();

    knlist(nl);

    um = umask(0000);

    // Does kmem exist, and is it correct?
    if (stat("/dev/kmem",&sb) == -1) {
        create_kmem();
    } else if (
            (!(sb.st_mode & S_IFCHR)) || 
             (major(sb.st_rdev)!=major(kmd)) || 
             (minor(sb.st_rdev)!=minor(kmd))
    ) {
        delete_device("/dev/kmem");
        create_kmem();
    }

    kmem = open("/dev/kmem",O_RDONLY);
    if (!kmem) {
        printf("devupdate: FATAL - unable to access /dev/kmem\n");
        return(10);
    }

    //printf("Block devices: \n");
    while (getbdev(kmem,bdevnum) == 1) {
        specno = 0;
        if (bdevsw.devs != 0) {
            while (getdevspec(kmem, (unsigned int)bdevsw.devs, specno) == 1) {
                getstring(kmem, (unsigned int)devspec.devname, buffer);
                specno++;
                //printf("  %s (%d,%d)\n", buffer, bdevnum, devspec.unit);
                add_device(buffer, bdevnum, devspec.unit, 'b');
            }
        }
        bdevnum++;
    }

    //printf("Chatacter devices: \n");
    while (getcdev(kmem,cdevnum) == 1) {
        specno = 0;
        if (cdevsw.devs != 0) {
            while (getdevspec(kmem, (unsigned int)cdevsw.devs, specno) == 1) {
                getstring(kmem, (unsigned int)devspec.devname, buffer);
                specno++;
                //printf("  %s (%d,%d)\n", buffer, cdevnum, devspec.unit);
                add_device(buffer, cdevnum, devspec.unit, 'c');
            }
        }
        cdevnum++;
    }

    close(kmem);


    dp = opendir("/dev");
    
    while(file = readdir(dp))
    {
        isgood = 0;

        if (file->d_name[0] == '.') 
            continue;

        dev = get_device(file->d_name);
        sprintf(buffer,"/dev/%s",file->d_name);

        if (!dev) {
            delete_device(buffer);
            continue;
        }

        stat(buffer, &sb);

        if ((dev->class == 'b') && !(sb.st_mode & S_IFBLK)) {
            delete_device(buffer);
            create_device(dev);
            remove_device(dev);
            continue;
        }
        if ((dev->class == 'c') && !(sb.st_mode & S_IFCHR)) {
            delete_device(buffer);
            create_device(dev);
            remove_device(dev);
            continue;
        }

        if (dev->major != major(sb.st_rdev)) {
            delete_device(buffer);
            create_device(dev);
            remove_device(dev);
            continue;
        }

        if (dev->minor != minor(sb.st_rdev)) {
            delete_device(buffer);
            create_device(dev);
            remove_device(dev);
            continue;
        }

        remove_device(dev);
    }
    closedir(dp);

    for (dev = devices; dev; dev = dev->next) {
        create_device(dev);
    }

    close(kmem);
    return 0;
}
    
