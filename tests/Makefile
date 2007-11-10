# $Id$

TMPDIR=/tmp
CFLAGS=-std=c99 -Werror -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wnested-externs -Wdisabled-optimization -Wundef -Wendif-labels -Wshadow -Wcast-align -Wstrict-aliasing=2 -fstrict-aliasing -Wwrite-strings -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wformat -pipe -O3 -I../src/include
LDFLAGS=-lm

ALL=equal1.sam inf dltest.so dltest2.so dltest3.so dltest4.so timer

all: $(ALL)

inf: inf.c
	$(CC) $(CFLAGS) -o $@ $<

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
	$(CC) $(CFLAGS) -o $@ $<

flop.sam: flop
	@./flop >$(TMPDIR)/$@

flop-bench: flop-bench.o
	$(CC) $(LDFLAGS) -o $@ $^

timer: timer.o
	$(CC) $(LDFLAGS) -lsam -L../build/libsam -o $@ $^

clean:
	$(RM) equal*.sam flop $(TMPDIR)/flop.sam flop-bench.o flop-bench timer.o $(ALL)
