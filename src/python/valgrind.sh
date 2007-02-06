#!/bin/sh

. common.sh

eval valgrind --suppressions=valgrind-python.supp python $python_args
