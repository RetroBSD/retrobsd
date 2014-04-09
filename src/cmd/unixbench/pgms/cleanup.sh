#!/bin/sh
###############################################################################
##
## UnixBench
## pgms/cleanup.sh
##
## Originally based on:
#  The BYTE UNIX Benchmarks - Release 3
#          Module: cleanup.sh   SID: 3.5 5/15/91 19:30:26
#          
###############################################################################
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Ben Smith or Rick Grehan at BYTE Magazine
#	ben@bytepb.UUCP    rick_g@bytepb.UUCP
#
###############################################################################
#  Modification Log:
#  added report for dhrystones 6/89 - ben
#
###############################################################################
ID="@(#)cleanup.sh:3.5 -- 5/15/91 19:30:26";
#
# $Header: cleanup,v 5.2 88/01/07 10:58:24 kenj Exp $
#
#  Cleanup when an iterative test terminates
#

BINDIR=${BINDIR-./pgms}
log=${LOG-./results/log}
timeaccum=${TIMEACCUM-./results/times}
flavor="${FLAVOR-SysV}"
while ( test $# -ge 1 )
do
    opt=$1
    shift
    case $opt
      in

    -a) # abort
	echo '' >>$LOGFILE
	echo '**************************' >>$LOGFILE
	echo '* Benchmark Aborted .... *' >>$LOGFILE
	echo '**************************' >>$LOGFILE
	echo
	echo 'Benchmark Aborted ....'    # notice displayed on screen
	echo "" >>$LOGFILE
	echo " " `who | wc -l` "interactive users." >>$LOGFILE
	echo "" >>$LOGFILE
	date=`date`
	echo "End Benchmark Run ($date) ...." >>$LOGFILE
	echo "End Benchmark Run ($date) ...."
	;;


    -f) # filesystem throughput
	awk -f ${BINDIR}/fs.awk <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;

    -d) # Dhrystone evaluation
	awk -f ${BINDIR}/dhry.awk <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;

    -t) # self-timing benchmarks in loops per second
	awk -f ${BINDIR}/lps.awk  <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;
    
    -w) # MWIPS, precalculated and pretimed
	awk -f ${BINDIR}/mwips.awk  <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;
    
    -l) # loops per second for a specified time evaluation
	awk -f ${BINDIR}/loops.awk  <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;
    
    
    -m) # loops per minute for a specified time evaluation
	awk -f ${BINDIR}/loopm.awk  <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;
    
    -i) # report last iteration
	echo "Terminated during iteration $1" >>$LOGFILE
	shift
	;;

    -L) # logfile
	LOGFILE=$1
	shift
	;;
    
    -r) # reason for failure
	echo $1
	echo $1 >>$LOGFILE
	shift
	;;

    -m) # mem throughput tests
	awk -f ${BINDIR}/mem.awk <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;

    -t) # timing with /bin/time
	awk -f ${BINDIR}/time.awk <$1 >>$LOGFILE
	cat $1 >> $timeaccum 2>/dev/null
	rm -f $1
	shift
	;;


    '') # 'skip it (residual effect of shifts)'
	;;

    *)
	echo "cleanup: bad option ($opt)" >>$LOGFILE
esac
done
exit
