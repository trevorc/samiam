# $Id$

include rul.mk

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
MANDIR=$(PREFIX)/share/man
INSTALL=install

all:
clean:

doc:
	$(MAKE) -Cdoc

install:
	$(INSTALL) -vd $(DESTDIR)$(LIBDIR)
	$(INSTALL) -vd $(DESTDIR)$(BINDIR)
	$(INSTALL) -vd $(DESTDIR)$(MANDIR)
	$(INSTALL) -vd $(DESTDIR)$(MANDIR)/man1
	$(INSTALL) -v libsam/libsam.so$(VERSION) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -v samiam/samiam $(DESTDIR)$(BINDIR)
	( cd $(DESTDIR)$(LIBDIR) && \
	  ln -sf libsam.so$(VERSION) libsam.so$(MAJOR) && \
	  ln -sf libsam.so$(VERSION) libsam.so )
	$(INSTALL) -vm 644 doc/samiam.1 $(DESTDIR)$(MANDIR)/man1

uninstall:
	$(RM) $(LIBDIR)/libsam.so$(VERSION)
	$(RM) $(LIBDIR)/libsam.so$(MAJOR)
	$(RM) $(LIBDIR)/libsam.so
	$(RM) $(BINDIR)/samiam
	$(RM) $(MANDIR)/man1/samiam.1

check: all
	$(MAKE) -Ctests $@

%:
	$(MAKE) -Clibsam $@
	$(MAKE) -Csamiam $@
	$(MAKE) -Cspawn $@
	$(MAKE) -Cjbc_test $@
	$(MAKE) -Ctests $@
