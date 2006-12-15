# $Id$

RM=rm -f
XSLTPROC=xsltproc
PREFIX=/usr/share/xml/docbook/stylesheet/nwalsh
OUT=samiam.1

all: $(OUT)

clean:
	$(RM) $(OUT)

%: %.xml
	$(XSLTPROC) -nonet $(PREFIX)/manpages/docbook.xsl $<
