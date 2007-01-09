# $Id$

CC=gcc
RM=rm -f
WFLAGS=-std=c99 -Werror -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wnested-externs -Wdisabled-optimization -Wundef -Wendif-labels -Wshadow -Wcast-align -Wstrict-aliasing=2 -fstrict-aliasing -Wwrite-strings -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wformat
OFLAGS=-pipe -g -ggdb
#OFLAGS=-pipe -O3
MFLAGS=
#MFLAGS=-msse3 -march=pentium-m
#MFLAGS=-mpower -mpowerpc -mcpu=powerpc
#MFLAGS=-mthumb-interwork -msoft-float
INCLUDES=-I../include
DEFS=
CFLAGS=$(WFLAGS) $(OFLAGS) $(MFLAGS) $(INCLUDES) $(DEFS)
LDFLAGS=-lm

VERSION=.1.0.0
MAJOR=.1
