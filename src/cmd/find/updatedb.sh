#!/bin/sh
#
#	@(#)updatedb.csh	4.6.1	(2.11BSD)	1996/10/23
#
SRCHPATHS="/"			# directories to be put in the database
LIBDIR=/libexec			# for subprograms
FINDHONCHO=root			# for error messages
FCODES=/var/db/find.codes	# the database

PATH=$LIBDIR:/bin
bigrams=/tmp/f.bigrams$$
filelist=/tmp/f.list$$
errs=/tmp/f.errs$$

# Make a file list and compute common bigrams.
# Alphabetize '/' before any other char with 'tr'.
# If the system is very short of sort space, 'bigram' can be made
# smarter to accumulate common bigrams directly without sorting
# ('awk', with its associative memory capacity, can do this in several
# lines, but is too slow, and runs out of string space on small machines).

nice +10
find ${SRCHPATHS} -print | tr '/' '\001' | \
   (sort -f; echo $status > $errs) | \
   tr '\001' '/' > $filelist
$LIBDIR/bigram < $filelist | \
   (sort; echo $status >> $errs) | uniq -c | sort -nr | \
   awk '{ if (NR <= 128) print $2 }' | tr -d '\012' > $bigrams

# code the file list

if grep -s -v 0 $errs
then
	echo 'squeeze error: out of sort space' | mail $FINDHONCHO
else
	$LIBDIR/code $bigrams < $filelist > $FCODES
	chmod 644 $FCODES
	rm $bigrams $filelist $errs
fi
