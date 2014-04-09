/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */
#include "externs.h"

#define strstr(x) x

int WEIGHT = MAXWEIGHT;
int CUMBER = MAXCUMBER;

#ifdef EXT_MESSAGE_FILE
unsigned objdes[NUMOFOBJECTS] = {
#else
char *objdes[NUMOFOBJECTS] = {
#endif
	strstr("There is a knife here"),
	strstr("There is an exquisitely crafted sword and scabbard here."),
	0,				/* can land from here */
	strstr("There is a fierce woodsman here brandishing a heavy mallet."),
	strstr("There is an unweildly two-handed sword here."),
	strstr("There is a bloody meat cleaver here."),
	strstr("A rusty broadsword is lying here."),
	strstr("There is an ancient coat of finely woven mail here."),
	strstr("There is a old dented helmet with an ostrich plume here."),
	strstr("There is a shield of some native tribe here."),
	strstr("The maid's body is lying here. She was murdered!"),
	strstr("There is a Viper ready for launch here."),
	strstr("A kerosene lantern is burning luridly here."),
	strstr("An old pair of shoes has been discarded here."),
	0,				/* cylon */
	strstr("There is a pair of pajamas here."),
	strstr("A kingly robe of royal purple and spun gold is draped here."),
	strstr("There is a strange golden amulet on the floor here."),
	strstr("A medallion of solid gold shimmers on the ground nearby."),
	strstr("A talisman of gold is lying here."),
	strstr("A dead woodsman has fallen here. He was savagely murdered."),
	strstr("A heavy wooden mallet lies nearby."),
	strstr("There is a laser pistol here."),
	strstr("A flower-like young goddess is bathing in the hot mineral pools. She is \nwatching you, but continues to steep and sing softly."),
	strstr("The goddess is reclining on a bed of ferns and studying you intently."),
	strstr("There is a grenade here"),
	strstr("There is a length of heavy chain here."),
	strstr("There is a stout rope here."),
	strstr("There is a pair of Levi's here."),
	strstr("A bloody mace is lying on the ground here."),
	strstr("There is a shovel here."),
	strstr("A long, sharp halberd is propped up here."),
	strstr("There is a compass here"),
	strstr("Wreckage and smoldering debris from a crash litter the ground here."),
	strstr("A woodland Elf armed with a shield and deadly halberd lunges toward you!"),
	strstr("I think I hear footsteps behind us."),
	strstr("There are a few coins here."),
	strstr("There are some matches here."),
	strstr("An unctuous man in a white suit and a dwarf are standing here."),
	strstr("There are some ripe papayas here."),
	strstr("There is a ripe pineapple here."),
	strstr("There are some kiwi fruit here."),
	strstr("There are some coconuts here."),
	strstr("There is a ripe mango here."),
	strstr("There is a sparkling diamond ring here."),
	strstr("There is a colorful pink potion in a small crystal vial here."),
	strstr("A gold bracelet is on the ground here."),
	strstr("A swarthy woman with stern features pulls you aside from the crowd,\n'I must talk to you -- but not here.  Meet me at midnight in the gardens.'"),
	strstr("The swarthy woman has been awaiting you anxiousy. 'I must warn you that the\nIsland has anticipated your Quest.  You will not be welcomed. The Darkness is\nstrong where you must search.  Seek not the shadows save only at night, for\nthen are they the weakest.  In the mountains far from here a canyon winds\nwith ferns and streams and forgotten vines.  There you must go. Take this\nrope.'"),
	strstr("Out from the shadows a figure leaps!  His black cape swirls around, and he\nholds a laser sword at your chest.  'So, you have come to fulfill the Quest.\nHa! Your weapons are no match for me!'"),
	strstr("An old timer with one eye missing and no money for a drink sits at the bar."),
	strstr("You are flying through an asteroid field!"),
	strstr("A planet is nearby."),
	strstr("The ground is charred here."),
	strstr("There is a thermonuclear warhead here."),
	strstr("The fragile, beautiful young goddess lies here.  You murdered her horribly."),
	strstr("The old timer is lying here.  He is dead."),
	strstr("The native girl's body is lying here."),
	strstr("A native girl is sitting here."),
	strstr("A gorgeous white stallion is standing here."),
	strstr("The keys are in the ignition."),
	strstr("A pot of pearls and jewels is sitting here."),
	strstr("A bar of solid gold is here."),
	strstr("There is a 10 kilogram diamond block here.")

};

char *objsht[NUMOFOBJECTS] = {
	"knife",
	"fine sword",
	0,
	"Woodsman",
	"two-handed sword",
	"meat cleaver",
	"broadsword",
	"coat of mail",
	"plumed helmet",
	"shield",
	"maid's body",
	"viper",
	"lantern",
	"shoes",
	0,
	"pajamas",
	"robe",
	"amulet",
	"medallion",
	"talisman",
	"woodsman's body",
	"wooden mallet",
	"laser",
	0,
	0,
	"grenade",
	"chain",
	"rope",
	"levis",
	"mace",
	"shovel",
	"halberd",
	"compass",
	0,
	"Elf",
	0,
	"coins",
	"match book",
	0,
	"papayas",
	"pineapple",
	"kiwi",
	"coconuts",
	"mango",
	"ring",
	"potion",
	"bracelet",
	0,
	0,
	"Dark Lord",
	0,
	0,
	0,
	0,
	"warhead",
	"goddess's body",
	"old timer's body",
	"girl's body",
	0,
	"stallion",
	"car",
	"pot of jewels",
	"bar of gold",
	"diamond block"
};

char *ouch[NUMOFINJURIES] = {
	"some minor abrasions",
	"some minor lacerations",
	"a minor puncture wound",
	"a minor amputation",
	"a sprained wrist",
	"a fractured ankle and shattered kneecap",
	"a broken arm and dislocated shoulder",
	"a few broken ribs",
	"a broken leg and torn ligaments",
	"a broken back and ruptured spleen",
	"some deep incisions and a loss of blood",
	"a fractured skull and mashed face",
	"a broken neck"
};

int objwt[NUMOFOBJECTS] = {
	1, 	5,	0,	10,	15,	2,	10,	10,
	3,	5,	50,	2500,	2,	1,	100,	1,
	2,	1,	1,	1,	60,	10,	5,	0,
	50,	5,	15,	5,	1,	20,	10,	10,
	0,	0,	0,	0,	1,	0,	0,	1,
	1,	1,	2,	1,	0,	0,	0,	0,
	0,	0,	100,	0,	0,	0,	55,	47,
	50,	45,	45,	100,	2000,	30,	20,	10
};

int objcumber[NUMOFOBJECTS] = {
	1, 	5,	0,	150,	10,	1,	5,	2,
	2,	1,	5,	10,	1,	1,	10,	1,
	1,	1,	1,	1,	7,	5,	4,	0,
	0,	1,	1,	1,	1,	5,	4,	4,
	1,	0,	0,	0,	1,	0,	0,	1,
	1,	1,	3,	1,	0,	0,	1,	0,
	0,	0,	10,	0,	0,	0,	7,	8,
	10,	8,	8,	10,	10,	3,	1,	2
};

int win = 1;
int matchcount = 20;
int followgod = -1;
int followfight = -1;
