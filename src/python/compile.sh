#!/bin/sh
if [ ! -d ../../build/python ]; then
    mkdir -p ../../build/python
fi

if [ ! -f ../../build/libsam/libsam.so* ]; then
    scons -C ../..
fi

CFLAGS='-std=c99 -Wall -W -g -ggdb' python setup.py build
