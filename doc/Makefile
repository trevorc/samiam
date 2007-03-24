# $Id$

RM=rm -f
XSLTPROC=xsltproc
PREFIX=/usr/share/xml/docbook/stylesheet/nwalsh
OUT=samiam.1 libsam.html

all: $(OUT)

clean:
	$(RM) $(OUT)

%.1: %.1.xml
	$(XSLTPROC) -o $@ -nonet $(PREFIX)/manpages/docbook.xsl $<

%.html: %.xml
	$(XSLTPROC) -o $@ -nonet $(PREFIX)/xhtml/docbook.xsl $<
