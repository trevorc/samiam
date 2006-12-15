# $Id$

include ../rul.mk

CFLAGS+=-fPIC
LDFLAGS+=-lc -shared -Wl,-soname,libsam.so
NAME=libsam
OBJS=sam_main.o sam_execute.o sam_parse.o sam_util.o

all: $(NAME).so
$(NAME).so: $(OBJS)
	$(CC) -o $@$(VERSION) $(LDFLAGS) $^
	ln -sf $@$(VERSION) $@$(MAJOR)
	ln -sf $@$(VERSION) $@

clean:
	$(RM) libsam.so libsam.so$(MAJOR) libsam.so$(VERSION)
	$(RM) $(OBJS)
