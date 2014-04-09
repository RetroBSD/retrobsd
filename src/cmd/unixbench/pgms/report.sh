##############################################################################
#  The BYTE UNIX Benchmarks - Release 1
#          Module: report.sh   SID: 1.4 5/15/91 19:30:26
#          
##############################################################################
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Ben Smith or Tom Yager at BYTE Magazine
#	ben@bytepb.byte.com   tyager@byptepb.byte.com
#
##############################################################################
# generate an report from test log
# 
#############################################################################
#  Modification Log:
#        created 4/1/91 - Ben Smith
#
##############################################################################
BINDIR=${BINDIR-pgms}
TARGET=${TARGET-results/log}
# RESULTS TARGET
#
if [ $# -lt 1 ]
then
	echo "Source File for target machine results: \c"
	read TARGET
else
	TARGET=$1
fi
# check for existance
if [ ! -r ${TARGET} ]
then
	echo "Cannot open $TARGET for reading"
	exit 1
fi
awk -f ${BINDIR}/report.awk $TARGET


