# $Id$

CC=gcc
RM=rm -f
WFLAGS=-std=c99 -pedantic -Werror -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wnested-externs
OFLAGS=-pipe -g -ggdb
OFLAGS=-pipe -O3
#MFLAGS=-msse3 -march=pentium-m
#MFLAGS=-mthumb-interwork -msoft-float
INCLUDES=-I../include
DEFS=
CFLAGS=$(WFLAGS) $(OFLAGS) $(MFLAGS) $(INCLUDES) $(DEFS)
LDFLAGS=-lm

VERSION=.1.0.0
MAJOR=.1
