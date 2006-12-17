# $Id$

LDFLAGS+=-L../libsam -lsam

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^

clean:
	$(RM) $(NAME) $(OBJS)
