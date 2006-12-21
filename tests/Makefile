# $Id$

RM=rm -f
TMPDIR=/tmp
CFLAGS=-Wall -W -ansi -pedantic -pipe -O3

all: equal1.sam inf

inf: inf.c

check: all
	@LD_PRELOAD=../libsam/libsam.so.1.0.0 perl tester.pl

equal1.sam: gen_equal.pl
	@perl gen_equal.pl

flop: flop.c

flop.sam: flop
	@./flop >$(TMPDIR)/$@

flop-bench: flop-bench.o
	$(CC) -lm -o $@ $^

clean:
	$(RM) equal*.sam flop $(TMPDIR)/flop.sam flop-bench.o flop-bench inf
