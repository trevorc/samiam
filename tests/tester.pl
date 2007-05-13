#!/usr/bin/perl
# tester.pl        tester application
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
# Revision 1.8  2007/01/04 06:11:52  trevor
# Update for new system.
#
# Revision 1.7  2006/12/19 23:41:45  trevor
# return values of infinity are handled by invoking a helper C program to get
# the system value of infinity cast to int.
#
# Revision 1.6  2006/12/19 05:37:08  trevor
# Swallow stdout from commands for future use.
#
# Revision 1.5  2006/12/17 15:29:36  trevor
# Typo.
#
# Revision 1.4  2006/12/17 15:27:26  trevor
# Compare the return value with the the expected value & 0xff.
#
# Revision 1.3  2006/12/12 23:31:36  trevor
# Added the Id and Log tags and copyright notice where they were missing.
#

use strict;
use warnings;

my $app = "../build/samiam/samiam";
my $filename = "tests.db";
my $sysinf = 0;
my @tests = ();
my @pids = ();

if (@ARGV > 0) {
    $filename = $ARGV[0];
}

system './inf';
if ($? == -1) {
    die "couldn't execute ./inf: $!.\n";
}
$sysinf = $? >> 8;

print "failed test cases:\n";
print "actual\texpected\ttest case\n";
open DB, "<$filename" or die "couldn't open $filename: $!.\n";
while (<DB>) {
    my $pid = fork;

    die "couldn't fork: $!.\n" unless (defined ($pid));
    push @pids, $pid;
    if ($pid == 0) {
	/\S/ or next;
	my ($test, $rv) = split /\s+/;
	$rv =~ /inf/ and $rv = $sysinf;
	my $output = `$app -q $test`;
	if ($? == -1) {
	    print "couldn't execute $app: $!.\n";
	} elsif ($? & 127) {
	    printf "$test died with signal %d.\n", ($? & 127);
	} elsif (($? >> 8) != ($rv & 0xff)) {
	    printf "%d\t%d\t\t$test\n", ($? >> 8), ($rv & 0xff);
	}
	exit;
    }
}
for my $pid(@pids) {
    waitpid $pid, 0;
}
close DB;
