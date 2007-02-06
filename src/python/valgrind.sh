#!/bin/sh

. common.sh

eval valgrind --suppressions=$oldpwd/valgrind-python.supp $python_cmd
