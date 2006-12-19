# $Id$

RM=rm -f
TMPDIR=/tmp

all: equal1.sam inf

inf: inf.c
	$(CC) -o $@ $^

check: equal1.sam
	@LD_PRELOAD=../libsam/libsam.so.1.0.0 perl tester.pl

equal1.sam: gen_equal.pl
	@perl gen_equal.pl

flop: flop.c
	$(CC) -O3 -o $@ $^

flop.sam: flop
	@./flop >$(TMPDIR)/$@

flop-bench: flop-bench.c
	$(CC) -O3 -o $@ $^

clean:
	$(RM) equal*.sam flop $(TMPDIR)/flop.sam flop-bench
