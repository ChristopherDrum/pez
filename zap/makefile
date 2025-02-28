# Makefile for zap
# Build mkcomptime.c with `cosmocc -o mkcomptime mkcomptime.c -mtiny`
# This executable is custom to this project and
# is used by the build process to generate another build script

CFLAGS =	-g
ZAPOBJS =	zap.o zasm.o zdir.o zop.o zlabel.o zeval.o zfixup.o zerror.o zlex.o\
		zsymtab.o zdata.o zobj.o zstring.o zsymout.o ztable.o zsegment.o\
		ALLOC.o resfile.o zapres.o
ZAPSRCS =	zap.c zasm.c zdir.c zop.c zlabel.c zeval.c zfixup.c zerror.c zlex.c\
		zsymtab.c zdata.c zobj.c zstring.c zsymout.c ztable.c zsegment.c\
		ALLOC.c resfile.c zapres.c
ALL=		zap zsplit zglue

all:		$(ALL)

comptime.o:	comptime.c
	./getdate

zap:		$(ZAPOBJS)
	make comptime.o; cc -o zap $(ZAPOBJS) comptime.o; rm comptime.o mkctscr

zsymtest:	zsymtest.o
	cc $(CFLAGS) -o zsymtest zsymtest.o

zglue.o:	zglue.c splitdefs.h

zglue:	zglue.o
	make comptime.o; cc $(CFLAGS) -o zglue zglue.o comptime.o; rm comptime.o mkctscr

zspix.o:	zspix.c splitdefs.h zsplitext.h splitpic.h

zsplit.o:	zsplit.c splitdefs.h zsplitext.h

zsplit:		zsplit.o zspix.o zsres.o resfile.o
	make comptime.o; cc -o zsplit zsplit.o zspix.o zsres.o resfile.o comptime.o; rm comptime.o mkctscr

$(ZAPOBJS):	zap.h

zapcomp:
	cc -O -o zap $(ZAPOBJS)
