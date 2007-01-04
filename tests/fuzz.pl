#!/usr/bin/perl
# $Id$
#
# part of samiam - the fast sam interpreter
#
# Copyright (c) 2006 Trevor Caira, Jimmy Hartzell
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# $Log$
# Revision 1.1  2007/01/04 01:06:40  trevor
# Generate somewhat random sam programs.
#

use strict;
use warnings;

my @opcodes = (
    "FTOI", "FTOIR", "ITOF", "PUSHIMM", "PUSHIMMF", "PUSHIMMCH", "PUSHIMMMA",
    "PUSHIMMPA", "PUSHIMMSTR", "PUSHSP", "PUSHFBR", "POPSP", "POPFBR", "DUP",
    "SWAP", "ADDSP", "MALLOC", "FREE", "PUSHIND", "STOREIND", "PUSHABS",
    "STOREABS", "PUSHOFF", "STOREOFF", "ADD", "SUB", "TIMES", "DIV", "MOD",
    "ADDF", "SUBF", "TIMESF", "DIVF", "LSHIFT", "LSHIFTIND", "RSHIFT",
    "RSHIFTIND", "LRSHIFT", "LRSHIFTIND", "AND", "OR", "NAND", "NOR", "XOR",
    "NOT", "BITAND", "BITOR", "BITNAND", "BITNOR", "BITXOR", "BITNOT", "CMP",
    "CMPF", "GREATER", "LESS", "EQUAL", "ISNIL", "ISPOS", "ISNEG", "JUMP",
    "JUMPC", "JUMPIND", "RST", "JSR", "JSRIND", "SKIP", "LINK", "UNLINK",
    "READ", "READF", "READCH", "READSTR", "WRITE", "WRITEF", "WRITECH",
    "WRITESTR", "STOP", "patoi", "import", "call",
);

#my $chars = "`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./~!@#\$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?";
my $chars = "1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

my $rand_string = sub {
    my $str = '';
    for (0 .. int(rand(20))) {
	my $index = int(rand(length $chars)) - 1;
	$str .= substr $chars, $index, 1;
    }
    return $str;
};

my @operands = (
    $rand_string,
    sub {
	return '';
    },
    sub {
	return int(rand(65535)) - 32768;
    },
    sub {
	return rand(40000) - 20000;
    },
);

my @line = (
    sub {
	return $rand_string->() . ":";
    },
    sub {
	return $opcodes[int(rand(@opcodes))];
    },
    sub {
	return $opcodes[int(rand(@opcodes))] . ' ' .
	       $operands[int(rand(@operands))]->();
    },
);

for (0 .. int(rand(5) + 5)) {
    print $line[int(rand(@line))]->() . "\n";
}
