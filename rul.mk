CC=gcc
RM=rm -f
LDFLAGS=-lm
WFLAGS=-ansi -pedantic -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wunreachable-code -Wnested-externs -Wdeclaration-after-statement
#OFLAGS=-pipe -g -ggdb
OFLAGS=-pipe -O3 -msse3 -march=pentium-m
INCLUDES=-I../include
CFLAGS=$(WFLAGS) $(OFLAGS) $(INCLUDES)

VERSION=.1.0.0
MAJOR=.1
