RM=rm -f
TMPDIR=/tmp


all: tests
tests: equal1.sam
	@perl tester.pl

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
