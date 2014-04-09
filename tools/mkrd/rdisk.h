#ifndef _RDISK_H
#define _RDISK_H

#define RDISK_FS   0xB7
#define RDISK_SWAP 0xB8

struct chs {
	unsigned char head;
	struct {
		unsigned cylhigh:2;
		unsigned sector:6;
	} __attribute__((packed));
	unsigned char cyllow;
}__attribute__((packed));

struct partition {
	unsigned char status;
	struct chs start;
	unsigned char type;
	struct chs end;
	unsigned lbastart;
	unsigned lbalength;
};

struct mbr {
	unsigned char bootstrap1[218];
	unsigned short pad0000;
	unsigned char biosdrive;
	unsigned char secs;
	unsigned char mins;
	unsigned char hours;
	unsigned char bootstrap2[216];
	unsigned int sig;
	unsigned short pad0001;
	struct partition partitions[4];
	unsigned short bootsig;
}__attribute__((packed));

#endif
