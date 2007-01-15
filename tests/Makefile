# $Id$

include ../rul.mk

RM=rm -f
TMPDIR=/tmp
ALL=equal1.sam inf dltest.so dltest2.so dltest3.so dltest4.so

all: $(ALL)

inf: inf.c
	$(CC) -pipe -O3 $(MFLAGS) -o $@ $<

%.so: %.c
	$(CC) -o $@ -lc -shared -Wl,-soname,$@ -fPIC $(CFLAGS) $(LDFLAGS) $<

dltest.so: dltest.c
dltest2.so: dltest2.c
dltest3.so: dltest3.c
dltest4.so: dltest4.c
	$(CC) -o $@ -lc -shared -Wl,-soname,$@ -fPIC $(CFLAGS) $(LDFLAGS) $<

check: all
	@LD_LIBRARY_PATH=../build/libsam:. perl tester.pl

equal1.sam: gen_equal.pl
	@perl gen_equal.pl

flop: flop.c
	$(CC) -pipe -O3 -o $@ $<

flop.sam: flop
	@./flop >$(TMPDIR)/$@

flop-bench: flop-bench.o
	$(CC) -lm -o $@ $^

clean:
	$(RM) equal*.sam flop $(TMPDIR)/flop.sam flop-bench.o flop-bench $(ALL)
