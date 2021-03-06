#!/usr/bin/perl
# tester.pl        generate the equal*.sam test cases
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
# Revision 1.2  2006/12/12 23:31:36  trevor
# Added the $Id$ and $Log$ tags and copyright notice where they were missing.
#

use strict;
use warnings;

my $testnum = 1;
my @firsts = ("PUSHIMM 51", "PUSHIMMF 3.0", "PUSHIMMCH '3'", "PUSHIMMMA 0", "ADDSP 1", "PUSHIMMPA 4");
my @seconds = ("PUSHIMM 51", "PUSHIMMF 3.0", "PUSHIMMCH '0'", "PUSHIMMMA 0", "ADDSP 1", "PUSHIMMPA 6");

for my $first(@firsts) {
    for my $second(@seconds) {
	open OUT, ">equal$testnum.sam"
	    or die "couldn't open equal$testnum.sam";
	print OUT $first . "\n" . $second . "\n";
	print OUT "EQUAL\n";
	print OUT "STOP\n";
	close OUT;
	++$testnum;
    }
}
