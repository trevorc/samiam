#!/bin/sh
# $Id$

export LD_PRELOAD=libsam/libsam.so.1.0.0
gdb samiam/samiam
