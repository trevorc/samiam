#!/bin/sh
# $Id$

export LD_PRELOAD=libsam/libsam.so.1.0.0
valgrind --tool=memcheck --leak-check=yes samiam/samiam $@
