# $Id$

include ../rul.mk

CFLAGS+=-fPIC
LDFLAGS+=-lc -shared -Wl,-soname,libsam.so -ldl -rdynamic
NAME=libsam
OBJS=sam_error.o sam_execute_types.o sam_main.o sam_parse.o sam_util.o sam_error.o sam_string.o sam_hash_table.o sam_array.o sam_execute.o sam_opcode.o sam_io.o sam_es.o

all: $(NAME).so
$(NAME).so: $(OBJS)
	$(CC) -o $@$(VERSION) $(LDFLAGS) $^
	ln -sf $@$(VERSION) $@$(MAJOR)
	ln -sf $@$(VERSION) $@

clean:
	$(RM) libsam.so libsam.so$(MAJOR) libsam.so$(VERSION)
	$(RM) $(OBJS)
