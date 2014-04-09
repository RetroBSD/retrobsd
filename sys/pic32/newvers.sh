#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
CV=`cat .compileversion`
CV=`expr $CV + 1`
OV=`cat .oldversion`
SVN=`svnversion`

if [ "x$SVN" = "xexported" ]
then
    PWD=`pwd`
    cd ..
    SVN=`svnversion`
    cd $PWD
fi

if [ "x$SVN" != "x$OV" ]
then
    CV=1
fi
echo $CV >.compileversion
echo $SVN >.oldversion

echo $SVN ${USER-root} `pwd` `date +'%Y-%m-%d'` `hostname` $CV| \
awk ' {
	version = $1;
    user = $2;
    dir = $3;
    date = $4;
	host = $5;
    cv = $6;
	printf "const char version[] = \"2.11 BSD Unix for PIC32, revision %s build %d:\\n", version, cv;
	printf "     Compiled %s by %s@%s:\\n", date, user, host;
	printf "     %s\\n\";\n", dir;
}'
