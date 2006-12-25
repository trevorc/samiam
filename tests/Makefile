# $Id$

include ../rul.mk

RM=rm -f
TMPDIR=/tmp

all: equal1.sam inf dltest.so dltest2.so

inf: inf.c
	$(CC) -pipe -O3 -o $@ $<

dltest.so: dltest.c
	$(CC) -o $@ -lc -shared -Wl,-soname,$@ -fPIC $(CFLAGS) $(LDFLAGS) $<

dltest2.so: dltest2.c
	$(CC) -o $@ -lc -shared -Wl,-soname,$@ -fPIC $(CFLAGS) $(LDFLAGS) $<

check: all
	@LD_LIBRARY_PATH=../libsam:. perl tester.pl

equal1.sam: gen_equal.pl
	@perl gen_equal.pl

flop: flop.c
	$(CC) -pipe -O3 -o $@ $<

flop.sam: flop
	@./flop >$(TMPDIR)/$@

flop-bench: flop-bench.o
	$(CC) -lm -o $@ $^

clean:
	$(RM) equal*.sam flop $(TMPDIR)/flop.sam flop-bench.o flop-bench inf
