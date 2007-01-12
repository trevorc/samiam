# $Id$

include ../rul.mk

CFLAGS+=-fPIC
LDFLAGS+=-lc -shared -Wl,-soname,libsam.so -ldl -rdynamic
NAME=libsam
OBJS=error.o execute_types.o main.o parse.o util.o error.o string.o hash_table.o array.o execute.o opcode.o io.o es.o

all: $(NAME).so
$(NAME).so: $(OBJS)
	$(CC) -o $@$(VERSION) $(LDFLAGS) $^
	ln -sf $@$(VERSION) $@$(MAJOR)
	ln -sf $@$(VERSION) $@

clean:
	$(RM) libsam.so libsam.so$(MAJOR) libsam.so$(VERSION)
	$(RM) $(OBJS)
