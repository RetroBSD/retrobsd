#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
CV=`cat .compileversion`
CV=`expr $CV + 1`
OV=`cat .oldversion`
GITREV=`git rev-list HEAD --count`

if [ "x$GITREV" = "x" ]
then
    GITREV="Untracked"
fi

if [ "x$GITREV" != "x$OV" ]
then
    CV=1
fi
echo $CV >.compileversion
echo $GITREV >.oldversion

echo $GITREV ${USER-root} `pwd` `date +'%Y-%m-%d'` `hostname` $CV| \
awk ' {
    version = $1;
    user = $2;
    dir = $3;
    date = $4;
    host = $5;
    cv = $6;
    printf "const char version[] = \"2.11 BSD Unix for PIC32, revision G%s build %d:\\n", version, cv;
    printf "     Compiled %s by %s@%s:\\n", date, user, host;
    printf "     %s\\n\";\n", dir;
}'
