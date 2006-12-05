include rul.mk

DESTDIR=
PREFIX=/usr/local
INSTALL=install

all:
clean:

install:
	$(INSTALL) -vd $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -vd $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -v libsam/libsam.so$(VERSION) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -v samiam/samiam $(DESTDIR)$(PREFIX)/bin

uninstall:
	$(RM) $(PREFIX)/lib/libsam.so$(VERSION)
	$(RM) $(PREFIX)/lib/libsam.so$(MAJOR)
	$(RM) $(PREFIX)/lib/libsam.so
	$(RM) $(PREFIX)/bin/samiam

check: all
	$(MAKE) -C tests

%:
	$(MAKE) -C libsam $@
	$(MAKE) -C samiam $@
	$(MAKE) -C spawn $@
	$(MAKE) -C jbc_test $@
