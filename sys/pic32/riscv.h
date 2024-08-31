#pragma once

#define ST_RP           0x08000000      /* Enable reduced power mode */
#define ST_UM           0x00000010      /* User mode */

struct devspec {
	unsigned _id;
	const char *_name;
};
