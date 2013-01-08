program := samiam

CPPFLAGS += -D_BSD_SOURCE -D_XOPEN_SOURCE=600 -I/usr/local/include
CFLAGS   += -std=c99 -pipe -O3 \
            -Wall -Wextra -Werror -Wcast-align -Wstrict-overflow -Wshadow \
            -Wstrict-aliasing
LDFLAGS  += -L/usr/local/lib

objects  := $(patsubst %.c,%.o,$(wildcard *.c))

$(program): $(objects)

.PHONY: clean
clean:
	-rm -f $(program) $(objects)
