#
# Makefile for lccom
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS		= -O0 -g -DCROSS

CFLAGS		+= -Wall -Werror -DNDEBUG
LDFLAGS		+= -g

OBJS            = $Balloc.o $Bbind.o $Bdag.o $Bdagcheck.o $Bdecl.o $Benode.o \
		  $Berror.o $Bexpr.o $Bevent.o $Binit.o $Binits.o $Binput.o \
		  $Blex.o $Blist.o $Boutput.o $Bprof.o $Bprofio.o \
		  $Bsimp.o $Bstmt.o $Bstring.o $Bsym.o $Btrace.o $Btree.o \
		  $Btypes.o $Bnull.o $Bsymbolic.o $Bgen.o $Bbytecode.o \
		  $Bmain.o $Bmips.o

TARGET          = TARGET_MIPS
BUILDDIR        = build
TSTDIR          = tst/mips-eb
B               = $(BUILDDIR)/
T               = $(TSTDIR)/

all:            build $Blburg lccom liblcc.a

clean:
		rm -rf lccom *.a *.dis *~ $B $T/*.1 $T/*.2 $T/*.s
		rm -f $T8q $Tarray $Tcf $Tcq $Tcvt $Tfields $Tfront $Tincr \
                    $Tinit $Tlimits $Tparanoia $Tsort $Tspill $Tstdarg \
                    $Tstruct $Tswitch $Twf1 $Tyacc

install:        all
		install lccom $(DESTDIR)/libexec/

build:
		mkdir -p $(BUILDDIR)

#
# lccom: the C compiler
#
lccom:          ${OBJS}
		${CC} ${LDFLAGS} -o lccom.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S lccom.elf > lccom.dis
		${SIZE} lccom.elf
		${ELF2AOUT} lccom.elf $@ && rm lccom.elf

$(OBJS):	c.h ops.h token.h config.h

$Balloc.o:	alloc.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bbind.o:	bind.c;		$(CC) $(CFLAGS) -D$(TARGET) -c -I. -o $@ $<
$Bdag.o:	dag.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bdecl.o:	decl.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Benode.o:	enode.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Berror.o:	error.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bevent.o:	event.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bexpr.o:	expr.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bgen.o:	gen.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Binit.o:	init.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Binits.o:	inits.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Binput.o:	input.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Blex.o:	lex.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Blist.o:	list.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bmain.o:	main.c;		$(CC) $(CFLAGS) -DVERSION=\"`svnversion`\" -c -I. -o $@ $<
$Bnull.o:	null.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Boutput.o:	output.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bprof.o:	prof.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bprofio.o:	profio.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bsimp.o:	simp.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bstmt.o:	stmt.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bstring.o:	string.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bsym.o:	sym.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bsymbolic.o:	symbolic.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bbytecode.o:	bytecode.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Btrace.o:	trace.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Btree.o:	tree.c;		$(CC) $(CFLAGS) -c -I. -o $@ $<
$Btypes.o:	types.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bstab.o:	stab.c stab.h;	$(CC) $(CFLAGS) -c -I. -o $@ $<

$Bdagcheck.o:	$Bdagcheck.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Balpha.o:	$Balpha.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bmips.o:	$Bmips.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bsparc.o:	$Bsparc.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bx86.o:	$Bx86.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<
$Bx86linux.o:	$Bx86linux.c;	$(CC) $(CFLAGS) -c -I. -o $@ $<

$Bdagcheck.c:	$Blburg dagcheck.md; $Blburg -n dagcheck.md $@
$Balpha.c:	$Blburg alpha.md;    $Blburg alpha.md    $@
$Bmips.c:	$Blburg mips.md;     $Blburg mips.md     $@
$Bsparc.c:	$Blburg sparc.md;    $Blburg sparc.md    $@
$Bx86.c:	$Blburg x86.md;      $Blburg x86.md      $@
$Bx86linux.c:	$Blburg x86linux.md; $Blburg x86linux.md $@

#
# lburg: code generator generator
#
$Blburg:	$Blburg.o $Bgram.o
		cc -g -o $@ $Blburg.o $Bgram.o

$Blburg.o $Bgram.o: lburg/lburg.h

$Blburg.o:	lburg/lburg.c;	cc -g -Wall -O -c -Ilburg -o $@ $<
$Bgram.o:	lburg/gram.c;	cc -g -Wall -O -c -Ilburg -o $@ $<

#
# liblcc: run time library
#
LIBOBJS         = $Bassert.o $Bbbexit.o $Byynull.o

liblcc.a:	$(LIBOBJS)
		$(AR) ruv $@ $Bassert.o $Bbbexit.o $Byynull.o; $(RANLIB) $@ || true

$Bassert.o:	lib/assert.c;	$(CC) $(CFLAGS) -c -o $@ $<
$Byynull.o:	lib/yynull.c;	$(CC) $(CFLAGS) -c -o $@ $<
$Bbbexit.o:	lib/bbexit.c;	$(CC) $(CFLAGS) -c -o $@ $<

#
# Tests
#
test:           $T8q.s $Tarray.s $Tcf.s $Tcq.s $Tcvt.s $Tfields.s $Tfront.s \
		$Tincr.s $Tinit.s $Tlimits.s $Tparanoia.s $Tsort.s \
		$Tspill.s $Tstdarg.s $Tstruct.s $Tswitch.s $Twf1.s $Tyacc.s

$T8q.s:		tst/8q.c tst/8q.0 all;		@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tarray.s:	tst/array.c tst/array.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tcf.s:		tst/cf.c tst/cf.0 all;		@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tcq.s:		tst/cq.c tst/cq.0 all;		@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tcvt.s:	tst/cvt.c tst/cvt.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tfields.s:	tst/fields.c tst/fields.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tfront.s:	tst/front.c tst/front.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tincr.s:	tst/incr.c tst/incr.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tinit.s:	tst/init.c tst/init.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tlimits.s:	tst/limits.c tst/limits.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tparanoia.s:	tst/paranoia.c tst/paranoia.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tsort.s:	tst/sort.c tst/sort.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tspill.s:	tst/spill.c tst/spill.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tstdarg.s:	tst/stdarg.c tst/stdarg.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tstruct.s:	tst/struct.c tst/struct.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tswitch.s:	tst/switch.c tst/switch.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Twf1.s:	tst/wf1.c tst/wf1.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
$Tyacc.s:	tst/yacc.c tst/yacc.0 all;	@env BUILDDIR=$(BUILDDIR) TSTDIR=$(TSTDIR) tst/run.sh $@
