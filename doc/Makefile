# $Id$

RM=rm -f
XSLTPROC=xsltproc
PREFIX=/usr/share/apps/ksgmltools2/docbook/xsl
OUT=samiam.1

all: $(OUT)

clean:
	$(RM) $(OUT)

%: %.xml
	$(XSLTPROC) -nonet $(PREFIX)/manpages/docbook.xsl $<
