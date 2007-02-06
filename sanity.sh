#!/bin/sh
# $Id$

. paths.sh
printf 'PUSHIMM 123 STOP' | samiam
ret=$?
if [ $ret == 123 ]; then
    echo OK
    exit 0
else
    echo Fail: $ret
    exit 1
fi
