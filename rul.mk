# $Id$

CC=gcc
RM=rm -f
WFLAGS=-ansi -pedantic -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wunreachable-code -Wnested-externs -Wdeclaration-after-statement
OFLAGS=-pipe -g -ggdb
#OFLAGS=-pipe -O3 -msse3 -march=pentium-m
INCLUDES=-I../include
DEFS=
CFLAGS=$(WFLAGS) $(OFLAGS) $(INCLUDES) $(DEFS)
LDFLAGS=-lm

VERSION=.1.0.0
MAJOR=.1
