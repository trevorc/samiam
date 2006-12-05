#!/usr/bin/perl
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
