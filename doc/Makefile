# $Id$

Z=gz
RM=rm -f
COMPRESS=gzip
OUT=samiam.1.gz

.SUFFIXES: .$(Z)

all: $(OUT)

clean:
	$(RM) $(OUT)

%.$(Z): %
	$(COMPRESS) -c --best $< >$@

