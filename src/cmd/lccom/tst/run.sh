#!/bin/sh
# run .../tst/target/foo.s

#set -x
target=`echo $1 | awk -F/ '{ print $(NF-1) }'`

case "$1" in
*symbolic)	idir=tst/mips-eb ;;
*symbolic64)	idir=tst/alpha-osf ;;
*)		idir=tst/$target ;;
esac

if [ ! -d "tst/$target" -o ! -d "$idir" ]; then
	echo 2>&1 $0: unknown combination '"'$target'"'
	echo target = $target
	echo idir = $idir
	exit 1
fi

C=`basename $1 .s`
BUILDDIR=${BUILDDIR-.}
LCC="${LCC-${BUILDDIR}/lcc} -Wo-lccdir=$BUILDDIR"
TSTDIR=${TSTDIR-tst/$target}
if [ ! -d $TSTDIR ]; then mkdir -p $TSTDIR; fi

echo ${BUILDDIR}/rcc$EXE -target=$target $1: 1>&2
$LCC -S -I$idir -Ualpha -Usun -Uvax -Umips -Ux86 \
	-Wf-errout=$TSTDIR/$C.2 -Wf-g0 \
	-Wf-target=$target -o $1 tst/$C.c
if [ -r tst/$target/$C.2bk ]; then
	diff tst/$target/$C.2bk $TSTDIR/$C.2
fi
if [ -r tst/$target/$C.sbk ]; then
	diff tst/$target/$C.sbk $TSTDIR/$C.s
fi

exit 0
