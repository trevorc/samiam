#!/bin/sh
# $Id$

valgrind=`which valgrind`
. paths.sh
$valgrind --tool=memcheck --leak-check=yes samiam $@
