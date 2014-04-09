##############################################################################
#  The BYTE UNIX Benchmarks - Release 3
#          Module: index.sh   SID: 3.5 5/15/91 19:30:24
#          
##############################################################################
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Ben Smith or Tom Yager at BYTE Magazine
#	ben@bytepb.byte.com   tyager@byptepb.byte.com
#
##############################################################################
# generate an index from test log
# 
#############################################################################
#  Modification Log:
#        created 4/1/91 - Ben Smith
#
##############################################################################
BINDIR=${BINDIR-pgms}
BASE=${BASE-pgms/index.base}
TARGET=${TARGET-results/log}
TEMP=/tmp/$$.dat
#
# BASELINE DATA
#
if [ $# -lt 1 ]
then
	echo "Data File for baseline: \c"
	read BASE
else
	BASE=$1
fi
# check for existance
if [ ! -r ${BASE} ]
then
	echo "Cannot open $BASE for reading"
	exit 1
fi
#
# RESULTS TARGET
#
if [ $# -lt 2 ]
then
	echo "Source File for target machine results: \c"
	read TARGET
else
	TARGET=$2
fi
# check for existance
if [ ! -r ${TARGET} ]
then
	echo "Cannot open $TARGET for reading"
	exit 1
fi
#
# make dat file for results
 sort $TARGET > ${TEMP}
#
# DESTINATION
#
if [ $# -eq 3 ]
then
	DEST=$3
	join -t'|' ${BASE} ${TEMP} | awk -f ${BINDIR}/index.awk > ${DEST}
else
	join -t'|' ${BASE} ${TEMP} | awk -f ${BINDIR}/index.awk
fi

# cleanup
rm -f ${TEMP}



